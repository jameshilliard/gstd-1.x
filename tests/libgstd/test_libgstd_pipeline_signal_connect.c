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

#define SIGNAL_TIMEOUT "signal_timeout"
#define SIGNAL_CONNECT "signal_connect"

static GstDManager *manager;
static gchar *parser_signal_timeout;
static gchar *parser_signal_connect;

static void
setup (void)
{
  gstd_manager_new (NULL, 0, &manager, NULL, 0, NULL);
}

static void
teardown (void)
{
  gstd_manager_free (manager);
  g_free (parser_signal_timeout);
  g_free (parser_signal_connect);
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

  if (!g_ascii_strcasecmp (SIGNAL_TIMEOUT, action)) {
    parser_signal_timeout = g_strdup_printf ("%s", cmd);
  } else if (!g_ascii_strcasecmp (SIGNAL_CONNECT, action)) {
    parser_signal_connect = g_strdup_printf ("%s", cmd);
    *response = g_strdup_printf ("%s", cmd);
  }

  return GSTD_LIB_OK;
}

GST_START_TEST (test_pipeline_signal_connect_success)
{
  GstdStatus ret;

  const gchar *pipeline_name = "pipe";
  const gchar *element_name = "element";
  const gchar *signal_name = "signal";
  gint signal_timeout = 100;
  gchar *response = NULL;

  const gchar *expected_signal_timeout =
      "signal_timeout pipe element signal 100";
  const gchar *expected_signal_connect = "signal_connect pipe element signal";

  ret = gstd_pipeline_signal_connect (manager, pipeline_name, element_name,
      signal_name, signal_timeout, &response);

  fail_if (GSTD_LIB_OK != ret);
  assert_equals_string (expected_signal_timeout, parser_signal_timeout);
  assert_equals_string (expected_signal_connect, parser_signal_connect);

  g_free (response);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_signal_connect_null_pipeline_name)
{
  GstdStatus ret;

  const gchar *element_name = "element";
  const gchar *signal_name = "signal";
  gint signal_timeout = 100;
  gchar *response = NULL;

  ret = gstd_pipeline_signal_connect (manager, NULL, element_name,
      signal_name, signal_timeout, &response);

  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);
  fail_if (NULL != response);

  g_free (response);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_signal_connect_null_element_name)
{
  GstdStatus ret;

  const gchar *pipeline_name = "pipe";
  const gchar *signal_name = "signal";
  gint signal_timeout = 100;
  gchar *response = NULL;

  ret = gstd_pipeline_signal_connect (manager, pipeline_name, NULL,
      signal_name, signal_timeout, &response);

  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);
  fail_if (NULL != response);

  g_free (response);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_signal_connect_null_signal_name)
{
  GstdStatus ret;

  const gchar *pipeline_name = "pipe";
  const gchar *element_name = "element";
  gint signal_timeout = 100;
  gchar *response = NULL;

  ret = gstd_pipeline_signal_connect (manager, pipeline_name, element_name,
      NULL, signal_timeout, &response);

  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);
  fail_if (NULL != response);

  g_free (response);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_signal_connect_null_manager)
{
  GstdStatus ret;

  const gchar *pipeline_name = "pipe";
  const gchar *element_name = "element";
  const gchar *signal_name = "signal";
  gint signal_timeout = 100;
  gchar *response = NULL;

  ret = gstd_pipeline_signal_connect (NULL, pipeline_name, element_name,
      signal_name, signal_timeout, &response);

  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);
  fail_if (NULL != response);

  g_free (response);
}

GST_END_TEST;


static Suite *
gstd_pipeline_signal_connect_suite (void)
{
  Suite *suite = suite_create ("gstd_pipeline_signal_connect");
  TCase *tc = tcase_create ("general");

  suite_add_tcase (suite, tc);

  tcase_add_checked_fixture (tc, setup, teardown);
  tcase_add_test (tc, test_pipeline_signal_connect_success);
  tcase_add_test (tc, test_pipeline_signal_connect_null_pipeline_name);
  tcase_add_test (tc, test_pipeline_signal_connect_null_element_name);
  tcase_add_test (tc, test_pipeline_signal_connect_null_signal_name);
  tcase_add_test (tc, test_pipeline_signal_connect_null_manager);

  return suite;
}

GST_CHECK_MAIN (gstd_pipeline_signal_connect);
