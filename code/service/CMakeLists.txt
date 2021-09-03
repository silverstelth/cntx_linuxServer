cmake_minimum_required(VERSION 3.10)

# set the project name
project(service VERSION 1.0)

# define the OS for Unix
set(UNIX_PLATFORM UNIX_CENTOS)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake )

# configurate the gnu install directory
include(GNUInstallDirs)
include(CheckCXXSymbolExists)

# specify a compiler
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0X" COMPILER_SUPPORTS_CXX0X)
if (COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
	message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

add_subdirectory(../sip/net/src/net net)
# add_subdirectory(../sip/plugins/sipstream/Codec/Source SipCodec)

add_subdirectory(Admin_Main_service 		01_admin_service)
add_subdirectory(admin_executor_service 	02_admin_executor_service)
add_subdirectory(start_service 				03_start_service)
add_subdirectory(login_service 				04_login_service)
add_subdirectory(naming_service 			05_naming_service)
add_subdirectory(welcome_service 			06_welcome_service)
add_subdirectory(frontend 					07_frontend_service)
add_subdirectory(openroom 					08_openroom_service)
add_subdirectory(RelayService 				09_relay_service)
add_subdirectory(HisManager_service			10_history_service)
# add_subdirectory(../sip/plugins/sipstream/StreamService			11_stream_service)
add_subdirectory(manager_service 			12_manager_service)
add_subdirectory(LogService 				20_log_service)
