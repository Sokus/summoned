cmake_minimum_required (VERSION 3.5)
project(summoned VERSION 0.1.0)

# set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules/" ${CMAKE_MODULE_PATH})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${PROJECT_NAME}")

option(DISTRIBUTE "Compile for distribution" OFF)

if(DISTRIBUTE)
	set(RESOURCE_PATH "./res")
else()
	set(RESOURCE_PATH "${PROJECT_SOURCE_DIR}/res")
endif()

configure_file(cmake/config.h.in config.h)

# file(GLOB BASE_SRC src/base/base_*.cpp)

add_executable(${PROJECT_NAME}
	src/main.c
	${BASE_SRC}
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		"${PROJECT_BINARY_DIR}"
		src
)

install(TARGETS ${PROJECT_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}")

# install(DIRECTORY res DESTINATION "${CMAKE_INSTALL_PREFIX}")