/*
 * GStreamer Daemon - gst-launch on steroids
 * C library abstracting gstd
 *
 * Copyright (c) 2015-2021 RidgeRun, LLC (http://www.ridgerun.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define _GNU_SOURCE
#include <stdarg.h>
#include <stdio.h>

#include "libgstd.h"
#include "gstd_ipc.h"
#include "gstd_tcp.h"
#include "gstd_unix.h"
#include "gstd_http.h"
#include "gstd_log.h"
#include "libgstd_assert.h"
#include "libgstd_json.h"
#include "libgstd_parser.h"
#include "libgstd_thread.h"
#include "gstd_element.h"


#define PRINTF_ERROR -1

static GType gstd_supported_ipc_to_ipc (SupportedIpcs code);
static void gstd_manager_init (GOptionGroup ** gst_group,
    int argc, char *argv[]);
static GstdStatus gstd_crud (GstDManager * manager, const char *operation,
    const char *pipeline_name);
static void *gstd_bus_thread (void *user_data);

struct _GstDManager
{
  GstdSession *session;
  GstdIpc **ipc_array;
  guint num_ipcs;
};

typedef struct _GstdThreadData GstdThreadData;
struct _GstdThreadData
{
  GstDManager *manager;
  const char *pipeline_name;
  const char *message;
  GstdPipelineBusWaitCallback func;
  void *user_data;
  long long timeout;
};

typedef struct _GstdSyncBusData GstdSyncBusData;
struct _GstdSyncBusData
{
  GstdCond cond;
  GstdMutex mutex;
  int waiting;
  char *message;
  GstdStatus ret;
};

static GType
gstd_supported_ipc_to_ipc (SupportedIpcs code)
{
  GType code_description[] = {
    [GSTD_IPC_TYPE_TCP] = GSTD_TYPE_TCP,
    [GSTD_IPC_TYPE_UNIX] = GSTD_TYPE_UNIX,
    [GSTD_IPC_TYPE_HTTP] = GSTD_TYPE_HTTP
  };

  const gint size = sizeof (code_description) / sizeof (gchar *);

  gstd_assert_and_ret_val (0 <= code, GSTD_TYPE_IPC);   // TODO: Proponer un GSTD_TYPE_DEFAULT
  gstd_assert_and_ret_val (size > code, GSTD_TYPE_IPC);

  return code_description[code];
}

static void
gstd_manager_init (GOptionGroup ** gst_group, int argc, char *argv[])
{
  gst_init (&argc, &argv);
  gstd_debug_init ();

  if (gst_group != NULL && *gst_group != NULL) {
    g_print ("OPTIONS INIT\n");
    *gst_group = gst_init_get_option_group ();
  } else {
    g_print ("SIMPLE INIT\n");
  }

}

static GstdStatus
gstd_crud (GstDManager * manager, const char *operation,
    const char *pipeline_name)
{
  GstdStatus ret = GSTD_LIB_OK;
  gchar *message = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != operation, GSTD_LIB_NULL_ARGUMENT);

  message = g_strdup_printf ("pipeline_%s %s", operation, pipeline_name);

  ret = gstd_parser (manager->session, message, NULL);

  g_free (message);
  message = NULL;

  return ret;
}

GstdStatus
gstd_manager_new (SupportedIpcs supported_ipcs[], guint num_ipcs,
    GstDManager ** out, GOptionGroup ** gst_group, int argc, char *argv[])
{
  GstdStatus ret = GSTD_LIB_OK;
  GstDManager *manager = NULL;
  GstdSession *session = NULL;
  GstdIpc **ipc_array = NULL;

  gstd_assert_and_ret_val (NULL != out, GSTD_LIB_NULL_ARGUMENT);

  manager = (GstDManager *) g_malloc0 (sizeof (*manager));
  session = gstd_session_new ("Session0");

  /* If there is ipcs, then initialize them */
  if (NULL != supported_ipcs && num_ipcs > 0) {
    ipc_array = g_malloc0 (num_ipcs * sizeof (*ipc_array));
    for (guint i = 0; i < num_ipcs; i++) {
      ipc_array[i] =
          GSTD_IPC (g_object_new (gstd_supported_ipc_to_ipc (supported_ipcs[i]),
              NULL));
    }
    manager->ipc_array = ipc_array;
  }

  manager->session = session;
  manager->num_ipcs = num_ipcs;

  *out = manager;

  /* Initialize GStreamer */
  gstd_manager_init (gst_group, argc, argv);

  return ret;
}

void
gstd_manager_ipc_options (GstDManager * manager, GOptionGroup ** ipc_group)
{
  gint i;

  gstd_assert_and_ret (NULL != manager);
  gstd_assert_and_ret (NULL != manager->ipc_array);
  gstd_assert_and_ret (NULL != ipc_group);

  for (i = 0; i < manager->num_ipcs; i++) {
    gstd_ipc_get_option_group (manager->ipc_array[i], &ipc_group[i]);
  }
}

gboolean
gstd_manager_ipc_start (GstDManager * manager)
{
  gboolean ipc_selected = FALSE;
  gboolean ret = TRUE;
  GstdReturnCode code;
  gint i;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->ipc_array, GSTD_LIB_NOT_FOUND);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NOT_FOUND);

  /* Verify if at leas one IPC mechanism was selected */
  for (i = 0; i < manager->num_ipcs; i++) {
    g_object_get (G_OBJECT (manager->ipc_array[i]), "enabled", &ipc_selected,
        NULL);

    if (ipc_selected) {
      break;
    }
  }

  /* If no IPC was selected, default to TCP */
  if (!ipc_selected) {
    g_object_set (G_OBJECT (manager->ipc_array[0]), "enabled", TRUE, NULL);
  }

  /* Run start for each IPC (each start method checks for the enabled flag) */
  for (i = 0; i < manager->num_ipcs; i++) {
    code = gstd_ipc_start (manager->ipc_array[i], manager->session);
    if (code) {
      g_printerr ("Couldn't start IPC : (%s)\n",
          G_OBJECT_TYPE_NAME (manager->ipc_array[i]));
      ret = FALSE;
    }
  }

  return ret;
}

void
gstd_manager_ipc_stop (GstDManager * manager)
{
  gint i = 0;

  gstd_assert_and_ret (NULL != manager);
  gstd_assert_and_ret (NULL != manager->session);

  /* Run stop for each IPC */
  for (i = 0; i < manager->num_ipcs; i++) {
    if (NULL != manager->ipc_array[i] && TRUE == manager->ipc_array[i]->enabled) {
      gstd_ipc_stop (manager->ipc_array[i]);
      g_clear_object (&manager->ipc_array[i]);
    }
  }
}

void
gstd_manager_free (GstDManager * manager)
{
  gstd_assert_and_ret (NULL != manager);
  gstd_manager_ipc_stop (manager);
  g_free (manager->ipc_array);
  g_object_unref (manager->session);
  g_free (manager);
  gst_deinit ();
}

GstdStatus
gstd_manager_debug (GstDManager * manager, const char *threshold,
    const int colors, const int reset)
{
  GstdStatus ret = GSTD_LIB_OK;
  const char *colored;
  const char *reset_bool;
  gchar *message = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != threshold, GSTD_LIB_NULL_ARGUMENT);

  message = g_strdup_printf ("debug_enable true");
  ret = gstd_parser (manager->session, message, NULL);
  if (ret != GSTD_LIB_OK) {
    goto out;
  }

  message = g_strdup_printf ("debug_threshold %s", threshold);
  ret = gstd_parser (manager->session, message, NULL);
  if (ret != GSTD_LIB_OK) {
    goto out;
  }

  colored = colors == 0 ? "false" : "true";
  message = g_strdup_printf ("debug_color %s", colored);
  ret = gstd_parser (manager->session, message, NULL);
  if (ret != GSTD_LIB_OK) {
    goto out;
  }

  reset_bool = reset == 0 ? "false" : "true";
  message = g_strdup_printf ("debug_reset %s", reset_bool);
  ret = gstd_parser (manager->session, message, NULL);
  if (ret != GSTD_LIB_OK) {
    goto out;
  }

out:
  g_free (message);
  message = NULL;

  return ret;
}

GstdStatus
gstd_pipeline_create (GstDManager * manager,
    const char *pipeline_name, const char *pipeline_desc)
{
  GstdStatus ret = GSTD_LIB_OK;
  gchar *message = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_desc, GSTD_LIB_NULL_ARGUMENT);

  message = g_strdup_printf ("%s %s", pipeline_name, pipeline_desc);

  ret = gstd_crud (manager, "create", message);

  g_free (message);
  message = NULL;

  return ret;
}

GstdStatus
gstd_pipeline_list (GstDManager * manager, char **pipelines[], int *list_lenght)
{
  GstdStatus ret = GSTD_LIB_OK;
  gchar *message = NULL;
  gchar *response = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipelines, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != list_lenght, GSTD_LIB_NULL_ARGUMENT);

  message = g_strdup_printf ("list_pipelines");

  ret = gstd_parser_parse_cmd (manager->session, message, &response);
  if (ret != GSTD_LIB_OK) {
    goto out;
  }

  ret =
      gstd_json_get_child_char_array (response, "nodes", "name", pipelines,
      list_lenght);
  if (ret != GSTD_LIB_OK) {
    goto out;
  }

out:
  g_free (message);
  g_free (response);
  message = NULL;
  response = NULL;

  return ret;
}

GstdStatus
gstd_pipeline_delete (GstDManager * manager, const char *pipeline_name)
{
  GstdStatus ret = GSTD_LIB_OK;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);

  ret = gstd_crud (manager, "delete", pipeline_name);

  return ret;
}

GstdStatus
gstd_pipeline_play (GstDManager * manager, const char *pipeline_name)
{
  GstdStatus ret = GSTD_LIB_OK;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);

  ret = gstd_crud (manager, "play", pipeline_name);

  return ret;
}

GstdStatus
gstd_pipeline_pause (GstDManager * manager, const char *pipeline_name)
{
  GstdStatus ret = GSTD_LIB_OK;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);

  ret = gstd_crud (manager, "pause", pipeline_name);

  return ret;
}

GstdStatus
gstd_pipeline_stop (GstDManager * manager, const char *pipeline_name)
{
  GstdStatus ret = GSTD_LIB_OK;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);

  ret = gstd_crud (manager, "stop", pipeline_name);

  return ret;
}

GstdStatus
gstd_pipeline_get_graph (GstDManager * manager, const char *pipeline_name,
    char **response)
{
  GstdStatus ret;

  gchar *message = NULL;
  gchar *output = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != response, GSTD_LIB_NULL_ARGUMENT);

  message = g_strdup_printf ("pipeline_get_graph %s", pipeline_name);

  ret = gstd_parser_parse_cmd (manager->session, message, &output);

  *response = g_strdup_printf ("%s", output);

  g_free (message);
  g_free (output);
  message = NULL;
  output = NULL;

  return ret;
}

GstdStatus
gstd_pipeline_verbose (GstDManager * manager, const char *pipeline_name,
    int value)
{
  GstdStatus ret = GSTD_LIB_OK;
  const char *verbosed;
  gchar *message = NULL;
  gchar *output = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);

  verbosed = value == 0 ? "false" : "true";
  message = g_strdup_printf ("pipeline_verbose %s %s", pipeline_name, verbosed);
  ret = gstd_parser_parse_cmd (manager->session, message, &output);

  g_free (message);
  g_free (output);
  message = NULL;
  output = NULL;

  return ret;
}

GstdStatus
gstd_element_get (GstDManager * manager, const char *pname,
    const char *element, const char *property, const char *format, ...)
{
  GstdStatus ret;
  va_list ap;
  gchar *message = NULL;
  gchar *response = NULL;
  char *out;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pname, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != element, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != property, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != format, GSTD_LIB_NULL_ARGUMENT);

  va_start (ap, format);
  message = g_strdup_printf ("element_get %s %s %s", pname, element, property);
  ret = gstd_parser (manager->session, message, &response);
  if (ret != GSTD_LIB_OK) {
    goto unref;
  }

  ret = gstd_json_child_string (response, "value", &out);
  if (ret != GSTD_LIB_OK) {
    goto unref_response;
  }

  vsscanf (out, format, ap);

  free (out);

unref_response:
  g_free (response);

unref:
  va_end (ap);
  return ret;
}

GstdStatus
gstd_element_set (GstDManager * manager, const char *pname,
    const char *element, const char *parameter, const char *format, ...)
{
  GstdStatus ret;
  int asprintf_ret;
  va_list ap;

  gchar *message = NULL;
  gchar *values = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pname, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != element, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != parameter, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != format, GSTD_LIB_NULL_ARGUMENT);

  va_start (ap, format);
  asprintf_ret = vasprintf (&values, format, ap);
  if (PRINTF_ERROR == asprintf_ret) {
    return GSTD_LIB_OOM;
  }

  message =
      g_strdup_printf ("element_set %s %s %s %s", pname, element, parameter,
      values);
  ret = gstd_parser (manager->session, message, NULL);
  if (ret != GSTD_LIB_OK) {
    goto unref;
  }

unref:
  g_free (message);
  message = NULL;

  return ret;
}

GstdStatus
gstd_element_properties_list (GstDManager * manager, const char *pipeline_name,
    char *element, char **properties[], int *list_lenght)
{
  GstdStatus ret = GSTD_LIB_OK;
  gchar *message = NULL;
  gchar *response = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != element, GSTD_LIB_NULL_ARGUMENT);

  message = g_strdup_printf ("list_properties %s %s", pipeline_name, element);

  ret = gstd_parser_parse_cmd (manager->session, message, &response);
  if (ret != GSTD_LIB_OK) {
    goto out;
  }
  ret =
      gstd_json_get_child_char_array (response, "nodes", "name", properties,
      list_lenght);

out:
  g_free (message);
  g_free (response);
  message = NULL;
  response = NULL;

  return ret;
}

GstdStatus
gstd_pipeline_flush_start (GstDManager * manager, const char *pipeline_name)
{
  GstdStatus ret = GSTD_LIB_OK;
  gchar *message = NULL;
  gchar *output = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);

  message = g_strdup_printf ("event_flush_start %s", pipeline_name);
  ret = gstd_parser_parse_cmd (manager->session, message, &output);

  g_free (message);
  g_free (output);
  message = NULL;
  output = NULL;

  return ret;
}

GstdStatus
gstd_pipeline_flush_stop (GstDManager * manager, const char *pipeline_name,
    const int reset)
{
  GstdStatus ret = GSTD_LIB_OK;
  const gchar *to_reset;
  gchar *message = NULL;
  gchar *output = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);

  to_reset = reset == 0 ? "false" : "true";
  message =
      g_strdup_printf ("event_flush_start %s %s", pipeline_name, to_reset);
  ret = gstd_parser_parse_cmd (manager->session, message, &output);

  g_free (message);
  g_free (output);
  message = NULL;
  output = NULL;

  return ret;
}

GstdStatus
gstd_pipeline_inject_eos (GstDManager * manager, const char *pipeline_name)
{
  GstdStatus ret = GSTD_LIB_OK;
  gchar *message = NULL;
  gchar *output = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);

  message = g_strdup_printf ("event_eos %s", pipeline_name);
  ret = gstd_parser_parse_cmd (manager->session, message, &output);

  g_free (message);
  g_free (output);
  message = NULL;
  output = NULL;

  return ret;
}

GstdStatus
gstd_pipeline_seek (GstDManager * manager, const char *pname,
    double rate, int format, int flags, int start_type, long long start,
    int stop_type, long long stop)
{
  GstdStatus ret = GSTD_LIB_OK;
  gchar *message = NULL;
  gchar *output = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pname, GSTD_LIB_NULL_ARGUMENT);

  message = g_strdup_printf ("event_seek %s %f %d %d %d %lld %d %lld",
      pname, rate, format, flags, start_type, start, stop_type, stop);
  ret = gstd_parser_parse_cmd (manager->session, message, &output);

  g_free (message);
  g_free (output);
  message = NULL;
  output = NULL;

  return ret;
}

GstdStatus
gstd_pipeline_list_elements (GstDManager * manager, const char *pipeline_name,
    char **elements[], int *list_lenght)
{
  GstdStatus ret = GSTD_LIB_OK;
  gchar *message = NULL;
  gchar *response = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != elements, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != list_lenght, GSTD_LIB_NULL_ARGUMENT);

  message = g_strdup_printf ("list_elements %s", pipeline_name);

  ret = gstd_parser_parse_cmd (manager->session, message, &response);
  if (ret != GSTD_LIB_OK) {
    goto out;
  }
  ret =
      gstd_json_get_child_char_array (response, "nodes", "name", elements,
      list_lenght);
  if (ret != GSTD_LIB_OK) {
    goto out;
  }

out:
  g_free (message);
  g_free (response);
  message = NULL;
  response = NULL;

  return ret;
}

static void *
gstd_bus_thread (void *user_data)
{
  GstdThreadData *data = (GstdThreadData *) user_data;
  GstdStatus ret = GSTD_LIB_OK;
  gchar *message = NULL;
  gchar *response = NULL;
  const char *pipeline_name = data->pipeline_name;
  const char *message_name = data->message;
  long long timeout = data->timeout;
  GstDManager *manager = data->manager;

  message = g_strdup_printf ("bus_read %s", pipeline_name);
  ret = gstd_parser (manager->session, message, &response);
  if (ret != GSTD_LIB_OK) {
    goto out;
  }

  data->func (manager, pipeline_name, message_name, timeout, response,
      data->user_data);

out:
  g_free (message);
  g_free (response);
  message = NULL;
  response = NULL;

  return NULL;
}

GstdStatus
gstd_pipeline_bus_wait_async (GstDManager * manager,
    const char *pipeline_name, const char *message_name,
    const long long timeout, GstdPipelineBusWaitCallback callback,
    void *user_data)
{
  GstdStatus ret = GSTD_LIB_OK;
  GstdThread thread;
  GstdThreadData *data;
  gchar *message = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != message_name, GSTD_LIB_NULL_ARGUMENT);

  message = g_strdup_printf ("bus_filter %s %s", pipeline_name, message_name);
  ret = gstd_parser (manager->session, message, NULL);
  if (ret != GSTD_LIB_OK) {
    goto out;
  }

  message = g_strdup_printf ("bus_timeout %s %lld", pipeline_name, timeout);
  ret = gstd_parser (manager->session, message, NULL);
  if (ret != GSTD_LIB_OK) {
    goto out;
  }

  data = malloc (sizeof (GstdThreadData));
  data->manager = manager;
  data->pipeline_name = pipeline_name;
  data->message = message_name;
  data->timeout = timeout;
  data->func = callback;
  data->user_data = user_data;

  ret = gstd_thread_new (&thread, gstd_bus_thread, data);

out:
  g_free (message);
  message = NULL;

  return ret;
}

/*
 * The Wunused-parameter warning is ignored for this function since it should
 * carry all of this information for the callback even when the parameters
 * aren't directly used
 */
#pragma GCC diagnostic ignored "-Wunused-parameter"
static GstdStatus
gstd_pipeline_bus_wait_callback (GstDManager * manager,
    const char *pipeline_name, const char *message_name,
    const long long timeout, char *message, void *user_data)
{
  GstdSyncBusData *data = (GstdSyncBusData *) user_data;
  GstdStatus ret = GSTD_LIB_OK;
  const int msglen = strlen (message) + 1;
  const char *response_tag = "response";
  int is_null;

  gstd_mutex_lock (&(data->mutex));
  data->waiting = 0;
  data->message = (char *) malloc (msglen);
  memcpy (data->message, message, msglen);

  /* If a valid string was received, a valid bus message was received.
     Otherwise, a timeout occurred */
  ret = gstd_json_is_null (message, response_tag, &is_null);
  if (GSTD_LIB_OK == ret) {
    data->ret = is_null ? GSTD_LIB_BUS_TIMEOUT : ret;
  } else {
    data->ret = ret;
  }

  gstd_cond_signal (&(data->cond));
  gstd_mutex_unlock (&(data->mutex));

  return GSTD_LIB_OK;
}

#pragma GCC diagnostic pop
GstdStatus
gstd_pipeline_bus_wait (GstDManager * manager,
    const char *pipeline_name, const char *message_name,
    const long long timeout, char **message)
{
  GstdStatus ret = GSTD_LIB_OK;
  GstdSyncBusData data;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != message_name, GSTD_LIB_NULL_ARGUMENT);

  gstd_cond_init (&(data.cond));
  gstd_mutex_init (&(data.mutex));
  data.waiting = 1;

  ret = gstd_pipeline_bus_wait_async (manager, pipeline_name, message_name,
      timeout, gstd_pipeline_bus_wait_callback, &data);
  if (GSTD_LIB_OK != ret) {
    return ret;
  }

  gstd_mutex_lock (&(data.mutex));
  while (1 == data.waiting) {
    gstd_cond_wait (&(data.cond), &(data.mutex));
  }
  gstd_mutex_unlock (&(data.mutex));

  /* Output the message back to the user */
  *message = data.message;

  return data.ret;
}

GstdStatus
gstd_pipeline_get_state (GstDManager * manager, const char *pipeline_name,
    char **out)
{
  GstdStatus ret = GSTD_LIB_OK;
  gchar *message = NULL;
  gchar *response = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != out, GSTD_LIB_NULL_ARGUMENT);

  message = g_strdup_printf ("read /pipelines/%s/state", pipeline_name);

  ret = gstd_parser_parse_cmd (manager->session, message, &response);
  if (ret != GSTD_LIB_OK) {
    goto out;
  }
  ret = gstd_json_child_string (response, "value", out);

out:
  g_free (message);
  g_free (response);
  message = NULL;
  response = NULL;

  return ret;
}

GstdStatus
gstd_pipeline_list_signals (GstDManager * manager, const char *pipeline_name,
    const char *element, char **signals[], int *list_lenght)
{
  GstdStatus ret = GSTD_LIB_OK;
  gchar *message = NULL;
  gchar *response = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != element, GSTD_LIB_NULL_ARGUMENT);

  message = g_strdup_printf ("list_signals %s %s", pipeline_name, element);

  ret = gstd_parser_parse_cmd (manager->session, message, &response);
  if (ret != GSTD_LIB_OK) {
    goto out;
  }
  ret =
      gstd_json_get_child_char_array (response, "nodes", "name", signals,
      list_lenght);

out:
  g_free (message);
  g_free (response);
  message = NULL;
  response = NULL;

  return ret;
}

GstdStatus
gstd_pipeline_signal_connect (GstDManager * manager, const char *pipeline_name,
    const char *element, const char *signal, const int timeout, char **response)
{
  GstdStatus ret = GSTD_LIB_OK;
  gchar *message = NULL;
  gchar *out = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != element, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != signal, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != response, GSTD_LIB_NULL_ARGUMENT);

  message =
      g_strdup_printf ("signal_timeout %s %s %s %d", pipeline_name, element,
      signal, timeout);
  ret = gstd_parser_parse_cmd (manager->session, message, &out);
  if (ret != GSTD_LIB_OK) {
    response = NULL;
    goto out;
  }

  message =
      g_strdup_printf ("signal_connect %s %s %s", pipeline_name, element,
      signal);
  ret = gstd_parser_parse_cmd (manager->session, message, response);

out:
  g_free (message);
  g_free (out);
  message = NULL;
  out = NULL;

  return ret;
}

GstdStatus
gstd_pipeline_signal_disconnect (GstDManager * manager,
    const char *pipeline_name, const char *element, const char *signal)
{
  GstdStatus ret = GSTD_LIB_OK;
  gchar *message = NULL;
  gchar *out = NULL;

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != element, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != signal, GSTD_LIB_NULL_ARGUMENT);

  message =
      g_strdup_printf ("signal_disconnect %s %s %s", pipeline_name, element,
      signal);
  ret = gstd_parser_parse_cmd (manager->session, message, &out);

  g_free (message);
  g_free (out);
  message = NULL;
  out = NULL;

  return ret;
}

GstdStatus
gstd_signal_sink_connect (GstDManager * manager,
    const char *pipeline_name, const char *sink_name, gulong * handler_id,
    GstdSinkCallback callback, void *user_data)
{
  GstdStatus ret = GSTD_LIB_OK;
  GstdObject *node;
  GstdObject *pipe;
  GstdList *listElements = NULL;
  GstdElement *appSinkGstD = NULL;
  GstElement *appSink = NULL;

  const gchar *uri = "/pipelines";

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != sink_name, GSTD_LIB_NULL_ARGUMENT);

  ret = gstd_get_by_uri (manager->session, uri, &node);
  if (ret || NULL == node) {
    goto out;
  }

  gstd_object_read (node, pipeline_name, &pipe);
  if (NULL == pipe) {
    ret = GSTD_LIB_NOT_FOUND;
    goto out;
  }

  g_object_get (pipe, "elements", &listElements, NULL);
  if (NULL == listElements) {
    ret = GSTD_LIB_NOT_FOUND;
    goto out;
  }

  appSinkGstD = (GstdElement *) gstd_list_find_child (listElements, sink_name);
  if (NULL == appSinkGstD) {
    ret = GSTD_LIB_NOT_FOUND;
    goto out;
  }

  g_object_get (appSinkGstD, "gstelement", &appSink, NULL);
  if (NULL == appSink) {
    ret = GSTD_LIB_NOT_FOUND;
    goto out;
  }

  *handler_id =
      g_signal_connect (appSink, "new-sample", G_CALLBACK (callback),
      user_data);
  if (0 >= handler_id) {
    ret = GSTD_LIB_NOT_FOUND;
  }

out:
  g_object_unref (node);
  g_object_unref (pipe);
  return ret;
}

GstdStatus
gstd_signal_sink_disconnect (GstDManager * manager,
    const char *pipeline_name, const char *sink_name, gulong handler_id)
{
  GstdStatus ret = GSTD_LIB_OK;
  GstdObject *node;
  GstdObject *pipe;
  GstdList *listElements = NULL;
  GstdElement *appSinkGstD = NULL;
  GstElement *appSink = NULL;

  const gchar *uri = "/pipelines";

  gstd_assert_and_ret_val (NULL != manager, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != manager->session, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != pipeline_name, GSTD_LIB_NULL_ARGUMENT);
  gstd_assert_and_ret_val (NULL != sink_name, GSTD_LIB_NULL_ARGUMENT);

  ret = gstd_get_by_uri (manager->session, uri, &node);
  if (ret || NULL == node) {
    goto out;
  }

  gstd_object_read (node, pipeline_name, &pipe);
  if (NULL == pipe) {
    ret = GSTD_LIB_NOT_FOUND;
    goto out;
  }

  g_object_get (pipe, "elements", &listElements, NULL);
  if (NULL == listElements) {
    ret = GSTD_LIB_NOT_FOUND;
    goto out;
  }

  appSinkGstD = (GstdElement *) gstd_list_find_child (listElements, sink_name);
  if (NULL == appSinkGstD) {
    ret = GSTD_LIB_NOT_FOUND;
    goto out;
  }

  g_object_get (appSinkGstD, "gstelement", &appSink, NULL);
  if (NULL == appSink) {
    ret = GSTD_LIB_NOT_FOUND;
    goto out;
  }

  g_signal_handler_disconnect (appSink, handler_id);

out:
  g_object_unref (node);
  g_object_unref (pipe);
  return ret;
}
