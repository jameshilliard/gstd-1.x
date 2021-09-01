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

#include "libgstd.h"
#include "libgstd_assert.h"
#include "libgstd_json.h"
#include "libgstd_parser.h"

static GstDManager *manager;
const gchar *pipeline_name = "mypipe";
const gchar *sink_name = "mypipe";

pthread_mutex_t lock;
int is_buffer = 0;

static void
setup (void)
{
  const gchar *pipeline_desc =
      g_strdup_printf
      ("videotestsrc name=vts ! jpegenc ! appsink name=%s emit-signals=true async=false drop=true",
      sink_name);

  gstd_manager_new (NULL, 0, &manager, NULL, 0, NULL);
  gstd_pipeline_create (manager, pipeline_name, pipeline_desc);
}

static void
teardown (void)
{
  gstd_pipeline_delete (manager, pipeline_name);
  gstd_manager_free (manager);
}

static GstdStatus
sink_callback (GstElement * source, void *user_data)
{
  pthread_mutex_lock (&lock);
  is_buffer = 1;
  pthread_mutex_unlock (&lock);

  return GSTD_LIB_OK;
}


GST_START_TEST (test_pipeline_sink_connect_successful)
{
  GstdStatus ret;
  gulong sink_callback_id = 0;

  if (pthread_mutex_init (&lock, NULL) != 0) {
    /* Exit if unable to create mutex lock */
    ASSERT_CRITICAL ();
  }

  ret =
      gstd_signal_sink_connect (manager, pipeline_name, sink_name,
      &sink_callback_id, sink_callback, NULL);

  assert_equals_int (GSTD_LIB_OK, ret);
  fail_if (0 >= sink_callback_id);

  gstd_pipeline_play (manager, pipeline_name);

  while (TRUE) {
    pthread_mutex_lock (&lock);
    if (is_buffer) {
      ret =
          gstd_signal_sink_disconnect (manager, pipeline_name, sink_name,
          sink_callback_id);
      assert_equals_int (GSTD_LIB_OK, ret);
      gstd_pipeline_stop (manager, pipeline_name);
      break;
    }
    pthread_mutex_unlock (&lock);
  }

}

GST_END_TEST;

GST_START_TEST (test_pipeline_sink_connect_wrong_pipeline_name)
{
  GstdStatus ret;

  gulong sink_callback_id = 0;

  ret =
      gstd_signal_sink_connect (manager, "pipeline_name", sink_name,
      &sink_callback_id, sink_callback, NULL);

  assert_equals_int (GSTD_LIB_NOT_FOUND, ret);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_sink_connect_wrong_sink_name)
{
  GstdStatus ret;

  gulong sink_callback_id = 0;

  ret =
      gstd_signal_sink_connect (manager, pipeline_name, "sink_name",
      &sink_callback_id, sink_callback, NULL);

  assert_equals_int (GSTD_LIB_NOT_FOUND, ret);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_sink_connect_null_pipeline_name)
{
  GstdStatus ret;

  gulong sink_callback_id = 0;

  ret =
      gstd_signal_sink_connect (manager, NULL, sink_name, &sink_callback_id,
      sink_callback, NULL);

  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_sink_connect_null_sink_name)
{
  GstdStatus ret;

  gulong sink_callback_id = 0;

  ret =
      gstd_signal_sink_connect (manager, pipeline_name, NULL, &sink_callback_id,
      sink_callback, NULL);

  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_sink_connect_null_sink_id)
{
  GstdStatus ret;

  ret =
      gstd_signal_sink_connect (manager, pipeline_name, sink_name, NULL,
      sink_callback, NULL);

  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);
}

GST_END_TEST;


static Suite *
gstd_signal_sink_connect_suite (void)
{
  Suite *suite = suite_create ("gstd_signal_sink_connect");
  TCase *tc = tcase_create ("general");

  suite_add_tcase (suite, tc);

  tcase_add_checked_fixture (tc, setup, teardown);
  tcase_add_test (tc, test_pipeline_sink_connect_successful);
  tcase_add_test (tc, test_pipeline_sink_connect_wrong_pipeline_name);
  tcase_add_test (tc, test_pipeline_sink_connect_wrong_sink_name);
  tcase_add_test (tc, test_pipeline_sink_connect_null_pipeline_name);
  tcase_add_test (tc, test_pipeline_sink_connect_null_sink_name);
  tcase_add_test (tc, test_pipeline_sink_connect_null_sink_id);

  return suite;
}

GST_CHECK_MAIN (gstd_signal_sink_connect);
