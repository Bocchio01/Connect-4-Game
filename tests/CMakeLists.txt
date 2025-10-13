file(GLOB CORE_TEST_SOURCES core/*.cpp)

foreach(test_src ${CORE_TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE connect4_core)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
