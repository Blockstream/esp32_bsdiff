cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(COMPONENTS main)
list(APPEND EXTRA_COMPONENT_DIRS "$ENV{IDF_PATH}/tools/mocks/driver/")
list(APPEND EXTRA_COMPONENT_DIRS "$ENV{IDF_PATH}/tools/mocks/freertos/")
list(APPEND EXTRA_COMPONENT_DIRS "..")

project(test_bsdiff)

#add_custom_command(
#    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/build/coverage.info"
#    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
#    COMMAND lcov --capture --directory . --output-file coverage.info
#    COMMENT "Create coverage report"
#    )

#add_custom_command(
#    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/build/coverage_report/"
#    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/build/coverage.info"
#    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
#    COMMAND genhtml coverage.info --output-directory coverage_report/
#    COMMENT "Turn coverage report into html-based visualization"
#    )

#add_custom_target(coverage
#    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
#    DEPENDS "coverage_report/"
#    )
