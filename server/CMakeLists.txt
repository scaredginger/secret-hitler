cmake_minimum_required(VERSION 3.14)
project(server)

include_directories(SYSTEM lib/googletest/googletest/include lib/googletest/googletest src)
include_directories(SYSTEM uWebSockets/src)
include_directories(SYSTEM uWebSockets/uSockets/src)
include_directories(SYSTEM include/)

 # Download and unpack googletest at configure time
 configure_file(cmake_includes/GoogleTest.txt.in googletest-download/CMakeLists.txt)
 execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
         RESULT_VARIABLE result
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
 if(result)
     message(FATAL_ERROR "CMake step for googletest failed: ${result}")
 endif()
 execute_process(COMMAND ${CMAKE_COMMAND} --build .
         RESULT_VARIABLE result
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
 if(result)
     message(FATAL_ERROR "Build step for googletest failed: ${result}")
 endif()

 # Prevent overriding the parent project's compiler/linker
 # settings on Windows
 set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

 # Add googletest directly to our build. This defines
 # the gtest and gtest_main targets.
 add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
         ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
         EXCLUDE_FROM_ALL)


set(CMAKE_CXX_STANDARD 17)

set(USOCKETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/uWebSockets/uSockets/)
set(USOCKETS ${USOCKETS_DIR}/uSockets.a)
add_custom_command(OUTPUT ${USOCKETS} COMMAND make WORKING_DIRECTORY ${USOCKETS_DIR})

add_executable(server main.cpp game.h player.h manager.h common.h ${USOCKETS} slotMap.h)
target_compile_options(server PUBLIC -Wall -Wextra -Werror -Wno-missing-field-initializers)
target_link_libraries(server crypto ssl fmt ${USOCKETS} z)
set_target_properties(server PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

add_executable(test test/gameTests.cpp)
target_link_libraries(test gtest_main fmt)
