#pragma once

#include <dbus/dbus.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef enum {
    FORMAT_XRGB8888 = 0,
    FORMAT_ARGB8888 = 1,
} RenderBufferFormat;

typedef struct Buffer {
    void *data;
    size_t size;
    size_t capacity;

    uint32_t stride;
    uint32_t height;
    uint32_t width;
    RenderBufferFormat format;

    int fd;
    bool mmaped;
    bool dirty;
} RenderBuffer_t;

typedef struct BufferMgr {
    pthread_t tid;
    bool running;
    DBusConnection *conn;

    RenderBuffer_t *buffers; // Buffer linked list
} BufferMgr_t;

extern struct BufferMgr *g_buffer_mgr;

void create_buffermgr_thread();
void stop_buffermgr_thread();
void cleanup_buffermgr();