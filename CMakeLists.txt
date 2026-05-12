cmake_minimum_required(VERSION 3.13)

project("AliceBot" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(Resources INTERFACE)

target_include_directories(Resources INTERFACE "include/Resources")



add_library(Dungeon INTERFACE)

target_include_directories(Dungeon INTERFACE "include/Dungeon")

target_link_libraries(Dungeon INTERFACE Resources)



add_library(Parser STATIC lib/Parser.cpp)

target_include_directories(Parser PUBLIC "include/Parser")

target_link_libraries(Parser PUBLIC Dungeon)




add_library(IBot INTERFACE)

target_include_directories(IBot INTERFACE "include/IBot")

target_link_libraries(IBot INTERFACE Resources)



add_library(AliceBot STATIC lib/AliceBot.cpp)

target_include_directories(AliceBot PUBLIC "include/AliceBot")

target_link_libraries(AliceBot PRIVATE IBot)



add_library(Simulator STATIC lib/Simulator.cpp)

target_include_directories(Simulator PUBLIC "include/Simulator")

target_link_libraries(Simulator PUBLIC Dungeon IBot)


add_executable(main main.cpp)

target_link_libraries(main PRIVATE AliceBot Simulator Parser)


option(BUILD_TESTS "Build tests" ON)
if (BUILD_TESTS)
    add_subdirectory(tests)
endif()