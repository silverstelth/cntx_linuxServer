# set the project name
project(relay_service)

file(GLOB SOURCES "*.cpp")

# add the executable
add_executable(relay_service 
	${SOURCES})

target_link_libraries(relay_service PRIVATE net)

target_include_directories(relay_service PUBLIC 
	${PROJECT_SOURCE_DIR})
