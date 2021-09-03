# set the project name
project(HisManager)

file(GLOB SOURCES "*.cpp")

# add the executable
add_executable(HisManager 
	${SOURCES}
	../Common/Lang.cpp
	../Common/ExcelXML.cpp
	../Common/ConfigFile.cpp)

target_include_directories(HisManager PUBLIC 
	${PROJECT_SOURCE_DIR})

target_link_libraries(HisManager 
	net)

