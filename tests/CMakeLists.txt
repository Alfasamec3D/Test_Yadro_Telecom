enable_testing()

find_package(GTest REQUIRED)

include(GoogleTest)


add_executable(dungeon_test unit/dungeon_test.cpp)

target_link_libraries(dungeon_test PRIVATE Dungeon GTest::gtest_main)

gtest_discover_tests(dungeon_test)



add_executable(parser_test unit/parser_test.cpp)

target_link_libraries(parser_test PRIVATE Parser GTest::gtest_main)

target_compile_definitions(parser_test PRIVATE
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/e2e/io")

gtest_discover_tests(parser_test)



add_executable(simulator_test unit/simulator_test.cpp)

target_link_libraries(simulator_test PRIVATE Simulator GTest::gtest_main)

gtest_discover_tests(simulator_test)



add_executable(alice_bot_test unit/alice_bot_test.cpp)

target_link_libraries(alice_bot_test PRIVATE AliceBot GTest::gtest_main)

gtest_discover_tests(alice_bot_test)


set(E2E_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/e2e")
set(CASES_DIR  "${E2E_DIR}/io")
set(RUNNER     "${E2E_DIR}/run.sh")
 
# Find all case directories at configure time.
file(GLOB case_dirs RELATIVE "${CASES_DIR}" "${CASES_DIR}/*")
 
foreach(case_name IN LISTS case_dirs)
    set(case_path "${CASES_DIR}/${case_name}")
    if(IS_DIRECTORY "${case_path}")
        add_test(
            NAME e2e_${case_name}
            COMMAND bash "${RUNNER}" "$<TARGET_FILE:main>" "${case_name}"
        )
    endif()
endforeach()