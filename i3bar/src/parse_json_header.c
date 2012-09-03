/*
 * vim:ts=4:sw=4:expandtab
 *
 * i3bar - an xcb-based status- and ws-bar for i3
 * © 2010-2012 Axel Wagner and contributors (see also: LICENSE)
 *
 * parse_json_header.c: Parse the JSON protocol header to determine
 *                      protocol version and features.
 *
 */
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <ev.h>
#include <stdbool.h>
#include <stdint.h>
#include <yajl/yajl_common.h>
#include <yajl/yajl_parse.h>
#include <yajl/yajl_version.h>

#include "common.h"

static enum {
    KEY_VERSION,
    NO_KEY
} current_key;

#if YAJL_MAJOR >= 2
static int header_integer(void *ctx, long long val) {
#else
static int header_integer(void *ctx, long val) {
#endif
    i3bar_child *child = ctx;

    switch (current_key) {
        case KEY_VERSION:
            child->version = val;
            break;
        default:
            break;
    }
    return 1;
}

#define CHECK_KEY(name) (stringlen == strlen(name) && \
                         STARTS_WITH((const char*)stringval, stringlen, name))

#if YAJL_MAJOR >= 2
static int header_map_key(void *ctx, const unsigned char *stringval, size_t stringlen) {
#else
static int header_map_key(void *ctx, const unsigned char *stringval, unsigned int stringlen) {
#endif
    if (CHECK_KEY("version")) {
        current_key = KEY_VERSION;
    }
    return 1;
}

static yajl_callbacks version_callbacks = {
    NULL, /* null */
    NULL, /* boolean */
    &header_integer,
    NULL, /* double */
    NULL, /* number */
    NULL, /* string */
    NULL, /* start_map */
    &header_map_key,
    NULL, /* end_map */
    NULL, /* start_array */
    NULL /* end_array */
};

static void child_init(i3bar_child *child) {
    child->version = 0;
}

/*
 * Parse the JSON protocol header to determine protocol version and features.
 * In case the buffer does not contain a valid header (invalid JSON, or no
 * version field found), the 'correct' field of the returned header is set to
 * false. The amount of bytes consumed by parsing the header is returned in
 * *consumed (if non-NULL).
 *
 */
void parse_json_header(i3bar_child *child, const unsigned char *buffer, int length, unsigned int *consumed) {
    child_init(child);

    current_key = NO_KEY;

#if YAJL_MAJOR >= 2
    yajl_handle handle = yajl_alloc(&version_callbacks, NULL, child);
    /* Allow trailing garbage. yajl 1 always behaves that way anyways, but for
     * yajl 2, we need to be explicit. */
    yajl_config(handle, yajl_allow_trailing_garbage, 1);
#else
    yajl_parser_config parse_conf = { 0, 0 };

    yajl_handle handle = yajl_alloc(&version_callbacks, &parse_conf, NULL, child);
#endif

    yajl_status state = yajl_parse(handle, buffer, length);
    if (state != yajl_status_ok) {
        child_init(child);
        if (consumed != NULL)
            *consumed = 0;
    } else {
        if (consumed != NULL)
            *consumed = yajl_get_bytes_consumed(handle);
    }

    yajl_free(handle);
}