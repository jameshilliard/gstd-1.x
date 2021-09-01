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

/* Test Fixture */
static GstDManager *manager;
static gchar *bus_filter, *bus_timeout, *bus_read;

enum
{
  TEST_OK,
  TEST_TIMEOUT,
  TEST_CORRUPTED
};
static gint test_to_run;

static const char *bus_response_ok_expected = "{\n\
  \"code\" : 0,\n\
  \"description\" : \"Success\",\n\
  \"response\" : {\n\
    \"type\" : \"eos\",\n\
    \"source\" : \"pipe\",\n\
    \"timestamp\" : \"99:99:99.999999999\",\n\
    \"seqnum\" : 1276\n\
}\n\
}";

static const char *bus_response_timeout_expected = "{\n\
  \"code\" : 0,\n\
  \"description\" : \"Success\",\n\
  \"response\" : null\n\
}";

static const char *bus_response_corrupted_expected = "{\n\
  \"code\" : 0,\n\
  \"description\" : \"Success\",\n\
  \"resp\" : null\n\
}";

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
  gchar **tokens = NULL;
  gchar *action = NULL;

  gstd_assert_and_ret_val (NULL != session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != cmd, GSTD_LIB_NULL_ARGUMENT);

  tokens = g_strsplit (cmd, " ", 2);
  action = tokens[0];

  if (!g_ascii_strcasecmp ("bus_filter", action)) {
    bus_filter = g_strdup_printf ("%s", cmd);
  } else if (!g_ascii_strcasecmp ("bus_timeout", action)) {
    bus_timeout = g_strdup_printf ("%s", cmd);
  } else if (!g_ascii_strcasecmp ("bus_read", action)) {
    bus_read = g_strdup_printf ("%s", cmd);
    switch (test_to_run) {
      case TEST_TIMEOUT:
        *response = g_strdup_printf ("%s", bus_response_timeout_expected);
        break;
      case TEST_CORRUPTED:
        *response = g_strdup_printf ("%s", bus_response_corrupted_expected);
        break;
      default:
        *response = g_strdup_printf ("%s", bus_response_ok_expected);
        break;
    }
  }

  return GSTD_LIB_OK;
}

GST_START_TEST (test_pipeline_bus_wait_success)
{
  GstdStatus ret;
  gchar *message = NULL;
  const gchar *pipeline_name = "pipe";
  const gchar *message_name = "eos";
  const gint64 timeout = -1;

  const gchar *bus_filter_expected = "bus_filter pipe eos";
  const gchar *bus_timeout_expected = "bus_timeout pipe -1";
  const gchar *bus_read_expected = "bus_read pipe";

  /* Test case to run */
  test_to_run = TEST_OK;

  ret =
      gstd_pipeline_bus_wait (manager, pipeline_name, message_name, timeout,
      &message);
  assert_equals_int (GSTD_LIB_OK, ret);

  assert_equals_string (bus_filter_expected, bus_filter);
  assert_equals_string (bus_timeout_expected, bus_timeout);
  assert_equals_string (bus_read_expected, bus_read);
  assert_equals_string (bus_response_ok_expected, message);

  g_free (message);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_bus_wait_timeout)
{
  GstdStatus ret;
  gchar *message = NULL;
  const gchar *pipeline_name = "pipe";
  const gchar *message_name = "eos";
  const gint64 timeout = -1;

  const gchar *bus_filter_expected = "bus_filter pipe eos";
  const gchar *bus_timeout_expected = "bus_timeout pipe -1";
  const gchar *bus_read_expected = "bus_read pipe";

  /* Test case to run */
  test_to_run = TEST_TIMEOUT;

  ret =
      gstd_pipeline_bus_wait (manager, pipeline_name, message_name, timeout,
      &message);
  assert_equals_int (GSTD_LIB_BUS_TIMEOUT, ret);

  assert_equals_string (bus_filter_expected, bus_filter);
  assert_equals_string (bus_timeout_expected, bus_timeout);
  assert_equals_string (bus_read_expected, bus_read);
  assert_equals_string (bus_response_timeout_expected, message);

  g_free (message);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_bus_wait_corrupted)
{
  GstdStatus ret;
  gchar *message = NULL;
  const gchar *pipeline_name = "pipe";
  const gchar *message_name = "eos";
  const gint64 timeout = -1;

  const gchar *bus_filter_expected = "bus_filter pipe eos";
  const gchar *bus_timeout_expected = "bus_timeout pipe -1";
  const gchar *bus_read_expected = "bus_read pipe";

  /* Test case to run */
  test_to_run = TEST_CORRUPTED;

  ret =
      gstd_pipeline_bus_wait (manager, pipeline_name, message_name, timeout,
      &message);
  assert_equals_int (GSTD_LIB_NOT_FOUND, ret);

  assert_equals_string (bus_filter_expected, bus_filter);
  assert_equals_string (bus_timeout_expected, bus_timeout);
  assert_equals_string (bus_read_expected, bus_read);
  assert_equals_string (bus_response_corrupted_expected, message);

  g_free (message);
}

GST_END_TEST;


static Suite *
gstd_pipeline_bus_wait_suite (void)
{
  Suite *suite = suite_create ("gstd_pipeline_bus_wait");
  TCase *tc = tcase_create ("general");

  suite_add_tcase (suite, tc);

  tcase_add_checked_fixture (tc, setup, teardown);
  tcase_add_test (tc, test_pipeline_bus_wait_success);
  tcase_add_test (tc, test_pipeline_bus_wait_timeout);
  tcase_add_test (tc, test_pipeline_bus_wait_corrupted);

  return suite;
}

GST_CHECK_MAIN (gstd_pipeline_bus_wait);
