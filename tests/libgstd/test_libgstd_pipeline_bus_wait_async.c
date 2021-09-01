/*
 * GStreamer Daemon - Gst Launch under steroids
 * Copyright (c) 2015-2021 Ridgerun, LLC (http://www.ridgerun.com)
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <gst/check/gstcheck.h>
#include <pthread.h>
#include <time.h>

#include "libgstd.h"
#include "libgstd_assert.h"
#include "libgstd_json.h"
#include "libgstd_parser.h"

#define BUS_FILTER "bus_filter"
#define BUS_TIMEOUT "bus_timeout"
#define BUS_READ "bus_read"

/* Test Fixture */
static GstDManager *manager;
static gchar *bus_filter, *bus_timeout, *bus_read;
pthread_mutex_t lock;
int send_wait_time = 0;

static void
setup (void)
{
  gstd_manager_new (NULL, 0, &manager, NULL, 0, NULL);
}

static void
teardown (void)
{
  gstd_manager_free (manager);
  g_free (bus_filter);
  g_free (bus_timeout);
  g_free (bus_read);
}

GstdStatus
gstd_json_get_child_char_array (const char *json,
    const char *array_name, const char *element_name, char **out[],
    int *array_lenght)
{
  gstd_assert_and_ret_val (NULL != json, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != array_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != element_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != out, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != array_lenght, GSTD_LIB_NULL_ARGUMENT);

  return GSTD_LIB_OK;
}

GstdStatus
gstd_json_child_string (const char *json, const char *data_name, char **out)
{
  gstd_assert_and_ret_val (NULL != json, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != data_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != out, GSTD_LIB_NULL_ARGUMENT);

  return GSTD_LIB_OK;
}

GstdStatus
gstd_parser (GstdSession * session, const gchar * cmd, gchar ** response)
{
  gchar **tokens;
  gchar *action;

  gstd_assert_and_ret_val (NULL != session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != cmd, GSTD_LIB_NULL_ARGUMENT);

  tokens = g_strsplit (cmd, " ", 2);
  action = tokens[0];

  if (!g_ascii_strcasecmp (BUS_FILTER, action)) {
    bus_filter = g_strdup_printf ("%s", cmd);
  } else if (!g_ascii_strcasecmp (BUS_TIMEOUT, action)) {
    bus_timeout = g_strdup_printf ("%s", cmd);
  } else if (!g_ascii_strcasecmp (BUS_READ, action)) {
    if (send_wait_time > 0) {
      sleep (send_wait_time);
    }
    bus_read = g_strdup_printf ("%s", cmd);
  }

  return GSTD_LIB_OK;
}

static GstdStatus
callback (GstDManager * manager, const gchar * pipeline_name,
    const gchar * message_name, const long long timeout, char *message,
    gpointer user_data)
{
  /* Unlock the mutex */
  pthread_mutex_unlock (&lock);
  return GSTD_LIB_OK;
}

GST_START_TEST (test_pipeline_bus_wait_async_success)
{
  GstdStatus ret;
  const gchar *pipeline_name = "pipe";
  const gchar *message_name = "eos";
  const gint64 timeout = -1;

  const gchar *bus_filter_expected = "bus_filter pipe eos";
  const gchar *bus_timeout_expected = "bus_timeout pipe -1";
  const gchar *bus_read_expected = "bus_read pipe";

  /* Mutex timeout 10ms */
  struct timespec ts;
  clock_gettime (CLOCK_REALTIME, &ts);
  ts.tv_nsec += 10 * 1000000;

  if (pthread_mutex_init (&lock, NULL) != 0) {
    /* Exit if unable to create mutex lock */
    ASSERT_CRITICAL ();
  }

  /* Set the send command to wait for 0s before responding */
  send_wait_time = 0;

  /*
   * Lock the mutex, this should be unlocked by the callback function
   */
  pthread_mutex_lock (&lock);

  ret =
      gstd_pipeline_bus_wait_async (manager, pipeline_name, message_name,
      timeout, callback, NULL);
  assert_equals_int (GSTD_LIB_OK, ret);

  assert_equals_string (bus_filter_expected, bus_filter);
  assert_equals_string (bus_timeout_expected, bus_timeout);

  /* Wait for the callback function to finish or timeout passes */
  pthread_mutex_timedlock (&lock, &ts);
  assert_equals_string (bus_read_expected, bus_read);
  pthread_mutex_unlock (&lock);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_bus_wait_async_bus_no_response)
{
  GstdStatus ret;
  const gchar *pipeline_name = "pipe";
  const gchar *message_name = "eos";
  const gint64 timeout = -1;

  const gchar *bus_filter_expected = "bus_filter pipe eos";
  const gchar *bus_timeout_expected = "bus_timeout pipe -1";
  const gchar *bus_read_expected = NULL;

  /* Mutex timeout 10ms */
  struct timespec ts;
  clock_gettime (CLOCK_REALTIME, &ts);
  ts.tv_nsec += 10 * 1000000;

  if (pthread_mutex_init (&lock, NULL) != 0) {
    /* Exit if unable to create mutex lock */
    ASSERT_CRITICAL ();
  }

  /* Set the send command to wait for 0s before responding */
  send_wait_time = 2;

  /*
   * Lock the mutex, this should be unlocked by the callback function
   */
  pthread_mutex_lock (&lock);

  ret =
      gstd_pipeline_bus_wait_async (manager, pipeline_name, message_name,
      timeout, callback, NULL);
  assert_equals_int (GSTD_LIB_OK, ret);

  assert_equals_string (bus_filter_expected, bus_filter);
  assert_equals_string (bus_timeout_expected, bus_timeout);

  /* Wait for the callback function to finish or timeout passes */
  pthread_mutex_timedlock (&lock, &ts);
  assert_equals_string (bus_read_expected, bus_read);
  pthread_mutex_unlock (&lock);
}

GST_END_TEST;


static Suite *
gstd_pipeline_bus_wait_async_suite (void)
{
  Suite *suite = suite_create ("gstd_pipeline_bus_wait_async");
  TCase *tc = tcase_create ("general");

  suite_add_tcase (suite, tc);

  tcase_add_checked_fixture (tc, setup, teardown);
  tcase_add_test (tc, test_pipeline_bus_wait_async_success);
  tcase_add_test (tc, test_pipeline_bus_wait_async_bus_no_response);

  return suite;
}

GST_CHECK_MAIN (gstd_pipeline_bus_wait_async);
