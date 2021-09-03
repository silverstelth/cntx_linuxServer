# set the project name
project(openroom)

file(REMOVE HuangdiRoom.cpp
	KongziRoom.cpp
	IndivEvent.cpp)
file(GLOB SOURCES "*.cpp")

# add the executable
add_executable(openroom 
	${SOURCES}
	../Common/Lang.cpp
	../Common/ExcelXML.cpp
	../Common/netCommon.cpp
	../Common/ConfigFile.cpp)

target_link_libraries(openroom PRIVATE net)

target_include_directories(openroom PUBLIC 
	${PROJECT_SOURCE_DIR})
