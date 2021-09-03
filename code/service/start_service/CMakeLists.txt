# set the project name
project(start_service)

file(REMOVE FtpDown.h 
	FtpDown.cpp
	HunqingServer.h
	HunqingServer.cpp
	PerfMon.h
	PerfMon.cpp)
file(GLOB SOURCES "*.cpp")

# add the executable
add_executable(start_service 
	${SOURCES}
	../Common/netCommon.cpp)

target_link_libraries(start_service PRIVATE net)

target_include_directories(start_service PUBLIC 
	${PROJECT_SOURCE_DIR})
