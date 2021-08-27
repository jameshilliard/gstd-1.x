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
#include "libgstd_parser.h"

/* Test Fixture */
static GstDManager *manager;
static gchar *enable, *threshold, *color, *reset;

static void
setup (void)
{
  gstd_manager_new (NULL, 0, &manager, NULL, 0, NULL);
}

static void
teardown (void)
{
  gstd_manager_free (manager);
}

GstdStatus
gstd_parser (GstdSession * session, const gchar * cmd, gchar ** response)
{
  GstdStatus ret = GSTD_LIB_OK;
  gchar **tokens;
  gchar *action, *args;

  gstd_assert_and_ret_val (NULL != session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != cmd, GSTD_LIB_NULL_ARGUMENT);

  tokens = g_strsplit (cmd, " ", 2);
  action = tokens[0];
  args = tokens[1];

  if (!g_ascii_strcasecmp ("debug_enable", action)) {
    enable = args;
  } else if (!g_ascii_strcasecmp ("debug_threshold", action)) {
    threshold = args;
  } else if (!g_ascii_strcasecmp ("debug_color", action)) {
    color = args;
  } else if (!g_ascii_strcasecmp ("debug_reset", action)) {
    reset = args;
  }

  return ret;
}

GST_START_TEST (test_debug_success_false_color)
{
  GstdStatus ret;
  const char *this_threshold = "*:2";
  const int this_color = 0;
  const int this_reset = 1;

  const gchar *expected_enable = "true";
  const gchar *expected_threshold = "*:2";
  const gchar *expected_color = "false";
  const gchar *expected_reset = "true";

  ret = gstd_manager_debug (manager, this_threshold, this_color, this_reset);
  assert_equals_int (GSTD_LIB_OK, ret);

  assert_equals_string (expected_enable, enable);
  assert_equals_string (expected_threshold, threshold);
  assert_equals_string (expected_color, color);
  assert_equals_string (expected_reset, reset);
}

GST_END_TEST;

GST_START_TEST (test_debug_success_true_color)
{
  GstdStatus ret;
  const char *this_threshold = "*:2";
  const int this_color = 1;
  const int this_reset = 1;

  const gchar *expected_enable = "true";
  const gchar *expected_threshold = "*:2";
  const gchar *expected_color = "true";
  const gchar *expected_reset = "true";

  ret = gstd_manager_debug (manager, this_threshold, this_color, this_reset);
  assert_equals_int (GSTD_LIB_OK, ret);

  assert_equals_string (expected_enable, enable);
  assert_equals_string (expected_threshold, threshold);
  assert_equals_string (expected_color, color);
  assert_equals_string (expected_reset, reset);
}

GST_END_TEST;

GST_START_TEST (test_manager_debug_null_client)
{
  GstdStatus ret;
  const char *threshold = "*:2";
  const int color_enable = 0;
  const int reset = 1;

  ret = gstd_manager_debug (NULL, threshold, color_enable, reset);
  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);
}

GST_END_TEST;

GST_START_TEST (test_manager_debug_null_threshold)
{
  GstdStatus ret;
  const char *threshold = NULL;
  const int color_enable = 0;
  const int reset = 1;

  ret = gstd_manager_debug (NULL, threshold, color_enable, reset);
  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);
}

GST_END_TEST;

static Suite *
gstd_manager_debug_suite (void)
{
  Suite *suite = suite_create ("gstd_manager_debug");
  TCase *tc = tcase_create ("general");

  suite_add_tcase (suite, tc);

  tcase_add_checked_fixture (tc, setup, teardown);
  tcase_add_test (tc, test_debug_success_false_color);
  tcase_add_test (tc, test_debug_success_true_color);
  tcase_add_test (tc, test_manager_debug_null_client);
  tcase_add_test (tc, test_manager_debug_null_threshold);

  return suite;
}

GST_CHECK_MAIN (gstd_manager_debug);
