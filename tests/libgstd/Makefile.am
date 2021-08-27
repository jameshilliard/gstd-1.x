TESTS = \
        test_libgstd_manager                    \
		test_libgstd_debug                      \
		test_libgstd_element_get                \
		test_libgstd_element_set                \
		test_libgstd_pipeline_bus_wait_async    \
		test_libgstd_pipeline_bus_wait          \
		test_libgstd_pipeline_create  

check_PROGRAMS = $(TESTS)

AM_CFLAGS = $(GST_CFLAGS)					\
			 -I$(top_srcdir)/gstd/          \
			 $(GIO_CFLAGS)					\
			 $(GIO_UNIX_CFLAGS)				\
			 $(LIBSOUP_CFLAGS)				\
			 $(GJSON_CFLAGS)				\
			 -DGSTD_LOG_STATE_DIR=\"$(GSTD_LOG_STATE_DIR)\"	\
			 -DGSTD_RUN_STATE_DIR=\"$(GSTD_RUN_STATE_DIR)\" \
			 $(JANSSON_CFLAGS) -pthread  \
             -I$(top_srcdir)/libgstd

AM_LDFLAGS = $(GST_LIBS) $(GIO_LIBS) $(GJSON_LIBS) $(LIBSOUP_LIBS) $(JANSSON_LIBS) -pthread
LDADD = $(top_srcdir)/gstd/libgstd-core.la
