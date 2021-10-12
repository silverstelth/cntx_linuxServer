cmake_minimum_required(VERSION 3.10)

# set the project name
project(Tutorial)

# add the executable
add_executable(Tutorial main.cpp)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(EXTRA_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include")

find_package(ODBC REQUIRED)

target_include_directories(Tutorial PUBLIC
	${PROJECT_BINARY_DIR}
	${EXTRA_INCLUDE_DIRS}
)

target_link_libraries(Tutorial
	ODBC::ODBC
)
