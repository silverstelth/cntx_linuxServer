# set the project name
project(login_service)

file(REMOVE connection_web.cpp)
file(GLOB SOURCES "*.cpp")

# add the executable
add_executable(login_service 
	${SOURCES}
	../Common/Lang.cpp
	../Common/ExcelXML.cpp
	../Common/netCommon.cpp
	../Common/ConfigFile.cpp)

target_include_directories(login_service PUBLIC 
	${PROJECT_SOURCE_DIR})

target_link_libraries(login_service 
	net)

