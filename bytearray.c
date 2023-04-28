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
        const char curr = ba->bytes[i];
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

    strcpy(buffer + buffer_count, ")");

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

#define DEF_GET_FUNCTION(n) \
static int YASL_bytearray_getint##n(struct YASL_State *S) { \
    struct YASL_ByteArray *ba = YASLX_checknbytearray(S, "bytearray.getint" #n, 0); \
    yasl_int index = YASLX_checknint(S, "bytearray.getint" #n, 1); \
\
    if (index + n / 8 > ba->len) { \
        YASL_print_err(S, "ValueError: invalid index %d.\n", (int)index); \
        YASL_throw_err(S, YASL_VALUE_ERROR); \
    } \
\
    YASL_pushint(S, *(int##n##_t*)(ba->bytes + index)); \
    return 1; \
}

DEF_GET_FUNCTION(8)
DEF_GET_FUNCTION(16)
DEF_GET_FUNCTION(32)
DEF_GET_FUNCTION(64)

int YASL_load_dyn_lib(struct YASL_State *S) {
    YASL_pushtable(S);
    YASL_registermt(S, BYTEARRAY_PRE);

    YASL_loadmt(S, BYTEARRAY_PRE);

    YASL_pushlit(S, "tostr");
    YASL_pushcfunction(S, YASL_bytearray_tostr, 1);
    YASL_tableset(S);

    YASL_pushlit(S, "__len");
    YASL_pushcfunction(S, YASL_bytearray___len, 1);
    YASL_tableset(S);

    YASL_pushlit(S, "__add");
    YASL_pushcfunction(S, YASL_bytearray___add, 2);
    YASL_tableset(S);

    YASL_pushlit(S, "tolist");
    YASL_pushcfunction(S, YASL_bytearray_tolist, 1);
    YASL_tableset(S);

    YASL_pushlit(S, "getint8");
    YASL_pushcfunction(S, YASL_bytearray_getint8, 2);
    YASL_tableset(S);

    YASL_pushlit(S, "getint16");
    YASL_pushcfunction(S, YASL_bytearray_getint16, 2);
    YASL_tableset(S);

    YASL_pushlit(S, "getint32");
    YASL_pushcfunction(S, YASL_bytearray_getint32, 2);
    YASL_tableset(S);

    YASL_pushlit(S, "getint64");
    YASL_pushcfunction(S, YASL_bytearray_getint64, 2);
    YASL_tableset(S);

    YASL_pushcfunction(S, YASL_bytearray_new, 1);

    return 1;
}

