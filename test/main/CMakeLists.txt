idf_component_register(SRCS "test_bsdiff.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../bsdiff.c"
                    INCLUDE_DIRS
                    "."
                    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
                    REQUIRES cmock esp32_bsdiff)

#target_compile_options(${COMPONENT_LIB} PUBLIC --coverage)
#target_link_libraries(${COMPONENT_LIB} --coverage)
