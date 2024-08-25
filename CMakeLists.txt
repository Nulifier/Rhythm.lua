cmake_minimum_required(VERSION 3.22)

project(rhythm VERSION 1.0.0
                       DESCRIPTION "Lua library for regularly scheduling tasks"
					   LANGUAGES CXX)

option(RHYTHM_SCHEDULER_METRICS "Enable metrics collection in the scheduler" ON)

if(CMAKE_BUILD_TYPE STREQUAL Release)
	set(RHYTHM_STACK_CHECK OFF)
else()
	set(RHYTHM_STACK_CHECK ON)
endif()

# Only do these if this is the main project
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	# Use folders in IDEs
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	include(CTest)

	find_package(Doxygen)
	if (DOXYGEN_FOUND)
		add_subdirectory(docs)
	else()
		message(STATUS "Doxygen not found, documentation target will not be available")
	endif()
endif()

find_package (Lua 5.1 REQUIRED)

set(INCLUDES
	inc/lua-rhythm.h
	src/lua-rhythm-private.hpp
	src/scheduler.hpp
	src/chrono-utils.hpp
)

set(SOURCES
	src/lua-rhythm.cpp
	src/scheduler.cpp
)

configure_file(
	src/rhythm-config.hpp.in
	${CMAKE_CURRENT_BINARY_DIR}/inc/rhythm-config.hpp
)

add_library(rhythm SHARED
	${INCLUDES}
	${SOURCES}
)

target_compile_features(rhythm PRIVATE cxx_std_17)
set_target_properties(rhythm PROPERTIES
	CXX_EXTENSIONS OFF
	PREFIX ""  # No 'lib' prefix on Unix-like systems
)

# Generate export header
include(GenerateExportHeader)
generate_export_header(rhythm
	BASE_NAME lua_rhythm
	EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/inc/lua-rhythm-export.h
)

# Enable Link Time Optimization if supported
include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if(result)
	set_target_properties(rhythm PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_include_directories(rhythm PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/inc>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
	$<INSTALL_INTERFACE:include>

	${LUA_INCLUDE_DIR}
)

target_link_libraries(rhythm PRIVATE
	${LUA_LIBRARIES}
)

# if ((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR MODERN_CMAKE_BUILD_TESTING) AND BUILD_TESTING)
# 	add_subdirectory(tests)
# endif()
