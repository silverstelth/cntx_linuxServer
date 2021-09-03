# set the project name
project(naming_service)

file(GLOB SOURCES "*.cpp")

# add the executable
add_executable(naming_service 
	${SOURCES})

target_link_libraries(naming_service PRIVATE net)

target_include_directories(naming_service PUBLIC 
	${PROJECT_SOURCE_DIR})
