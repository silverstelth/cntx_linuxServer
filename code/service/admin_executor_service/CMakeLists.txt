cmake_minimum_required(VERSION 3.10)

# set the project name
project(admin_executor_service)

# add the executable
add_executable(admin_executor_service 
	admin_executor_service.cpp
	log_report.cpp
	../../common/commonForAdmin.cpp)

target_link_libraries(admin_executor_service PRIVATE net)

target_include_directories(admin_executor_service PUBLIC 
	${PROJECT_SOURCE_DIR})
