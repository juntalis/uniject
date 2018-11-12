
include_directories("${UNIJECT_SOURCE_DIR}/lib")

add_executable(data-test data_test.c)

set_target_properties(data-test PROPERTIES CLEAN_DIRECT_OUTPUT 1)

target_link_libraries(data-test uniject)
