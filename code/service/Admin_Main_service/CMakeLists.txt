# set the project name
project(admin_main_service)

file(GLOB SOURCES "*.cpp")

# add the executable
add_executable(admin_main_service 
	${SOURCES}
	../../common/commonForAdmin.cpp)

target_link_libraries(admin_main_service PRIVATE net)

target_include_directories(admin_main_service PUBLIC 
	${PROJECT_SOURCE_DIR})
