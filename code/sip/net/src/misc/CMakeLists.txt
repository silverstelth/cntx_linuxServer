file(REMOVE win_displayer.cpp)
file(REMOVE string_stream.cpp)
file(GLOB SOURCES "*.cpp")
list(APPEND SOURCES 
	config_file/config_file.cpp
	config_file/cf_lexical.cpp
	config_file/cf_gramatical.cpp)

find_package(LibXml2 REQUIRED)
find_package(GTK2 2.10 REQUIRED gtk)

if (!GTK2_FOUND)
	message ("Please install gtk2 package(gtk2-devel-2.x.x.x86_64.rpm)")
endif ()

add_library(misc STATIC ${SOURCES})

target_include_directories(misc
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include
		${CMAKE_CURRENT_SOURCE_DIR}/config_file
		${CMAKE_INSTALL_OLDINCLUDEDIR}/libxml2
		${GTK2_INCLUDE_DIRS}
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(misc 
	${GTK2_LIBRARIES}
	LibXml2::LibXml2)
