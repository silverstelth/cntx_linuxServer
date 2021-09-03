# set the project name
project(LogService)

file(GLOB SOURCES "*.cpp")

# add the executable
add_executable(LogService
	${SOURCES})

target_include_directories(LogService PUBLIC 
	${PROJECT_SOURCE_DIR})

target_link_libraries(LogService
	net)

