cmake_minimum_required(VERSION 3.9)
project(dex)

set(LIBSCRIPT_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libscript" CACHE PATH "Provide the directory of the libscript library")

if ("${LIBSCRIPT_PROJECT_DIR}" STREQUAL "")
  message(FATAL_ERROR "The directory of the libscript project must be specified")  
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # disables warning about DLL export 
  # this is annoying when using C++ STL
  add_compile_options(/wd4251)  
endif()

set(CMAKE_AUTOMOC ON)
find_package(Qt5Core)
find_package(Qt5Widgets)

##################################################################
###### libscript
##################################################################

## Patch liquid

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/patch/libscript.patch" "${CMAKE_CURRENT_SOURCE_DIR}/libscript/libscript.patch" COPYONLY)

execute_process (
    COMMAND git "apply" "libscript.patch"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libscript"
	RESULT_VARIABLE GIT_PATCH_LIBSCRIPT_RESULT
)

## Create libscript target

file(GLOB_RECURSE LIBSCRIPT_SRC_FILES ${LIBSCRIPT_PROJECT_DIR}/src/*.cpp)
file(GLOB_RECURSE LIBSCRIPT_HDR_FILES 
  ${LIBSCRIPT_PROJECT_DIR}/include/*.h
  ${LIBSCRIPT_PROJECT_DIR}/src/*.h
)

add_library(libscript SHARED ${LIBSCRIPT_HDR_FILES} ${LIBSCRIPT_SRC_FILES})
target_include_directories(libscript PUBLIC "${LIBSCRIPT_PROJECT_DIR}/include")
target_compile_definitions(libscript PRIVATE -DLIBSCRIPT_COMPILE_LIBRARY)
#target_compile_definitions(libscript PUBLIC -DLIBSCRIPT_STATIC_LINKING)
target_link_libraries(libscript Qt5::Core)
target_link_libraries(libscript Qt5::Gui)
target_link_libraries(libscript Qt5::Widgets)

foreach(_source IN ITEMS ${LIBSCRIPT_HDR_FILES} ${LIBSCRIPT_SRC_FILES})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

set_target_properties(libscript PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(libscript PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

##################################################################
###### liquid
##################################################################

set(LIQUID_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/liquid" CACHE PATH "Provide the directory of the liquid library")

## Patch liquid

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/patch/liquid.patch" "${CMAKE_CURRENT_SOURCE_DIR}/liquid/liquid.patch" COPYONLY)

execute_process (
    COMMAND git "apply" "liquid.patch"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/liquid"
	RESULT_VARIABLE GIT_PATCH_LIQUID_RESULT
)

## Copy JSON config

file(COPY "${LIQUID_PROJECT_DIR}/include/liquid/json-config.h" DESTINATION "${LIQUID_PROJECT_DIR}/json-toolkit/include/json-toolkit")
file(GLOB_RECURSE JSONTOOLKIT_HDR_FILES ${LIQUID_PROJECT_DIR}/json-toolkit/include/*.h)
add_custom_target(json-toolkit SOURCES ${JSONTOOLKIT_HDR_FILES})

## Create target

file(GLOB_RECURSE LIQUID_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/liquid/src/*.cpp)
file(GLOB_RECURSE LIQUID_HDR_FILES ${CMAKE_CURRENT_SOURCE_DIR}/liquid/include/*.h)
add_library(liquid SHARED ${LIQUID_HDR_FILES} ${LIQUID_SRC_FILES})
target_compile_definitions(liquid PRIVATE -DLIQUID_BUILD_SHARED_LIBRARY)
target_compile_definitions(liquid PUBLIC -DJSONTOOLKIT_HAS_CONFIG)
target_include_directories(liquid PUBLIC "${LIQUID_PROJECT_DIR}/json-toolkit/include")
target_include_directories(liquid PUBLIC "${LIQUID_PROJECT_DIR}/include")
target_link_libraries(liquid Qt5::Core)

set_target_properties(liquid PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(liquid PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

##################################################################
###### dex library
##################################################################

file(GLOB_RECURSE DEX_LIBRARY_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE DEX_LIBRARY_HDR_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/dex/*.h)

list(FILTER DEX_LIBRARY_SRC_FILES EXCLUDE REGEX "main.cpp")

add_library(dex STATIC ${DEX_LIBRARY_HDR_FILES} ${DEX_LIBRARY_SRC_FILES})
target_include_directories(dex PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(dex PUBLIC "${LIBSCRIPT_PROJECT_DIR}/include")
target_include_directories(dex PUBLIC "${LIQUID_PROJECT_DIR}/include")
target_include_directories(dex PUBLIC "${LIQUID_PROJECT_DIR}/json-toolkit/include")
target_include_directories(dex PUBLIC "lib/libscript")
target_link_libraries(dex Qt5::Core)
target_link_libraries(dex Qt5::Widgets)
target_link_libraries(dex libscript)
target_link_libraries(dex liquid)

foreach(_source IN ITEMS ${DEX_LIBRARY_HDR_FILES} ${DEX_LIBRARY_SRC_FILES})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

set_target_properties(dex PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(dex PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

##################################################################
###### dex exceutable
##################################################################

add_executable(app "src/main.cpp")
target_include_directories(app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_dependencies(app dex)
target_link_libraries(app dex)

#file(COPY "profiles" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(TARGET dex POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/profiles $<TARGET_FILE_DIR:dex>/profiles)

##################################################################
###### tests
##################################################################

add_subdirectory(tests)
