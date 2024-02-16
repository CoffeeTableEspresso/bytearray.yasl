#include <yasl/yasl.h>
#include <yasl/yasl_aux.h>

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#define BYTEARRAY_PRE "bytearray"
#define BYTEARRAY_NAME BYTEARRAY_PRE

struct YASL_ByteArray {
    size_t len;
    size_t capacity;
    char *bytes;
};

struct YASL_ByteArray *bytearray_alloc() {
    struct YASL_ByteArray *ba = malloc(sizeof(struct YASL_ByteArray));
    ba->len = 0;
    ba->capacity = 0;
    ba->bytes = NULL;
    return ba;
}

void bytearray_free(struct YASL_State *S, struct YASL_ByteArray *ba) {
    (void) S;
    free(ba->bytes);
    free(ba);
}

static void YASL_pushbytearray(struct YASL_State *S, struct YASL_ByteArray *ba) {
    YASL_pushuserdata(S, ba, BYTEARRAY_NAME, (void(*)(struct YASL_State *, void *))bytearray_free);
    YASL_loadmt(S, BYTEARRAY_PRE);
    YASL_setmt(S);
}

static int YASL_bytearray_new(struct YASL_State *S) {
    if (YASL_isundef(S)) {
        struct YASL_ByteArray *ba = bytearray_alloc();
        YASL_pushbytearray(S, ba);

        return 1;
    }

    if (!YASL_isstr(S)) {
        YASL_print_err(S, "TypeError: expected string.");
        YASL_throw_err(S, YASL_TYPE_ERROR);
    }

    char *copy = YASL_peekcstr(S);
    YASL_len(S);
    yasl_int len = YASL_popint(S);

    struct YASL_ByteArray *ba = bytearray_alloc();
    ba->len = len;
    ba->capacity = len;
    ba->bytes = copy;

    YASL_pushbytearray(S, ba);

    return 1;
}

static struct YASL_ByteArray *YASLX_checknbytearray(struct YASL_State *S, const char *name, unsigned n) {
    return (struct YASL_ByteArray *)YASLX_checknuserdata(S, BYTEARRAY_NAME, name, n);
}

static int YASL_bytearray_tostr(struct YASL_State *S) {
    struct YASL_ByteArray *ba = YASLX_checknbytearray(S, "bytearray.tostr", 0);

    size_t buffer_size = strlen("bytearray(") + ba->len + strlen(")");
    char *buffer = malloc(buffer_size);
    size_t buffer_count = 0;

    strcpy(buffer, "bytearray(");
    buffer_count += strlen("bytearray(");
    for (size_t i = 0; i < ba->len; i++) {
        const unsigned char curr = (unsigned char)ba->bytes[i];
        if (isprint(curr)) {
            buffer[buffer_count++] = curr;
        } else {
            char tmp[3] = { '0', '0', '\0' };
            sprintf(tmp + (curr < 16), "%x", curr);
            buffer_size += 3;
            buffer = realloc(buffer, buffer_size);
            buffer[buffer_count++] = '\\';
            buffer[buffer_count++] = 'x';
            memcpy(buffer + buffer_count, tmp, 2);
            buffer_count += 2;
        }
    }

    buffer[buffer_count++] = ')';

    YASL_pushlstr(S, buffer, buffer_size);
    return 1;
}

static int YASL_bytearray_tolist(struct YASL_State *S) {
    struct YASL_ByteArray *ba = YASLX_checknbytearray(S, "bytearray.tolist", 0);

    YASL_pushlist(S);

    for (size_t i = 0; i < ba->len; i++) {
        YASL_pushint(S, (unsigned char)ba->bytes[i]);
        YASL_listpush(S);
    }

    return 1;
}

static int YASL_bytearray___len(struct YASL_State *S) {
    struct YASL_ByteArray *ba = YASLX_checknbytearray(S, "bytearray.__len", 0);

    YASL_pushint(S, ba->len);
    return 1;
}

static int YASL_bytearray___add(struct YASL_State *S) {
    struct YASL_ByteArray *left = YASLX_checknbytearray(S, "bytearray.__add", 0);
    struct YASL_ByteArray *right = YASLX_checknbytearray(S, "bytearray.__add", 1);

    char *buffer = malloc(left->len + right->len);
    memcpy(buffer, left->bytes, left->len);
    memcpy(buffer + left->len, right->bytes, right->len);

    struct YASL_ByteArray *ba = bytearray_alloc();
    ba->len = left->len + right->len;
    ba->capacity = ba->len;
    ba->bytes = buffer;

    YASL_pushbytearray(S, ba);

    return 1;
}

#define DEF_GET_FUNCTION(n, type, prefix) \
static int YASL_bytearray_get##prefix##n(struct YASL_State *S) { \
    struct YASL_ByteArray *ba = YASLX_checknbytearray(S, "bytearray.get" #prefix #n, 0); \
    yasl_int index = YASLX_checknint(S, "bytearray.get" #prefix #n, 1); \
\
    if (index + n / 8 > ba->len) { \
        YASL_print_err(S, "ValueError: invalid index %d.\n", (int)index); \
        YASL_throw_err(S, YASL_VALUE_ERROR); \
    } \
\
    YASL_pushint(S, *(type##int##n##_t*)(ba->bytes + index)); \
    return 1; \
}

DEF_GET_FUNCTION(8, , i)
DEF_GET_FUNCTION(16, , i)
DEF_GET_FUNCTION(32, , i)
DEF_GET_FUNCTION(64, , i)

DEF_GET_FUNCTION(8, u, u)
DEF_GET_FUNCTION(16, u, u)
DEF_GET_FUNCTION(32, u, u)
DEF_GET_FUNCTION(64, u, u)

static int YASL_bytearray_getchars(struct YASL_State *S) {
    struct YASL_ByteArray *ba = YASLX_checknbytearray(S, "bytearray.getchars", 0);
    yasl_int index = YASLX_checknint(S, "bytearray.getchars", 1);
    yasl_int len = YASLX_checknint(S, "bytearray.getchars", 2);

    if (index + len > ba->len) {
        YASL_print_err(S, "ValueError: invalid index %d.\n", (int)index);
        YASL_throw_err(S, YASL_VALUE_ERROR);
    }

    YASL_pushlstr(S, ba->bytes + index, len);
    return 1;
}

int YASL_load_dyn_lib(struct YASL_State *S) {
    YASL_pushtable(S);
    YASL_registermt(S, BYTEARRAY_PRE);

    struct YASLX_function functions[] = {
        { "tostr", YASL_bytearray_tostr, 1 },
        { "__len", YASL_bytearray___len, 1 },
        { "__add", YASL_bytearray___add, 2 },
        { "tolist", YASL_bytearray_tolist, 1 },
        { "geti8", YASL_bytearray_geti8, 2 },
        { "geti16", YASL_bytearray_geti16, 2 },
        { "geti32", YASL_bytearray_geti32, 2 },
        { "geti64", YASL_bytearray_geti64, 2 },
        { "getu8", YASL_bytearray_getu8, 2 },
        { "getu16", YASL_bytearray_getu16, 2 },
        { "getu32", YASL_bytearray_getu32, 2 },
        { "getu64", YASL_bytearray_getu64, 2 },
        { "getchars", YASL_bytearray_getchars, 3 },
        { NULL, NULL, 0 }
    };

    YASL_loadmt(S, BYTEARRAY_PRE);
    YASLX_tablesetfunctions(S, functions);

    YASL_pushcfunction(S, YASL_bytearray_new, 1);

    return 1;
}

