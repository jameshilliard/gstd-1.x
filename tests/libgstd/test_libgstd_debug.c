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
#include <string.h>

#include "gstd_parser.h"
#include "libgstd.h"
#include "libgstd_assert.h"

/* Test Fixture */
static GstDManager *manager;

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

GST_START_TEST (test_manager_debug_successful)
{
  GstdStatus ret;
  const char *threshold = "*:2";
  const int color_enable = 0;
  const int reset = 1;

  ret = gstd_manager_debug (manager, threshold, color_enable, reset);
  assert_equals_int (GSTD_LIB_OK, ret);
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
  tcase_add_test (tc, test_manager_debug_successful);
  tcase_add_test (tc, test_manager_debug_null_client);
  tcase_add_test (tc, test_manager_debug_null_threshold);

  return suite;
}

GST_CHECK_MAIN (gstd_manager_debug);
