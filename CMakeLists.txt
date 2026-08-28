cmake_minimum_required(VERSION 3.22)
project(VEK VERSION 1.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VEK_BUILD_CLI "Build the vek command-line executable" ON)
option(VEK_BUILD_TESTS "Build VEK tests" ON)

add_library(vek_runtime STATIC
    src/VekScriptEngine.cpp
    src/VekGameSystems.cpp
    src/VekEditorSystems.cpp
)
target_include_directories(vek_runtime PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
add_library(VEK::Runtime ALIAS vek_runtime)

if(VEK_BUILD_CLI)
    add_executable(vek cli/main.cpp)
    target_link_libraries(vek PRIVATE VEK::Runtime)
endif()

if(VEK_BUILD_TESTS)
    enable_testing()
    add_executable(vek_tests tests/vek_tests.cpp)
    target_link_libraries(vek_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_language COMMAND vek_tests)
    add_executable(vek_game_systems_tests tests/game_systems_tests.cpp)
    target_link_libraries(vek_game_systems_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_game_systems COMMAND vek_game_systems_tests)
    add_executable(vek_editor_systems_tests tests/editor_systems_tests.cpp)
    target_link_libraries(vek_editor_systems_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_editor_systems COMMAND vek_editor_systems_tests)
endif()

include(GNUInstallDirs)
install(TARGETS vek_runtime EXPORT VEKTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT VEKTargets FILE VEKTargets.cmake NAMESPACE VEK:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VEK)
