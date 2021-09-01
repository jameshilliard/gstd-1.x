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

GST_START_TEST (test_pipeline_sink_disconnect_wrong_pipeline_name)
{
  GstdStatus ret;

  gulong sink_callback_id = 0;

  ret =
      gstd_signal_sink_disconnect (manager, "pipeline_name", sink_name,
      sink_callback_id);

  assert_equals_int (GSTD_LIB_NOT_FOUND, ret);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_sink_disconnect_wrong_sink_name)
{
  GstdStatus ret;

  gulong sink_callback_id = 0;

  ret =
      gstd_signal_sink_disconnect (manager, pipeline_name, "sink_name",
      sink_callback_id);

  assert_equals_int (GSTD_LIB_NOT_FOUND, ret);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_sink_disconnect_null_pipeline_name)
{
  GstdStatus ret;

  gulong sink_callback_id = 0;

  ret =
      gstd_signal_sink_disconnect (manager, NULL, sink_name, sink_callback_id);

  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_sink_disconnect_null_sink_name)
{
  GstdStatus ret;

  gulong sink_callback_id = 0;

  ret =
      gstd_signal_sink_disconnect (manager, pipeline_name, NULL,
      sink_callback_id);

  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);
}

GST_END_TEST;

static Suite *
gstd_signal_sink_disconnect_suite (void)
{
  Suite *suite = suite_create ("gstd_signal_sink_disconnect");
  TCase *tc = tcase_create ("general");

  suite_add_tcase (suite, tc);

  tcase_add_checked_fixture (tc, setup, teardown);
  // tcase_add_test (tc, test_pipeline_sink_disdisconnect_successful); /* This is tested in test_libgstd_signal_sink_disconnect.c */
  tcase_add_test (tc, test_pipeline_sink_disconnect_wrong_pipeline_name);
  tcase_add_test (tc, test_pipeline_sink_disconnect_wrong_sink_name);
  tcase_add_test (tc, test_pipeline_sink_disconnect_null_pipeline_name);
  tcase_add_test (tc, test_pipeline_sink_disconnect_null_sink_name);

  return suite;
}

GST_CHECK_MAIN (gstd_signal_sink_disconnect);
