cmake_minimum_required(VERSION 3.10)
project(ear-production-combined VERSION 0.6.0.0 LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_modules")

include(eps_set_install_paths)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    eps_set_install_paths()
endif()

include(CTest)
option(BUILD_TESTING "Build tests" ON)
set(Boost_USE_STATIC_LIBS ON)
include("enable_msvc_static_runtime")
find_package(nng REQUIRED)
add_subdirectory(submodules)

add_subdirectory(ear-production-suite)
set_target_properties(
	ear_logging_server
	ear-plugin-base
	direct_speakers_VST3 
	ear_binaural_monitoring_VST3
	ear_monitoring_0+2+0_VST3
	ear_monitoring_0+5+0_VST3
	ear_monitoring_0+7+0_VST3
	ear_monitoring_2+5+0_VST3
	ear_monitoring_3+7+0_VST3
	ear_monitoring_4+5+0_VST3
	ear_monitoring_4+5+1_VST3
	ear_monitoring_4+7+0_VST3
	ear_monitoring_4+9+0_VST3
	ear_monitoring_9+10+3_VST3
	object_VST3
	scene_VST3
PROPERTIES FOLDER EPS-ear_production_suite_vsts)
set_target_properties(
	connection_id_tests 
	connection_manager_tests 
	message_coding_tests 
	monitoring_audio_processor_tests 
	nng_tests 
	plugin_test_tools 
	programme_store_adm_populator_tests
	programme_store_adm_serializer_tests
	protobuf_tests
	scene_gains_calculator_tests
	scene_tests
	test_host_runner
	test_sender
	variable_block_adapter_tests
PROPERTIES FOLDER EPS-ear_production_suite_vsts-tests)
set_target_properties(
	input_plugin_mockup 
	monitoring_mockup 
	scene_mockup
PROPERTIES FOLDER EPS-ear_production_suite_vsts-mockups)

add_subdirectory(reaper-adm-extension)
set_target_properties(reaper_adm reaper_adm_object PROPERTIES FOLDER EPS-reaper_extension)
set_target_properties(testacceptreaper_adm testreaper_adm PROPERTIES FOLDER EPS-reaper_extension-tests)

add_subdirectory(reaper_adm_stem_plugin)
set_target_properties(adm_export_source_VST3 PROPERTIES FOLDER EPS-adm_export_source_vst)

add_subdirectory(tools)
set_target_properties(proj_upgrade project_upgrade PROPERTIES FOLDER EPS-tools)
set_target_properties(test_project_upgrade PROPERTIES FOLDER EPS-tools-tests)

list(FILTER HEADERS EXCLUDE REGEX "^\\..+") 
list(FILTER SOURCES EXCLUDE REGEX "^\\..+")

if(EPS_BUILD_PACKAGE)
    add_subdirectory(packaging)
endif()
