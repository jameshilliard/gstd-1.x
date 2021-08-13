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

#include <stdlib.h>

#include "libgstd_assert.h"
#include "libgstd_thread.h"

GstdStatus
gstd_thread_new (GstdThread * self, GstdThreadFunction func, void *user_data)
{
  int ret;
  pthread_attr_t attr;

  gstd_assert_and_ret_val (NULL != func, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != self, GSTD_LIB_NULL_ARGUMENT);

  ret = pthread_attr_init (&attr);
  if (ret) {
    return GSTD_LIB_THREAD_ERROR;
  }

  ret = pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
  if (ret) {
    return GSTD_LIB_THREAD_ERROR;
  }

  ret = pthread_create (&(self->thread), &attr, func, user_data);
  if (ret) {
    return GSTD_LIB_THREAD_ERROR;
  }

  return GSTD_LIB_OK;
}

void
gstd_mutex_init (GstdMutex * mutex)
{
  gstd_assert_and_ret (NULL != mutex);

  pthread_mutex_init (&(mutex->mutex), NULL);
}

void
gstd_mutex_lock (GstdMutex * mutex)
{
  gstd_assert_and_ret (NULL != mutex);

  pthread_mutex_lock (&(mutex->mutex));
}

void
gstd_mutex_unlock (GstdMutex * mutex)
{
  gstd_assert_and_ret (NULL != mutex);

  pthread_mutex_unlock (&(mutex->mutex));
}

void
gstd_cond_init (GstdCond * cond)
{
  gstd_assert_and_ret (NULL != cond);

  pthread_cond_init (&(cond->cond), NULL);
}

void
gstd_cond_wait (GstdCond * cond, GstdMutex * mutex)
{
  gstd_assert_and_ret (NULL != cond);
  gstd_assert_and_ret (NULL != mutex);

  pthread_cond_wait (&(cond->cond), &(mutex->mutex));
}

void
gstd_cond_signal (GstdCond * cond)
{
  gstd_assert_and_ret (NULL != cond);

  pthread_cond_signal (&(cond->cond));
}
