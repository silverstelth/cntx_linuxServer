# set the project name
project(welcome_service)

file(REMOVE welcome_service_itf.cpp)
file(GLOB SOURCES "*.cpp")

# add the executable
add_executable(welcome_service 
	${SOURCES}
	../Common/netCommon.cpp)

target_link_libraries(welcome_service PRIVATE net)

target_include_directories(welcome_service PUBLIC 
	${PROJECT_SOURCE_DIR})
