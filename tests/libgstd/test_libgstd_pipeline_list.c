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
gstd_parser (GstdSession * session, const gchar * cmd, gchar ** response)
{
  gstd_assert_and_ret_val (NULL != session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != cmd, GSTD_LIB_NULL_ARGUMENT);

  return GSTD_LIB_OK;
}

GST_START_TEST (test_pipeline_list_success)
{
  GstdStatus ret;
  gchar **response = NULL;
  int array_lenght;

  ret = gstd_pipeline_list (manager, &response, &array_lenght);

  fail_if (GSTD_LIB_OK != ret);

  g_free (response);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_list_null_list_lenght)
{
  GstdStatus ret;
  gchar **response = NULL;

  ret = gstd_pipeline_list (manager, &response, NULL);

  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);

  g_free (response);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_list_null_response)
{
  GstdStatus ret;
  int array_lenght;

  ret = gstd_pipeline_list (manager, NULL, &array_lenght);

  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);

}

GST_END_TEST;

GST_START_TEST (test_pipeline_list_null_manager)
{
  GstdStatus ret;
  gchar **response = NULL;
  int array_lenght;

  ret = gstd_pipeline_list (NULL, &response, &array_lenght);
  assert_equals_int (GSTD_LIB_NULL_ARGUMENT, ret);

  g_free (response);
}

GST_END_TEST;


static Suite *
gstd_pipeline_list_suite (void)
{
  Suite *suite = suite_create ("gstd_pipeline_list");
  TCase *tc = tcase_create ("general");

  suite_add_tcase (suite, tc);

  tcase_add_checked_fixture (tc, setup, teardown);
  tcase_add_test (tc, test_pipeline_list_success);
  tcase_add_test (tc, test_pipeline_list_null_list_lenght);
  tcase_add_test (tc, test_pipeline_list_null_response);
  tcase_add_test (tc, test_pipeline_list_null_manager);

  return suite;
}

GST_CHECK_MAIN (gstd_pipeline_list);
