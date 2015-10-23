cmake_minimum_required (VERSION 2.8)

project (LMBd)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/dist)
INCLUDE(CheckLSBTypes)


option(LMB_DEBUG "Enable additional debugging information" OFF)

if(LMB_DEBUG)
    set(CMAKE_VERBOSE_MAKEFILE true)
    add_definitions(-DLMB_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
else()
    set(CMAKE_BUILD_TYPE Release)
endif(LMB_DEBUG)

add_executable(LMBd 
	src/Driver-DX.cpp 
	src/FileMonitor.cpp 
	src/lmbcli.cpp 
	src/lmbd.cpp
	src/serial/src/serial.cc 
	src/serial/src/impl/unix.cc
	src/serial/src/impl/list_ports/list_ports_linux.cc 
	src/libcli/libcli.c
	src/LoggerCpp/Config.cpp
 	src/LoggerCpp/DateTime.cpp
 	src/LoggerCpp/Log.cpp
 	src/LoggerCpp/Logger.cpp
 	src/LoggerCpp/Manager.cpp
 	src/LoggerCpp/OutputConsole.cpp
 	src/LoggerCpp/OutputDebug.cpp
 	src/LoggerCpp/OutputFile.cpp)

if (LSB_DISTRIBUTOR_ID STREQUAL "centos")
if (LSB_RELEASE STREQUAL "6")
message("Enabling CentOS 6 workaround for Boost")
set(Boost_NO_BOOST_CMAKE ON)
endif()
endif()


find_package(Boost REQUIRED COMPONENTS thread filesystem program_options)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(LMBd -pthread ${Boost_LIBRARIES})
endif()

target_link_libraries(LMBd -lcrypt)

include_directories(src src/serial/include/)
add_definitions (-std=c++0x)  # -std=c++11

#file(STRINGS FL_HEADERS fl-headers)
#file(STRINGS FL_SOURCES fl-sources)



set_target_properties(LMBd PROPERTIES OUTPUT_NAME LMBd)

include(GNUInstallDirs)

install(TARGETS LMBd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES config/LMBd.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})
install(FILES scripts/zabbix_stats.cfg DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})
install(FILES scripts/zabbix_stats.py DESTINATION ${CMAKE_INSTALL_BINDIR})


