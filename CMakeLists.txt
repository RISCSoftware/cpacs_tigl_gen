# CPACSGen cmake project
# author: Bernhard Manfred Gruber

cmake_minimum_required(VERSION 3.2)

project(CPACSGen)

# cache variables
set(BOOST_ROOT                                          "" CACHE PATH "Boost installation prefix")
set(TIXI_PATH                                           "" CACHE PATH "Tixi installation prefix")
set(GENERATOR_OUTPUT_DIR                                "" CACHE PATH "Output directory for the generated CPACS classes (Tigl src/generated directory)")
set(GENERATOR_INPUT_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/input CACHE PATH "Generator input files (CPACS schema, configuration tables ...)")

# find boost
set(Boost_USE_STATIC_LIBS ON)
find_package (Boost 1.55.0 REQUIRED COMPONENTS system filesystem)
mark_as_advanced(CLEAR BOOST_ROOT) # find module for boost marks this variable as advanced

# find tixi
set(CMAKE_PREFIX_PATH "${TIXI_PATH};${CMAKE_PREFIX_PATH}")
find_package(TIXI 2.1.0 REQUIRED)
if(TIXI_VERSION VERSION_GREATER 2.1.0)
  set(HAVE_TIXI_SETPRINTMSG TRUE)
endif()

# source files
file(GLOB ALL_SRCS src/*.cpp)
file(GLOB ALL_HEADERS src/*.h*)
file(GLOB ALL_INPUTS input/*.*)

#includes
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${TIXI_INCLUDE_DIRS})

# exe
add_executable(${PROJECT_NAME} ${ALL_SRCS} ${ALL_HEADERS} ${ALL_INPUTS})

# library dependencies
message("${Boost_LIBRARIES}")
message("${Boost_INCLUDE_DIRS}")
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${TIXI_LIBRARIES})

source_group(" " FILES ${ALL_SRCS} ${ALL_HEADERS})
source_group("inputs" FILES ${ALL_INPUTS})

# add custom target step to generate files side by side to TIGL source
add_custom_target(
	generate
	COMMAND ${PROJECT_NAME} ${GENERATOR_INPUT_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src ${GENERATOR_OUTPUT_DIR}
	COMMENT Running CPACSGen
	VERBATIM
)

# linker flags
if (NOT WIN32)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif (NOT WIN32)
