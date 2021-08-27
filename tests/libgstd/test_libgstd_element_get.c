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

#define GET_INTEGER 0
#define GET_STRING 1
#define GET_MIX 2

/* Test Fixture */
static GstDManager *manager;
static int json_case = 0;

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
gstd_json_is_null (const gchar * json, const gchar * name, gint * out)
{
  *out = 0;
  return GSTD_LIB_OK;
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
  switch (json_case) {
    case GET_INTEGER:
      *out = malloc (2);
      (*out)[0] = '5';
      (*out)[1] = '\0';
      break;
    case 1:
      *out = malloc (2);
      (*out)[0] = 'a';
      (*out)[1] = '\0';
      break;
    case 2:
      *out = malloc (4);
      (*out)[0] = 'a';
      (*out)[1] = ' ';
      (*out)[2] = '5';
      (*out)[3] = '\0';
      break;
  }
  return GSTD_LIB_OK;
}

GstdStatus
gstd_parser (GstdSession * session, const gchar * cmd, gchar ** response)
{
  gstd_assert_and_ret_val (NULL != session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != cmd, GSTD_LIB_NULL_ARGUMENT);

  *response = g_strdup_printf ("%s", "");

  return GSTD_LIB_OK;
}

GST_START_TEST (test_property_get_int)
{
  GstdStatus ret;
  const gchar *pipe_name = "pipe";
  const gchar *elem_name = "elem";
  const gchar *prop_name = "prop";
  int integer = -1;

  json_case = GET_INTEGER;
  ret =
      gstd_element_get (manager, pipe_name, elem_name, prop_name, "%d",
      &integer);
  assert_equals_int (GSTD_LIB_OK, ret);

  assert_equals_int (5, integer);
}

GST_END_TEST;

GST_START_TEST (test_property_get_string)
{
  GstdStatus ret;
  const gchar *pipe_name = "pipe";
  const gchar *elem_name = "elem";
  const gchar *prop_name = "prop";
  char string[64];

  json_case = GET_STRING;
  ret =
      gstd_element_get (manager, pipe_name, elem_name, prop_name, "%s",
      &string);
  assert_equals_int (GSTD_LIB_OK, ret);

  assert_equals_string ("a", string);
}

GST_END_TEST;

GST_START_TEST (test_property_get_combined)
{
  GstdStatus ret;
  const gchar *pipe_name = "pipe";
  const gchar *elem_name = "elem";
  const gchar *prop_name = "prop";
  char string[64];
  int integer = -1;

  json_case = GET_MIX;
  ret =
      gstd_element_get (manager, pipe_name, elem_name, prop_name, "%s %d",
      string, &integer);
  assert_equals_int (GSTD_LIB_OK, ret);

  assert_equals_string ("a", string);
  assert_equals_int (5, integer);
}

GST_END_TEST;

static Suite *
gstd_element_get_suite (void)
{
  Suite *suite = suite_create ("gstd_element_get");
  TCase *tc = tcase_create ("general");

  suite_add_tcase (suite, tc);

  tcase_add_checked_fixture (tc, setup, teardown);
  tcase_add_test (tc, test_property_get_int);
  tcase_add_test (tc, test_property_get_string);
  tcase_add_test (tc, test_property_get_combined);

  return suite;
}

GST_CHECK_MAIN (gstd_element_get);
