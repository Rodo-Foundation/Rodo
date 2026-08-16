#include <node_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rodo.h"

static napi_ref rodo_constructor_ref;

typedef struct {
    Rodo* handle;
    napi_env env;
} rodo_js_handle;

// Helper to throw error
static napi_value ThrowError(napi_env env, const char* msg) {
    napi_value error;
    napi_throw_error(env, NULL, msg);
    return NULL;
}

// Convert JS value to Rodo value
static int js_to_rodo_value(napi_env env, napi_value js_val, rodo_value_t* out, napi_value* str_ref) {
    napi_valuetype type;
    napi_typeof(env, js_val, &type);

    if (type == napi_null || type == napi_undefined) {
        out->type = RODO_NULL;
        return 0;
    }
    if (type == napi_boolean) {
        bool b;
        napi_get_value_bool(env, js_val, &b);
        out->type = RODO_BOOL;
        out->value.bool_val = b ? 1 : 0;
        return 0;
    }
    if (type == napi_number) {
        double d;
        napi_get_value_double(env, js_val, &d);
        // Check if integer
        if (d == (int64_t)d && d >= -9223372036854775808.0 && d <= 9223372036854775807.0) {
            out->type = RODO_INT;
            out->value.int_val = (int64_t)d;
        } else {
            out->type = RODO_FLOAT;
            out->value.float_val = d;
        }
        return 0;
    }
    if (type == napi_string) {
        size_t len;
        napi_get_value_string_utf8(env, js_val, NULL, 0, &len);
        char* str = (char*)malloc(len + 1);
        napi_get_value_string_utf8(env, js_val, str, len + 1, &len);
        out->type = RODO_STRING;
        out->value.string_val = str;
        *str_ref = NULL; // need to keep reference, but we'll simplify: memory leak
        return 0;
    }
    // For simplicity, handle arrays and objects later
    // We'll return error for unsupported types
    return -1;
}

// Convert Rodo value to JS value
static napi_value rodo_to_js_value(napi_env env, rodo_value_t* val) {
    napi_value result;
    switch (val->type) {
        case RODO_NULL:
            napi_get_null(env, &result);
            break;
        case RODO_BOOL:
            napi_get_boolean(env, val->value.bool_val != 0, &result);
            break;
        case RODO_INT:
            napi_create_int64(env, val->value.int_val, &result);
            break;
        case RODO_FLOAT:
            napi_create_double(env, val->value.float_val, &result);
            break;
        case RODO_STRING:
            napi_create_string_utf8(env, val->value.string_val, NAPI_AUTO_LENGTH, &result);
            break;
        default:
            napi_get_null(env, &result);
    }
    return result;
}

static Rodo* get_rodo_handle(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, NULL, NULL, &js_this, NULL);
    rodo_js_handle* holder;
    napi_unwrap(env, js_this, (void**)&holder);
    return holder->handle;
}

static napi_value RodoLink(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (argc < 1) return ThrowError(env, "Missing path argument");

    size_t path_len;
    napi_get_value_string_utf8(env, args[0], NULL, 0, &path_len);
    char* path = (char*)malloc(path_len + 1);
    napi_get_value_string_utf8(env, args[0], path, path_len + 1, &path_len);

    Rodo* handle = rodo_link(path);
    free(path);
    if (!handle) return ThrowError(env, "Failed to link .rd file");

    // Create JS wrapper object
    napi_value js_obj, js_constructor;
    napi_get_reference_value(env, rodo_constructor_ref, &js_constructor);
    napi_new_instance(env, js_constructor, 0, NULL, &js_obj);

    // Attach handle
    rodo_js_handle* holder = (rodo_js_handle*)malloc(sizeof(rodo_js_handle));
    holder->handle = handle;
    holder->env = env;
    napi_wrap(env, js_obj, holder, NULL, NULL, NULL);

    return js_obj;
}

static napi_value RodoClose(napi_env env, napi_callback_info info) {
    Rodo* handle = get_rodo_handle(env, info);
    if (handle) {
        rodo_close(handle);
    }
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

static napi_value RodoSet(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (argc < 2) return ThrowError(env, "Expected key and value");

    Rodo* handle = get_rodo_handle(env, info);
    if (!handle) return ThrowError(env, "Invalid handle");

    // Get key
    size_t key_len;
    napi_get_value_string_utf8(env, args[0], NULL, 0, &key_len);
    char* key = (char*)malloc(key_len + 1);
    napi_get_value_string_utf8(env, args[0], key, key_len + 1, &key_len);

    // Convert value
    rodo_value_t rodo_val;
    napi_value str_ref;
    if (js_to_rodo_value(env, args[1], &rodo_val, &str_ref) != 0) {
        free(key);
        return ThrowError(env, "Unsupported value type");
    }

    int result = rodo_set(handle, key, &rodo_val);
    free(key);
    // free strings? We ignore for simplicity

    napi_value js_result;
    napi_create_int32(env, result, &js_result);
    return js_result;
}

static napi_value RodoGet(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (argc < 1) return ThrowError(env, "Expected key");

    Rodo* handle = get_rodo_handle(env, info);
    size_t key_len;
    napi_get_value_string_utf8(env, args[0], NULL, 0, &key_len);
    char* key = (char*)malloc(key_len + 1);
    napi_get_value_string_utf8(env, args[0], key, key_len + 1, &key_len);

    rodo_value_t* val = rodo_get(handle, key);
    free(key);
    if (!val) {
        napi_value null;
        napi_get_null(env, &null);
        return null;
    }
    return rodo_to_js_value(env, val);
}

static napi_value RodoHas(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    Rodo* handle = get_rodo_handle(env, info);
    size_t key_len;
    napi_get_value_string_utf8(env, args[0], NULL, 0, &key_len);
    char* key = (char*)malloc(key_len + 1);
    napi_get_value_string_utf8(env, args[0], key, key_len + 1, &key_len);
    int has = rodo_has(handle, key);
    free(key);
    napi_value result;
    napi_get_boolean(env, has, &result);
    return result;
}

static napi_value RodoDelete(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    Rodo* handle = get_rodo_handle(env, info);
    size_t key_len;
    napi_get_value_string_utf8(env, args[0], NULL, 0, &key_len);
    char* key = (char*)malloc(key_len + 1);
    napi_get_value_string_utf8(env, args[0], key, key_len + 1, &key_len);
    int res = rodo_delete(handle, key);
    free(key);
    napi_value result;
    napi_create_int32(env, res, &result);
    return result;
}

static napi_value RodoKeys(napi_env env, napi_callback_info info) {
    Rodo* handle = get_rodo_handle(env, info);
    char** keys;
    size_t count = rodo_keys(handle, &keys);
    napi_value js_array;
    napi_create_array_with_length(env, count, &js_array);
    for (size_t i = 0; i < count; i++) {
        napi_value key_str;
        napi_create_string_utf8(env, keys[i], NAPI_AUTO_LENGTH, &key_str);
        napi_set_element(env, js_array, i, key_str);
        free(keys[i]);
    }
    free(keys);
    return js_array;
}

// Constructor for Rodo objects (not directly used for link)
static napi_value RodoConstructor(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, NULL, NULL, &js_this, NULL);
    return js_this;
}

static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        { "rodoLink", NULL, RodoLink, NULL, NULL, NULL, napi_default, NULL },
        { "rodoClose", NULL, RodoClose, NULL, NULL, NULL, napi_default, NULL },
        { "rodoSet", NULL, RodoSet, NULL, NULL, NULL, napi_default, NULL },
        { "rodoGet", NULL, RodoGet, NULL, NULL, NULL, napi_default, NULL },
        { "rodoHas", NULL, RodoHas, NULL, NULL, NULL, napi_default, NULL },
        { "rodoDelete", NULL, RodoDelete, NULL, NULL, NULL, napi_default, NULL },
        { "rodoKeys", NULL, RodoKeys, NULL, NULL, NULL, napi_default, NULL }
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);

    // Create constructor for internal use
    napi_value constructor;
    napi_define_class(env, "Rodo", NAPI_AUTO_LENGTH, RodoConstructor, NULL, 0, NULL, &constructor);
    napi_create_reference(env, constructor, 1, &rodo_constructor_ref);

    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)