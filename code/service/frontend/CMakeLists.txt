# set the project name
project(frontend)

file(GLOB SOURCES "*.cpp")

# add the executable
add_executable(frontend 
	${SOURCES}
	../Common/netCommon.cpp)

target_link_libraries(frontend PRIVATE net)

target_include_directories(frontend PUBLIC 
	${PROJECT_SOURCE_DIR})
