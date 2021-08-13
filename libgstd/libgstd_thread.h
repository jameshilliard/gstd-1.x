/*
 * GStreamer Daemon - gst-launch on steroids
 * C client library abstracting gstd interprocess communication
 *
 * Copyright (c) 2015-2018 RidgeRun, LLC (http://www.ridgerun.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __LIBGSTD_THREAD_H__
#define __LIBGSTD_THREAD_H__

#include <pthread.h>

#include "libgstd.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _GstdThread GstdThread;
typedef struct _GstdCond GstdCond;
typedef struct _GstdMutex GstdMutex;

struct _GstdThread
{
  pthread_t thread;
};

struct _GstdCond
{
  pthread_cond_t cond;
};

struct _GstdMutex
{
  pthread_mutex_t mutex;
};

typedef void *(*GstdThreadFunction) (void *);
  
GstdStatus
gstd_thread_new (GstdThread *thread, GstdThreadFunction func, void * user_data);

void
gstd_mutex_init (GstdMutex *mutex);

void
gstd_mutex_lock (GstdMutex *mutex);

void
gstd_mutex_unlock (GstdMutex *mutex);

void
gstd_cond_init (GstdCond *mutex);

void
gstd_cond_wait (GstdCond *cond, GstdMutex *mutex);

void
gstd_cond_signal (GstdCond *cond);

#ifdef __cplusplus
}
#endif

#endif // __LIBGSTD_THREAD_H__
