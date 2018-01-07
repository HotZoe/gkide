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

typedef struct multiqueue_item MultiQueueItem;

struct multiqueue_item
{
    union
    {
        MultiQueue *queue;
        struct
        {
            Event event;
            MultiQueueItem *parent_item;
        } item;
    } data;
    /// true: current item is just a link to a node in a child queue
    bool link;
    queue_st node;
};

struct multiqueue
{
    MultiQueue *parent;  ///<
    queue_st headtail;    ///< circularly-linked
    put_callback put_cb; ///<
    void *data;          ///<
    size_t size;         ///<
};

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "event/multiqueue.c.generated.h"
#endif

static Event nil_event = { 0 }; // { .handler = NULL, .argv = {NULL} }

MultiQueue *multiqueue_new_parent(put_callback put_cb, void *data)
{
    return multiqueue_new(NULL, put_cb, data);
}

MultiQueue *multiqueue_new_child(MultiQueue *parent)
FUNC_ATTR_NONNULL_ALL
{
    // parent cannot have a parent, more like a "root"
    assert(!parent->parent);

    parent->size++;

    return multiqueue_new(parent, NULL, NULL);
}

static MultiQueue *multiqueue_new(MultiQueue *parent,
                                  put_callback put_cb,
                                  void *data)
{
    MultiQueue *rv = xmalloc(sizeof(MultiQueue));
    queue_init(&rv->headtail);
    rv->size = 0;
    rv->parent = parent;
    rv->put_cb = put_cb;
    rv->data = data;
    return rv;
}

void multiqueue_free(MultiQueue *ptr)
{
    assert(ptr);

    while(!queue_empty(&ptr->headtail))
    {
        queue_st *q = QUEUE_HEAD(&ptr->headtail);
        MultiQueueItem *item = multiqueue_node_data(q);

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
Event multiqueue_get(MultiQueue *ptr)
{
    return multiqueue_empty(ptr) ? nil_event : multiqueue_remove(ptr);
}

void multiqueue_put_event(MultiQueue *ptr, Event event)
{
    assert(ptr);
    multiqueue_push(ptr, event);

    if(ptr->parent && ptr->parent->put_cb)
    {
        ptr->parent->put_cb(ptr->parent, ptr->parent->data);
    }
}

void multiqueue_process_events(MultiQueue *ptr)
{
    assert(ptr);

    while(!multiqueue_empty(ptr))
    {
        Event event = multiqueue_remove(ptr);

        if(event.handler)
        {
            event.handler(event.argv);
        }
    }
}

/// Removes all events without processing them.
void multiqueue_purge_events(MultiQueue *ptr)
{
    assert(ptr);

    while(!multiqueue_empty(ptr))
    {
        (void)multiqueue_remove(ptr);
    }
}

bool multiqueue_empty(MultiQueue *ptr)
{
    assert(ptr);
    return queue_empty(&ptr->headtail);
}

void multiqueue_replace_parent(MultiQueue *ptr, MultiQueue *new_parent)
{
    assert(multiqueue_empty(ptr));
    ptr->parent = new_parent;
}

/// Gets the count of all events currently in the queue.
size_t multiqueue_size(MultiQueue *ptr)
{
    return ptr->size;
}

/// Gets an Event from an item.
///
/// @param remove   Remove the node from its queue, and free it.
static Event multiqueueitem_get_event(MultiQueueItem *item, bool remove)
{
    assert(item != NULL);
    Event ev;

    if(item->link)
    {
        // get the next node in the linked queue
        MultiQueue *linked = item->data.queue;
        assert(!multiqueue_empty(linked));

        MultiQueueItem *child =
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

static Event multiqueue_remove(MultiQueue *ptr)
{
    assert(!multiqueue_empty(ptr));
    queue_st *h = QUEUE_HEAD(&ptr->headtail);
    queue_remove(h);
    MultiQueueItem *item = multiqueue_node_data(h);

    // Only a parent queue has link-nodes
    assert(!item->link || !ptr->parent);

    Event ev = multiqueueitem_get_event(item, true);
    ptr->size--;
    xfree(item);

    return ev;
}

static void multiqueue_push(MultiQueue *ptr, Event event)
{
    MultiQueueItem *item = xmalloc(sizeof(MultiQueueItem));
    item->link = false;
    item->data.item.event = event;
    item->data.item.parent_item = NULL;
    queue_insert_tail(&ptr->headtail, &item->node);

    if(ptr->parent)
    {
        // push link node to the parent queue
        item->data.item.parent_item = xmalloc(sizeof(MultiQueueItem));
        item->data.item.parent_item->link = true;
        item->data.item.parent_item->data.queue = ptr;

        queue_insert_tail(&ptr->parent->headtail,
                          &item->data.item.parent_item->node);
    }

    ptr->size++;
}

static MultiQueueItem *multiqueue_node_data(queue_st *q)
{
    return QUEUE_DATA(q, MultiQueueItem, node);
}
