/// @file nvim/msgpack/helpers.c

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include <msgpack.h>

#include "nvim/api/private/dispatch.h"
#include "nvim/api/private/helpers.h"
#include "nvim/msgpack/helpers.h"
#include "nvim/lib/kvec.h"
#include "nvim/vim.h"
#include "nvim/log.h"
#include "nvim/memory.h"
#include "nvim/assert.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "msgpack/helpers.c.generated.h"
#endif

static msgpack_zone zone;
static msgpack_sbuffer sbuffer;

#define HANDLE_TYPE_CONVERSION_IMPL(t, lt)                                  \
    static bool rpc_to_##lt(const msgpack_object *const obj,                \
                            Integer *const arg)                             \
    FUNC_ATTR_NONNULL_ALL                                                   \
    FUNC_ATTR_WARN_UNUSED_RESULT                                            \
    {                                                                       \
        if(obj->type != MSGPACK_OBJECT_EXT                                  \
           || obj->via.ext.type + EXT_OBJECT_TYPE_SHIFT != kObjectType##t)  \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        msgpack_object data;                                                \
        msgpack_unpack_return ret = msgpack_unpack(obj->via.ext.ptr,        \
                                                   obj->via.ext.size,       \
                                                   NULL,                    \
                                                   &zone,                   \
                                                   &data);                  \
                                                                            \
        if(ret != MSGPACK_UNPACK_SUCCESS)                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *arg = (handle_kt)data.via.i64;                                     \
        return true;                                                        \
    }                                                                       \
                                                                            \
    static void rpc_from_##lt(Integer o, msgpack_packer *res)               \
    FUNC_ATTR_NONNULL_ARG(2)                                                \
    {                                                                       \
        msgpack_packer pac;                                                 \
        msgpack_packer_init(&pac, &sbuffer, msgpack_sbuffer_write);         \
        msgpack_pack_int64(&pac, (handle_kt)o);                             \
        msgpack_pack_ext(res, sbuffer.size,                                 \
                         kObjectType##t - EXT_OBJECT_TYPE_SHIFT);           \
        msgpack_pack_ext_body(res, sbuffer.data, sbuffer.size);             \
        msgpack_sbuffer_clear(&sbuffer);                                    \
    }

void rpc_helpers_init(void)
{
    msgpack_zone_init(&zone, 0xfff);
    msgpack_sbuffer_init(&sbuffer);
}

HANDLE_TYPE_CONVERSION_IMPL(Buffer, buffer)
HANDLE_TYPE_CONVERSION_IMPL(Window, window)
HANDLE_TYPE_CONVERSION_IMPL(Tabpage, tabpage)

typedef struct
{
    const msgpack_object *mobj;
    Object *aobj;
    bool container;
    size_t idx;
} MPToAPIObjectStackItem;

/// Convert type used by msgpack parser to Nvim API type.
///
/// @param[in]  obj  Msgpack value to convert.
/// @param[out] arg  Location where result of conversion will be saved.
///
/// @return true in case of success, false otherwise.
bool rpc_to_object(const msgpack_object *const obj, Object *const arg)
FUNC_ATTR_NONNULL_ALL
{
    bool ret = true;

    kvec_t(MPToAPIObjectStackItem) stack = KV_INITIAL_VALUE;

    kv_push(stack, ((MPToAPIObjectStackItem)
    {
        obj, arg, false, 0
    }));

    while(ret && kv_size(stack))
    {
        MPToAPIObjectStackItem cur = kv_last(stack);

        if(!cur.container)
        {
            *cur.aobj = NIL;
        }

        switch(cur.mobj->type)
        {
            case MSGPACK_OBJECT_NIL:
            {
                break;
            }

            case MSGPACK_OBJECT_BOOLEAN:
            {
                *cur.aobj = BOOLEAN_OBJ(cur.mobj->via.boolean);
                break;
            }

            case MSGPACK_OBJECT_NEGATIVE_INTEGER:
            {
                STATIC_ASSERT(sizeof(Integer) == sizeof(cur.mobj->via.i64),
                              "Msgpack integer size does not match API integer");

                *cur.aobj = INTEGER_OBJ(cur.mobj->via.i64);

                break;
            }

            case MSGPACK_OBJECT_POSITIVE_INTEGER:
            {
                STATIC_ASSERT(sizeof(Integer) == sizeof(cur.mobj->via.u64),
                              "Msgpack integer size does not match API integer");

                if(cur.mobj->via.u64 > API_INTEGER_MAX)
                {
                    ret = false;
                }
                else
                {
                    *cur.aobj = INTEGER_OBJ((Integer)cur.mobj->via.u64);
                }

                break;
            }

        #ifdef NVIM_MSGPACK_HAS_FLOAT32
            case MSGPACK_OBJECT_FLOAT32:
            case MSGPACK_OBJECT_FLOAT64:
        #else
            case MSGPACK_OBJECT_FLOAT:
        #endif
                {
                    STATIC_ASSERT(sizeof(Float) == sizeof(cur.mobj->via.f64),
                                  "Msgpack floating-point size does not match API integer");

                    *cur.aobj = FLOAT_OBJ(cur.mobj->via.f64);

                    break;
                }

#define STR_CASE(type, attr, obj, dest, conv)                                 \
case type:                                                                    \
{                                                                             \
    dest = conv(((String) {                                                   \
        .size = obj->via.attr.size,                                           \
        .data = (obj->via.attr.ptr == NULL                                    \
                 ||  obj->via.attr.size == 0                                  \
                     ? xmemdupz("", 0)                                        \
                     : xmemdupz(obj->via.attr.ptr, obj->via.attr.size)), })); \
    break;                                                                    \
}
            STR_CASE(MSGPACK_OBJECT_STR, str, cur.mobj, *cur.aobj, STRING_OBJ)
            STR_CASE(MSGPACK_OBJECT_BIN, bin, cur.mobj, *cur.aobj, STRING_OBJ)

            case MSGPACK_OBJECT_ARRAY:
            {
                const size_t size = cur.mobj->via.array.size;

                if(cur.container)
                {
                    if(cur.idx >= size)
                    {
                        (void)kv_pop(stack);
                    }
                    else
                    {
                        const size_t idx = cur.idx;
                        cur.idx++;
                        kv_last(stack) = cur;

                        kv_push(stack, ((MPToAPIObjectStackItem) {
                            .mobj = &cur.mobj->via.array.ptr[idx],
                            .aobj = &cur.aobj->data.array.items[idx],
                            .container = false,
                        }));
                    }
                }
                else
                {
                    *cur.aobj = ARRAY_OBJ(((Array) {
                        .size = size,
                        .capacity = size,
                        .items = (size > 0
                                  ? xcalloc(size,
                                            sizeof(*cur.aobj->data.array.items))
                                  : NULL),
                    }));

                    cur.container = true;
                    kv_last(stack) = cur;
                }

                break;
            }

            case MSGPACK_OBJECT_MAP:
            {
                const size_t size = cur.mobj->via.map.size;

                if(cur.container)
                {
                    if(cur.idx >= size)
                    {
                        (void)kv_pop(stack);
                    }
                    else
                    {
                        const size_t idx = cur.idx;
                        cur.idx++;
                        kv_last(stack) = cur;
                        const msgpack_object *const key =
                            &cur.mobj->via.map.ptr[idx].key;

                        switch(key->type)
                        {
                            #define ID(x) x

                            STR_CASE(MSGPACK_OBJECT_STR,
                                     str,
                                     key,
                                     cur.aobj->data.dictionary.items[idx].key,
                                     ID)

                            STR_CASE(MSGPACK_OBJECT_BIN,
                                     bin,
                                     key,
                                     cur.aobj->data.dictionary.items[idx].key,
                                     ID)

                            #undef ID

                            case MSGPACK_OBJECT_NIL:
                            case MSGPACK_OBJECT_BOOLEAN:
                            case MSGPACK_OBJECT_POSITIVE_INTEGER:
                            case MSGPACK_OBJECT_NEGATIVE_INTEGER:
                        #ifdef NVIM_MSGPACK_HAS_FLOAT32
                            case MSGPACK_OBJECT_FLOAT32:
                            case MSGPACK_OBJECT_FLOAT64:
                        #else
                            case MSGPACK_OBJECT_FLOAT:
                        #endif
                            case MSGPACK_OBJECT_EXT:
                            case MSGPACK_OBJECT_MAP:
                            case MSGPACK_OBJECT_ARRAY:
                            {
                                ret = false;
                                break;
                            }
                        }

                        if(ret)
                        {
                            kv_push(stack, ((MPToAPIObjectStackItem) {
                                .mobj = &cur.mobj->via.map.ptr[idx].val,
                                .aobj = &cur.aobj->data.dictionary.items[idx].value,
                                .container = false,
                            }));
                        }
                    }
                }
                else
                {
                    *cur.aobj = DICTIONARY_OBJ(((Dictionary) {
                        .size = size,
                        .capacity = size,
                        .items = (size > 0
                                  ? xcalloc(size,
                                            sizeof(*cur.aobj->data.dictionary.items))
                                  : NULL),
                    }));

                    cur.container = true;
                    kv_last(stack) = cur;
                }

                break;
            }

            case MSGPACK_OBJECT_EXT:
            {
                switch((object_type_et)(cur.mobj->via.ext.type + EXT_OBJECT_TYPE_SHIFT))
                {
                    case kObjectTypeBuffer:
                    {
                        cur.aobj->type = kObjectTypeBuffer;
                        ret = rpc_to_buffer(cur.mobj,
                                            &cur.aobj->data.integer);
                        break;
                    }

                    case kObjectTypeWindow:
                    {
                        cur.aobj->type = kObjectTypeWindow;
                        ret = rpc_to_window(cur.mobj,
                                            &cur.aobj->data.integer);
                        break;
                    }

                    case kObjectTypeTabpage:
                    {
                        cur.aobj->type = kObjectTypeTabpage;
                        ret = rpc_to_tabpage(cur.mobj,
                                             &cur.aobj->data.integer);
                        break;
                    }

                    case kObjectTypeNil:
                    case kObjectTypeBoolean:
                    case kObjectTypeInteger:
                    case kObjectTypeFloat:
                    case kObjectTypeString:
                    case kObjectTypeArray:
                    case kObjectTypeDictionary:
                    {
                        break;
                    }
                }

                break;
            }
#undef STR_CASE
        }

        if(!cur.container)
        {
            (void)kv_pop(stack);
        }
    }

    kv_destroy(stack);

    return ret;
}

static bool rpc_to_string(const msgpack_object *const obj,
                          String *const arg)
FUNC_ATTR_NONNULL_ALL
{
    if(obj->type == MSGPACK_OBJECT_BIN || obj->type == MSGPACK_OBJECT_STR)
    {
        arg->data = obj->via.bin.ptr != NULL
        ? xmemdupz(obj->via.bin.ptr, obj->via.bin.size) : NULL;
        arg->size = obj->via.bin.size;

        return true;
    }

    return false;
}

bool rpc_to_array(const msgpack_object *const obj, Array *const arg)
FUNC_ATTR_NONNULL_ALL
{
    if(obj->type != MSGPACK_OBJECT_ARRAY)
    {
        return false;
    }

    arg->size = obj->via.array.size;
    arg->items = xcalloc(obj->via.array.size, sizeof(Object));

    for(uint32_t i = 0; i < obj->via.array.size; i++)
    {
        if(!rpc_to_object(obj->via.array.ptr + i, &arg->items[i]))
        {
            return false;
        }
    }

    return true;
}

bool rpc_to_dictionary(const msgpack_object *const obj, Dictionary *const arg)
FUNC_ATTR_NONNULL_ALL
{
    if(obj->type != MSGPACK_OBJECT_MAP)
    {
        return false;
    }

    arg->size = obj->via.array.size;
    arg->items = xcalloc(obj->via.map.size, sizeof(key_value_pair_st));

    for(uint32_t i = 0; i < obj->via.map.size; i++)
    {
        if(!rpc_to_string(&obj->via.map.ptr[i].key, &arg->items[i].key))
        {
            return false;
        }

        if(!rpc_to_object(&obj->via.map.ptr[i].val, &arg->items[i].value))
        {
            return false;
        }
    }

    return true;
}

void rpc_from_boolean(Boolean result, msgpack_packer *res)
FUNC_ATTR_NONNULL_ARG(2)
{
    if(result)
    {
        msgpack_pack_true(res);
    }
    else
    {
        msgpack_pack_false(res);
    }
}

void rpc_from_integer(Integer result, msgpack_packer *res)
FUNC_ATTR_NONNULL_ARG(2)
{
    msgpack_pack_int64(res, result);
}

void rpc_from_float(Float result, msgpack_packer *res)
FUNC_ATTR_NONNULL_ARG(2)
{
    msgpack_pack_double(res, result);
}

void rpc_from_string(const String result, msgpack_packer *res)
FUNC_ATTR_NONNULL_ARG(2)
{
    msgpack_pack_str(res, result.size);

    if(result.size > 0)
    {
        msgpack_pack_str_body(res, result.data, result.size);
    }
}

typedef struct api2mpobj_stackitem_s
{
    const Object *aobj;
    bool container;
    size_t idx;
} api2mpobj_stackitem_st;

/// Convert type used by Neovim API to msgpack
///
/// @param[in]  result
/// Object to convert.
///
/// @param[out] res
/// Structure that defines where conversion results are saved.
///
/// @return true in case of success, false otherwise.
void rpc_from_object(const Object result, msgpack_packer *const res)
FUNC_ATTR_NONNULL_ARG(2)
{
    kvec_t(api2mpobj_stackitem_st) stack = KV_INITIAL_VALUE;

    kv_push(stack, ((api2mpobj_stackitem_st) {
        &result, false, 0
    }));

    while(kv_size(stack))
    {
        api2mpobj_stackitem_st cur = kv_last(stack);

        STATIC_ASSERT(kObjectTypeWindow == kObjectTypeBuffer + 1
                      && kObjectTypeTabpage == kObjectTypeWindow + 1,
                      "Buffer, window and tabpage enum items are in order");

        switch(cur.aobj->type)
        {
            case kObjectTypeNil:
            {
                msgpack_pack_nil(res);
                break;
            }

            case kObjectTypeBoolean:
            {
                rpc_from_boolean(cur.aobj->data.boolean, res);
                break;
            }

            case kObjectTypeInteger:
            {
                rpc_from_integer(cur.aobj->data.integer, res);
                break;
            }

            case kObjectTypeFloat:
            {
                rpc_from_float(cur.aobj->data.floating, res);
                break;
            }

            case kObjectTypeString:
            {
                rpc_from_string(cur.aobj->data.string, res);
                break;
            }

            case kObjectTypeBuffer:
            {
                rpc_from_buffer(cur.aobj->data.integer, res);
                break;
            }

            case kObjectTypeWindow:
            {
                rpc_from_window(cur.aobj->data.integer, res);
                break;
            }

            case kObjectTypeTabpage:
            {
                rpc_from_tabpage(cur.aobj->data.integer, res);
                break;
            }

            case kObjectTypeArray:
            {
                const size_t size = cur.aobj->data.array.size;

                if(cur.container)
                {
                    if(cur.idx >= size)
                    {
                        (void)kv_pop(stack);
                    }
                    else
                    {
                        const size_t idx = cur.idx;
                        cur.idx++;
                        kv_last(stack) = cur;

                        kv_push(stack, ((api2mpobj_stackitem_st) {
                            .aobj = &cur.aobj->data.array.items[idx],
                            .container = false,
                        }));
                    }
                }
                else
                {
                    msgpack_pack_array(res, size);
                    cur.container = true;
                    kv_last(stack) = cur;
                }

                break;
            }

            case kObjectTypeDictionary:
            {
                const size_t size = cur.aobj->data.dictionary.size;

                if(cur.container)
                {
                    if(cur.idx >= size)
                    {
                        (void)kv_pop(stack);
                    }
                    else
                    {
                        const size_t idx = cur.idx;
                        cur.idx++;

                        kv_last(stack) = cur;

                        rpc_from_string(cur.aobj->data.dictionary.items[idx].key,
                                        res);

                        kv_push(stack, ((api2mpobj_stackitem_st) {
                            .aobj = &cur.aobj->data.dictionary.items[idx].value,
                            .container = false,
                        }));
                    }
                }
                else
                {
                    msgpack_pack_map(res, size);
                    cur.container = true;
                    kv_last(stack) = cur;
                }

                break;
            }
        }

        if(!cur.container)
        {
            (void)kv_pop(stack);
        }
    }

    kv_destroy(stack);
}

void rpc_from_array(Array result, msgpack_packer *res)
FUNC_ATTR_NONNULL_ARG(2)
{
    msgpack_pack_array(res, result.size);

    for(size_t i = 0; i < result.size; i++)
    {
        rpc_from_object(result.items[i], res);
    }
}

void rpc_from_dictionary(Dictionary result, msgpack_packer *res)
FUNC_ATTR_NONNULL_ARG(2)
{
    msgpack_pack_map(res, result.size);

    for(size_t i = 0; i < result.size; i++)
    {
        rpc_from_string(result.items[i].key, res);
        rpc_from_object(result.items[i].value, res);
    }
}

/// Handler executed when an invalid method name is passed
Object rpc_handle_missing_method(uint64_t FUNC_ARGS_UNUSED_REALY(channel_id),
                                 Array FUNC_ARGS_UNUSED_REALY(args),
                                 error_st *error)
{
    api_set_error(error, kErrorTypeException, "Invalid method name");
    return NIL;
}

/// Handler executed when malformated arguments are passed
Object rpc_handle_invalid_arguments(uint64_t FUNC_ARGS_UNUSED_REALY(channel_id),
                                    Array FUNC_ARGS_UNUSED_REALY(args),
                                    error_st *error)
{
    api_set_error(error, kErrorTypeException, "Invalid method arguments");
    return NIL;
}

/// Serializes a msgpack-rpc request or notification(id == 0)
void rpc_serialize_request(uint64_t request_id,
                           const String method,
                           Array args,
                           msgpack_packer *pac)
FUNC_ATTR_NONNULL_ARG(4)
{
    msgpack_pack_array(pac, request_id ? 4 : 3);
    msgpack_pack_int(pac, request_id ? 0 : 2);

    if(request_id)
    {
        msgpack_pack_uint64(pac, request_id);
    }

    rpc_from_string(method, pac);
    rpc_from_array(args, pac);
}

/// Serializes a msgpack-rpc response
void rpc_serialize_response(uint64_t response_id,
                            error_st *err,
                            Object arg,
                            msgpack_packer *pac)
FUNC_ATTR_NONNULL_ARG(2, 4)
{
    msgpack_pack_array(pac, 4);
    msgpack_pack_int(pac, 1);
    msgpack_pack_uint64(pac, response_id);

    if(ERROR_SET(err))
    {
        // error represented by a [type, message] array
        msgpack_pack_array(pac, 2);
        rpc_from_integer(err->type, pac);
        rpc_from_string(cstr_as_string(err->msg), pac);
        msgpack_pack_nil(pac); // Nil result
    }
    else
    {
        msgpack_pack_nil(pac); // Nil error
        rpc_from_object(arg, pac); // Return value
    }
}

static bool rpc_is_notification(msgpack_object *req)
{
    return req->via.array.ptr[0].via.u64 == 2;
}

msgpack_object *rpc_method(msgpack_object *req)
{
    msgpack_object *obj =
        req->via.array.ptr + (rpc_is_notification(req) ? 1 : 2);

    return (obj->type == MSGPACK_OBJECT_STR
            || obj->type == MSGPACK_OBJECT_BIN) ? obj : NULL;
}

msgpack_object *rpc_args(msgpack_object *req)
{
    msgpack_object *obj =
        req->via.array.ptr + (rpc_is_notification(req) ? 2 : 3);

    return (obj->type == MSGPACK_OBJECT_ARRAY) ? obj : NULL;
}

static msgpack_object *rpc_msg_id(msgpack_object *req)
{
    if(rpc_is_notification(req))
    {
        return NULL;
    }

    msgpack_object *obj = &req->via.array.ptr[1];
    return (obj->type == MSGPACK_OBJECT_POSITIVE_INTEGER) ? obj : NULL;
}

void rpc_validate(uint64_t *response_id,
                  msgpack_object *req,
                  error_st *err)
{
    *response_id = NO_RESPONSE; // response id not known yet

    // Validate the basic structure of the msgpack-rpc payload
    if(req->type != MSGPACK_OBJECT_ARRAY)
    {
        api_set_error(err, kErrorTypeValidation,
                      "Message is not an array");
        return;
    }

    if(req->via.array.size == 0)
    {
        api_set_error(err, kErrorTypeValidation,
                      "Message is empty");
        return;
    }

    if(req->via.array.ptr[0].type != MSGPACK_OBJECT_POSITIVE_INTEGER)
    {
        api_set_error(err, kErrorTypeValidation,
                      "Message type must be an integer");
        return;
    }

    uint64_t type = req->via.array.ptr[0].via.u64;

    if(type != kMessageTypeRequest && type != kMessageTypeNotification)
    {
        api_set_error(err, kErrorTypeValidation,
                      "Unknown message type");
        return;
    }

    if((type == kMessageTypeRequest && req->via.array.size != 4)
       || (type == kMessageTypeNotification && req->via.array.size != 3))
    {
        api_set_error(err, kErrorTypeValidation,
                      "Request array size must be 4 (request) or 3 (notification)");
        return;
    }

    if(type == kMessageTypeRequest)
    {
        msgpack_object *id_obj = rpc_msg_id(req);

        if(!id_obj)
        {
            api_set_error(err, kErrorTypeValidation,
                          "ID must be a positive integer");
            return;
        }

        *response_id = id_obj->via.u64;
    }

    if(!rpc_method(req))
    {
        api_set_error(err, kErrorTypeValidation,
                      "Method must be a string");
        return;
    }

    if(!rpc_args(req))
    {
        api_set_error(err, kErrorTypeValidation,
                      "Parameters must be an array");
        return;
    }
}
