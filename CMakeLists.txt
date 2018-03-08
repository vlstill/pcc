cmake_minimum_required(VERSION 3.0)
project(pcc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -pedantic -Werror -g" )
if ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
endif()

include(ExternalProject)
find_package(Git REQUIRED)

ExternalProject_Add(
    catch
    PREFIX ${CMAKE_BINARY_DIR}/catch
    GIT_REPOSITORY https://github.com/philsquared/Catch.git
    TIMEOUT 10
    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
   )

# Expose required variable (CATCH_INCLUDE_DIR) to parent scope
ExternalProject_Get_Property(catch source_dir)
set(CATCH_INCLUDE_DIR ${source_dir}/single_include CACHE INTERNAL "Path to include folder for Catch")

include_directories("${CATCH_INCLUDE_DIR}")

add_executable(pcc main.cpp pcc.cpp)
add_dependencies(pcc catch)
add_custom_target(pcc_run
                  COMMAND pcc --use-colour yes
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BUILD_DIR}
                  SOURCES pcc.cpp
                  DEPENDS pcc
                  VERBATIM
                 )
