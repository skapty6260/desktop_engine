#pragma once

#include <dbus/dbus.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct BufferMgr {
    pthread_t tid;
    bool running;
    DBusConnection *conn;
} BufferMgr_t;

extern struct BufferMgr *g_buffer_mgr;

void create_buffermgr_thread();
void stop_buffermgr_thread();
void cleanup_buffermgr();