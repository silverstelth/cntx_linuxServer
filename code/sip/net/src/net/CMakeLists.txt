file(REMOVE net_manager.cpp)
file(REMOVE unified_network_mt.cpp)
file(GLOB SOURCES "*.cpp")

# dependencies
find_package(FreeTDS REQUIRED)
find_package(ODBC REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

check_symbol_exists(getifaddrs "sys/types.h;ifaddrs.h" SP_HAVE_IFADDRS)

add_subdirectory(../misc misc)
list(APPEND EXTRA_LIBS misc)

add_library(net STATIC ${SOURCES})

if (SP_HAVE_IFADDRS)
	target_compile_definitions(net PRIVATE "SP_HAVE_IFADDRS")
endif ()

target_include_directories(net
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(net
	misc
	dl
	Threads::Threads
	ODBC::ODBC
	crypto)
