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

add_subdirectory(lib)

##################################################################
###### core
##################################################################

file(GLOB_RECURSE CORE_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp)
file(GLOB_RECURSE CORE_HDR_FILES 
  ${CMAKE_CURRENT_SOURCE_DIR}/include/dex/core/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.h
)
add_library(core STATIC ${CORE_HDR_FILES} ${CORE_SRC_FILES})
target_include_directories(core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(core PUBLIC "${LIBSCRIPT_PROJECT_DIR}/include")
target_include_directories(core PUBLIC "lib/libscript")
target_link_libraries(core Qt5::Core)
target_link_libraries(core Qt5::Widgets)
target_link_libraries(core libscript)

##################################################################
###### api
##################################################################

file(GLOB_RECURSE API_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/api/*.cpp)
file(GLOB_RECURSE API_HDR_FILES 
  ${CMAKE_CURRENT_SOURCE_DIR}/include/dex/api/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/api/*.h
)
add_library(api STATIC ${API_HDR_FILES} ${API_SRC_FILES})
add_dependencies(api core)
target_link_libraries(api core)

##################################################################
###### liquid
##################################################################

file(GLOB_RECURSE LIQUID_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/liquid/*.cpp)
file(GLOB_RECURSE LIQUID_HDR_FILES 
  ${CMAKE_CURRENT_SOURCE_DIR}/include/dex/liquid/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/liquid/*.h
)
add_library(liquid STATIC ${LIQUID_HDR_FILES} ${LIQUID_SRC_FILES})
add_dependencies(liquid core)
target_link_libraries(liquid core)

##################################################################
###### processor
##################################################################

file(GLOB_RECURSE PROCESSOR_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/processor/*.cpp)
file(GLOB_RECURSE PROCESSOR_HDR_FILES 
  ${CMAKE_CURRENT_SOURCE_DIR}/include/dex/processor/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/processor/*.h
)

add_library(processor STATIC ${PROCESSOR_HDR_FILES} ${PROCESSOR_SRC_FILES})
add_dependencies(processor api)
target_link_libraries(processor api)
add_dependencies(processor liquid)
target_link_libraries(processor liquid)

##################################################################
###### dex
##################################################################

add_executable(dex "include/dex/dex.h" "src/dex.cpp" "src/main.cpp")
target_include_directories(dex PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_dependencies(dex processor)
target_link_libraries(dex processor)

#file(COPY "profiles" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(TARGET dex POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/profiles $<TARGET_FILE_DIR:dex>/profiles)

##################################################################
###### tests
##################################################################

add_subdirectory(tests)
