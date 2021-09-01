# Tests and condition when to skip the test
gstd_tests = [
  ['test_libgstd_manager.c'],
  ['test_libgstd_debug.c'],
  ['test_libgstd_element_get.c'],
  ['test_libgstd_element_set.c'],
  ['test_libgstd_pipeline_bus_wait_async.c'],
  ['test_libgstd_pipeline_bus_wait.c'],
  ['test_libgstd_pipeline_create.c'],
  ['test_libgstd_pipeline_delete.c'],
  ['test_libgstd_pipeline_flush_start.c'],
  ['test_libgstd_pipeline_flush_stop.c'],
  ['test_libgstd_pipeline_get_graph.c'],
  ['test_libgstd_pipeline_get_state.c'],
  ['test_libgstd_pipeline_inject_eos.c'],
  ['test_libgstd_pipeline_list_elements.c'],
  ['test_libgstd_pipeline_list_properties.c'],
  ['test_libgstd_pipeline_list_signals.c'],
  ['test_libgstd_pipeline_list.c'],
  ['test_libgstd_pipeline_pause.c'],
  ['test_libgstd_pipeline_play.c'],
  ['test_libgstd_pipeline_stop.c'],
  ['test_libgstd_pipeline_seek.c'],
  ['test_libgstd_pipeline_signal_connect.c'],
  ['test_libgstd_pipeline_signal_disconnect.c'],
  ['test_libgstd_pipeline_verbose.c'],
  ['test_libgstd_signal_sink_connect.c'],
  ['test_libgstd_signal_sink_disconnect.c']
]
# Add C Definitions for tests
test_defines = [
  '-DTESTFILE="' + meson.current_source_dir() + '/meson.build"',
]

plugins_dir = []
# Define plugins path
if gst_dep.type_name() == 'pkgconfig'
   plugins_dir =[gst_dep.get_pkgconfig_variable('pluginsdir')]
endif

# Create environment object to  stores information about the environment
# variables set during tests.
# Define constant enviroment variable
env = environment()
env.set('GST_PLUGIN_SYSTEM_PATH_1_0', '')
env.set('CK_DEFAULT_TIMEOUT', '120')
env.set('GST_PLUGIN_PATH_1_0',  [meson.build_root()] + plugins_dir)

# Build and run tests
foreach t : gstd_tests
  fname = t[0]
  test_name = fname.split('.')[0].underscorify()
  skip_test = t.get(1, false)
  link_with_libs = t.get(2, [])

  if not skip_test
    # Creates a new executable for each test
    exe = executable(test_name, fname,
        c_args : gst_c_args + test_defines,
        cpp_args : gst_c_args + test_defines,
        include_directories : [configinc, libgstd_inc_dir, gstd_inc_dir],
        link_with : link_with_libs,
        dependencies : [test_libgstd_deps, lib_gstd_manager_dep, lib_gstd_dep],
    )

    # Define enviroment variable
    env.set('GST_REGISTRY', '@0@/@1@.registry'.format(meson.current_build_dir(), test_name))

    # Run tests
    test(test_name, exe, env: env, timeout : 60)
  endif
endforeach
