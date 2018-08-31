# CPACSGen cmake project
# author: Bernhard Manfred Gruber

cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(CPACSGen)

# cache variables
set(BOOST_ROOT "" CACHE PATH "Boost installation prefix")
set(TIXI_PATH  "" CACHE PATH "Tixi installation prefix")
set(TIGL_PATH  "" CACHE PATH "Tigl installation prefix")

# find boost
set(Boost_USE_STATIC_LIBS ON CACHE BOOL "Find static or dynamic libs. Static libs require lib prefix.")
find_package (Boost REQUIRED COMPONENTS system filesystem date_time regex)
mark_as_advanced(CLEAR BOOST_ROOT) # find module for boost marks these variables as advanced
mark_as_advanced(CLEAR Boost_USE_STATIC_LIBS)

# find tixi
set(CMAKE_PREFIX_PATH "${TIXI_PATH};${CMAKE_PREFIX_PATH}")
find_package(TIXI REQUIRED)

# input files and exe
file(GLOB ALL_INPUTS src/*.cpp *src/*.h *.md)
add_executable(${PROJECT_NAME} ${ALL_INPUTS})
source_group(" " FILES ${ALL_INPUTS})

# compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
	-DCPACS_GEN
)

# includes
target_include_directories(${PROJECT_NAME} PRIVATE
	${Boost_INCLUDE_DIRS}
	${TIXI_INCLUDE_DIRS}
)

# library dependencies
target_link_libraries(${PROJECT_NAME}
	${Boost_LIBRARIES}
	${TIXI_LIBRARIES}
)

if(TIGL_PATH)
	set(GENERATOR_INPUT_DIR  ${TIGL_PATH}/cpacs_gen_input CACHE PATH "Generator input files (Tigl cpacs_gen_input directory)")
	set(GENERATOR_OUTPUT_DIR ${TIGL_PATH}/src/generated   CACHE PATH "Output directory for the generated CPACS classes (Tigl src/generated directory)")

	# add custom target to generate files side by side to TIGL source
	add_custom_target(
		generate
		COMMAND ${PROJECT_NAME} ${GENERATOR_INPUT_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src ${GENERATOR_OUTPUT_DIR}
		COMMENT Running CPACSGen
		VERBATIM
	)
else()
	message(WARNING "TIGL_PATH is not specified. Target generate will not be created")
endif()

# linker flags
if (NOT WIN32)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif (NOT WIN32)

# copy shared library to ouput directory
if (WIN32)
	get_target_property(TIXI_SHARED_LIBRARY tixi3 LOCATION)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${TIXI_SHARED_LIBRARY}" "${CMAKE_BINARY_DIR}/$<CONFIG>/")
endif (WIN32)
