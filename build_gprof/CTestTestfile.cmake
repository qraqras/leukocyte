# CMake generated Testfile for 
# Source directory: /workspaces/leukocyte
# Build directory: /workspaces/leukocyte/build_gprof
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_io_scan "/workspaces/leukocyte/build_gprof/test_io_scan")
set_tests_properties(test_io_scan PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/leukocyte/CMakeLists.txt;64;add_test;/workspaces/leukocyte/CMakeLists.txt;0;")
add_test(test_generated_config "/workspaces/leukocyte/build_gprof/test_generated_config")
set_tests_properties(test_generated_config PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/leukocyte/CMakeLists.txt;69;add_test;/workspaces/leukocyte/CMakeLists.txt;0;")
add_test(test_yaml_helpers "/workspaces/leukocyte/build_gprof/test_yaml_helpers")
set_tests_properties(test_yaml_helpers PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/leukocyte/CMakeLists.txt;74;add_test;/workspaces/leukocyte/CMakeLists.txt;0;")
add_test(test_rule_registry "/workspaces/leukocyte/build_gprof/test_rule_registry")
set_tests_properties(test_rule_registry PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/leukocyte/CMakeLists.txt;79;add_test;/workspaces/leukocyte/CMakeLists.txt;0;")
add_test(test_rule_manager "/workspaces/leukocyte/build_gprof/test_rule_manager")
set_tests_properties(test_rule_manager PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/leukocyte/CMakeLists.txt;83;add_test;/workspaces/leukocyte/CMakeLists.txt;0;")
add_test(test_yaml_case_sensitive "/workspaces/leukocyte/build_gprof/test_yaml_case_sensitive")
set_tests_properties(test_yaml_case_sensitive PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/leukocyte/CMakeLists.txt;88;add_test;/workspaces/leukocyte/CMakeLists.txt;0;")
add_test(test_config_loader "/workspaces/leukocyte/build_gprof/test_config_loader")
set_tests_properties(test_config_loader PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/leukocyte/CMakeLists.txt;93;add_test;/workspaces/leukocyte/CMakeLists.txt;0;")
add_test(test_loader_fullname "/workspaces/leukocyte/build_gprof/test_loader_fullname")
set_tests_properties(test_loader_fullname PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/leukocyte/CMakeLists.txt;98;add_test;/workspaces/leukocyte/CMakeLists.txt;0;")
add_test(test_config_loader_edgecases "/workspaces/leukocyte/build_gprof/test_config_loader_edgecases")
set_tests_properties(test_config_loader_edgecases PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/leukocyte/CMakeLists.txt;103;add_test;/workspaces/leukocyte/CMakeLists.txt;0;")
add_test(test_leukocyte "/workspaces/leukocyte/build_gprof/test_leukocyte")
set_tests_properties(test_leukocyte PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/leukocyte/CMakeLists.txt;136;add_test;/workspaces/leukocyte/CMakeLists.txt;0;")
add_test(test_cli "/workspaces/leukocyte/build_gprof/test_cli")
set_tests_properties(test_cli PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/leukocyte/CMakeLists.txt;141;add_test;/workspaces/leukocyte/CMakeLists.txt;0;")
subdirs("vendor/libyaml")
