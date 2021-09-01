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
static gchar *parser_response;

static void
setup (void)
{
  gstd_manager_new (NULL, 0, &manager, NULL, 0, NULL);
}

static void
teardown (void)
{
  gstd_manager_free (manager);
  g_free (parser_response);
}


GstdStatus
gstd_parser (GstdSession * session, const gchar * cmd, gchar ** response)
{
  gstd_assert_and_ret_val (NULL != session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != cmd, GSTD_LIB_NULL_ARGUMENT);

  parser_response = g_strdup_printf ("%s", cmd);

  return GSTD_LIB_OK;
}

GST_START_TEST (test_pipeline_seek_success)
{
  GstdStatus ret;

  const gchar *pipe_name = "pipe";
  const double rate = 1.0;
  const int format = 3;
  const int flags = 1;
  const int start_type = 1;
  const long long start = 0;
  const int end_type = 1;
  const long long end = 9999;
  const gchar *expected = "event_seek pipe 1.000000 3 1 1 0 1 9999";

  ret =
      gstd_pipeline_seek (manager, pipe_name, rate, format, flags, start_type,
      start, end_type, end);

  fail_if (GSTD_LIB_OK != ret);
  assert_equals_string (expected, parser_response);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_seek_null_pipeline_name)
{
  GstdStatus ret;

  const double rate = 1.0;
  const int format = 3;
  const int flags = 1;
  const int start_type = 1;
  const long long start = 0;
  const int end_type = 1;
  const long long end = 9999;

  ret =
      gstd_pipeline_seek (manager, NULL, rate, format, flags, start_type,
      start, end_type, end);
  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_seek_null_manager)
{
  GstdStatus ret;

  const gchar *pipe_name = "pipe";
  const double rate = 1.0;
  const int format = 3;
  const int flags = 1;
  const int start_type = 1;
  const long long start = 0;
  const int end_type = 1;
  const long long end = 9999;

  ret =
      gstd_pipeline_seek (NULL, pipe_name, rate, format, flags, start_type,
      start, end_type, end);
  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);
}

GST_END_TEST;


static Suite *
gstd_pipeline_seek_suite (void)
{
  Suite *suite = suite_create ("gstd_pipeline_seek");
  TCase *tc = tcase_create ("general");

  suite_add_tcase (suite, tc);

  tcase_add_checked_fixture (tc, setup, teardown);
  tcase_add_test (tc, test_pipeline_seek_success);
  tcase_add_test (tc, test_pipeline_seek_null_pipeline_name);
  tcase_add_test (tc, test_pipeline_seek_null_manager);

  return suite;
}

GST_CHECK_MAIN (gstd_pipeline_seek);
