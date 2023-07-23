cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+
project(synth_force_2099)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
set(RAYLIB_VERSION 4.5.0)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
  endif()
endif()

# Our Project
#file (GLOB source_files "code/*.cpp")  
set(source_files "code/main.cpp")

add_executable(${PROJECT_NAME}  ${source_files})

#set(raylib_VERBOSE 1)
target_link_libraries(${PROJECT_NAME} raylib)

# Web Configurations
if ("${PLATFORM}" STREQUAL "Web")
    # Tell Emscripten to build an example.html file.
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
    #set(CMAKE_EXECUTABLE_SUFFIX ".html")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY")
endif()

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()

message(PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
message(CMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")

# copy resource files into build folder:
# set resource filename, ${PROJECT_SOURCE_DIR} not mandatory, but recommended 
set(resource_files 
  "${PROJECT_SOURCE_DIR}/data/"
)
# copy files
file(COPY "${resource_files}" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
