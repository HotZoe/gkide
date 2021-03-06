/// @file nvim/event/multiqueue.c
///
/// Multi-level queue for selective async event processing.
/// Not threadsafe; access must be synchronized externally.
///
/// Multiqueue supports a parent-child relationship with these properties:
/// - pushing a node to a child queue will push a corresponding link node
///   to the parent queue
/// - removing a link node from a parent queue will remove the next node
///   in the linked child queue
/// - removing a node from a child queue will remove the corresponding
///   link node in the parent queue
///
/// These properties allow Nvim to organize and process events from different
/// sources with a certain degree of control. How the multiqueue is used:
///
///                         +----------------+
///                         |   Main loop    |
///                         +----------------+
///
///                         +----------------+
///         +-------------->|   Event loop   |<------------+
///         |               +--+-------------+             |
///         |                  ^           ^               |
///         |                  |           |               |
///    +-----------+   +-----------+    +---------+    +---------+
///    | Channel 1 |   | Channel 2 |    |  Job 1  |    |  Job 2  |
///    +-----------+   +-----------+    +---------+    +---------+
///
///
/// The lower boxes represent event emitters, each with its own private queue
/// having the event loop queue as the parent.
///
/// When idle, the main loop spins the event loop which queues events from many
/// sources (channels, jobs, user...). Each event emitter pushes events to its
/// private queue which is propagated to the event loop queue. When the main
/// loop consumes an event, the corresponding event is removed from the
/// emitter's queue.
///
/// The main reason for this queue hierarchy is to allow focusing on a single
/// event emitter while blocking the main loop. For example, if the `jobwait`
/// VimL function is called on job1, the main loop will temporarily stop
/// polling the event loop queue and poll job1 queue instead. Same with
/// channels, when calling `rpcrequest` we want to temporarily stop processing
/// events from other sources and focus on a specific channel.

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include <uv.h>

#include "nvim/event/multiqueue.h"
#include "nvim/memory.h"
#include "nvim/os/time.h"

typedef struct multiqueue_item_s multiqueue_item_st;
struct multiqueue_item_s
{
    union
    {
        multiqueue_st *queue;
        struct
        {
            event_msg_st event;
            multiqueue_item_st *parent_item;
        } item;
    } data;
    /// true: current item is just a link to a node in a child queue
    bool link;
    queue_st node;
};

struct multiqueue_s
{
    multiqueue_st *parent; ///<
    queue_st headtail;     ///< circularly-linked
    put_callback_ft put_cb;///<
    void *data;            ///<
    size_t size;           ///<
};

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "event/multiqueue.c.generated.h"
#endif

static event_msg_st nil_event = { 0 }; // { .handler = NULL, .argv = {NULL} }

multiqueue_st *multiqueue_new_parent(put_callback_ft put_cb, void *data)
{
    return multiqueue_new(NULL, put_cb, data);
}

multiqueue_st *multiqueue_new_child(multiqueue_st *parent)
FUNC_ATTR_NONNULL_ALL
{
    // parent cannot have a parent, more like a "root"
    assert(!parent->parent);

    parent->size++;

    return multiqueue_new(parent, NULL, NULL);
}

static multiqueue_st *multiqueue_new(multiqueue_st *parent,
                                     put_callback_ft put_cb,
                                     void *data)
{
    multiqueue_st *rv = xmalloc(sizeof(multiqueue_st));
    queue_init(&rv->headtail);
    rv->size = 0;
    rv->parent = parent;
    rv->put_cb = put_cb;
    rv->data = data;
    return rv;
}

void multiqueue_free(multiqueue_st *ptr)
{
    assert(ptr);

    while(!queue_empty(&ptr->headtail))
    {
        queue_st *q = QUEUE_HEAD(&ptr->headtail);
        multiqueue_item_st *item = multiqueue_node_data(q);

        if(ptr->parent)
        {
            queue_remove(&item->data.item.parent_item->node);
            xfree(item->data.item.parent_item);
        }

        queue_remove(q);
        xfree(item);
    }

    xfree(ptr);
}

/// Removes the next item and returns its Event.
event_msg_st multiqueue_get(multiqueue_st *ptr)
{
    return multiqueue_empty(ptr) ? nil_event : multiqueue_remove(ptr);
}

void multiqueue_put_event(multiqueue_st *ptr, event_msg_st event)
{
    assert(ptr);
    multiqueue_push(ptr, event);

    if(ptr->parent && ptr->parent->put_cb)
    {
        ptr->parent->put_cb(ptr->parent, ptr->parent->data);
    }
}

void multiqueue_process_events(multiqueue_st *ptr)
{
    assert(ptr);

    while(!multiqueue_empty(ptr))
    {
        event_msg_st event = multiqueue_remove(ptr);

        if(event.handler)
        {
            event.handler(event.argv);
        }
    }
}

/// Removes all events without processing them.
void multiqueue_purge_events(multiqueue_st *ptr)
{
    assert(ptr);

    while(!multiqueue_empty(ptr))
    {
        (void)multiqueue_remove(ptr);
    }
}

bool multiqueue_empty(multiqueue_st *ptr)
{
    assert(ptr);
    return queue_empty(&ptr->headtail);
}

void multiqueue_replace_parent(multiqueue_st *ptr, multiqueue_st *new_parent)
{
    assert(multiqueue_empty(ptr));
    ptr->parent = new_parent;
}

/// Gets the count of all events currently in the queue.
size_t multiqueue_size(multiqueue_st *ptr)
{
    return ptr->size;
}

/// Gets an Event from an item.
///
/// @param remove   Remove the node from its queue, and free it.
static event_msg_st multiqueueitem_get_event(multiqueue_item_st *item, bool remove)
{
    assert(item != NULL);
    event_msg_st ev;

    if(item->link)
    {
        // get the next node in the linked queue
        multiqueue_st *linked = item->data.queue;
        assert(!multiqueue_empty(linked));

        multiqueue_item_st *child =
            multiqueue_node_data(QUEUE_HEAD(&linked->headtail));

        ev = child->data.item.event;

        // remove the child node
        if(remove)
        {
            queue_remove(&child->node);
            xfree(child);
        }
    }
    else
    {
        // remove the corresponding link node in the parent queue
        if(remove && item->data.item.parent_item)
        {
            queue_remove(&item->data.item.parent_item->node);
            xfree(item->data.item.parent_item);
            item->data.item.parent_item = NULL;
        }

        ev = item->data.item.event;
    }

    return ev;
}

static event_msg_st multiqueue_remove(multiqueue_st *ptr)
{
    assert(!multiqueue_empty(ptr));
    queue_st *h = QUEUE_HEAD(&ptr->headtail);
    queue_remove(h);
    multiqueue_item_st *item = multiqueue_node_data(h);

    // Only a parent queue has link-nodes
    assert(!item->link || !ptr->parent);

    event_msg_st ev = multiqueueitem_get_event(item, true);
    ptr->size--;
    xfree(item);

    return ev;
}

static void multiqueue_push(multiqueue_st *ptr, event_msg_st event)
{
    multiqueue_item_st *item = xmalloc(sizeof(multiqueue_item_st));
    item->link = false;
    item->data.item.event = event;
    item->data.item.parent_item = NULL;
    queue_insert_tail(&ptr->headtail, &item->node);

    if(ptr->parent)
    {
        // push link node to the parent queue
        item->data.item.parent_item = xmalloc(sizeof(multiqueue_item_st));
        item->data.item.parent_item->link = true;
        item->data.item.parent_item->data.queue = ptr;

        queue_insert_tail(&ptr->parent->headtail,
                          &item->data.item.parent_item->node);
    }

    ptr->size++;
}

static multiqueue_item_st *multiqueue_node_data(queue_st *q)
{
    return QUEUE_DATA(q, multiqueue_item_st, node);
}
