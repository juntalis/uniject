
include_directories("${UNIJECT_SOURCE_DIR}/lib")

set(HIJACK_TEST "hijack-test${UNIJECT_BITS}")
set(INJECT_TEST "inject-test${UNIJECT_BITS}")


add_definitions(-DUNICODE=1)
add_definitions(-D_UNICODE=1)

add_executable(packing-test packing-test.c)
add_executable(${HIJACK_TEST} hijack-test.c)

set_target_properties(packing-test PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(${HIJACK_TEST} PROPERTIES CLEAN_DIRECT_OUTPUT 1)

target_link_libraries(packing-test uniject)
