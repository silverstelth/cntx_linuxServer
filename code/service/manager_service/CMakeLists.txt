# set the project name
project(manager)

file(GLOB SOURCES "*.cpp")

# add the executable
add_executable(manager 
	${SOURCES}
	../Common/Lang.cpp
	../Common/netCommon.cpp
	../Common/ExcelXML.cpp
	../openroom/mst_room.cpp
	../Common/ConfigFile.cpp)

target_link_libraries(manager PRIVATE net)

target_include_directories(manager PUBLIC 
	${PROJECT_SOURCE_DIR})
