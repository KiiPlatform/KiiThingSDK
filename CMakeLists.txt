PROJECT(KiiThingSDK)
INCLUDE(CPack)
INCLUDE(ExternalProject)
INCLUDE(cmake_utils/FindJansson.cmake)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8 FATAL_ERROR)

FIND_PACKAGE(CURL)
MESSAGE(STATUS "Jansson libraries found at: ${JANSSON_LIBRARIES}")
MESSAGE(STATUS "JANSSON includes found at: ${JANSSON_INCLUDE_DIRS}")
IF(CURL_FOUND)
  MESSAGE(STATUS "Curl libraries found at: ${CURL_LIBRARIES}")
  MESSAGE(STATUS "Curl includes found at: ${CURL_INCLUDE_DIRS}")
else()
  MESSAGE(SEND_ERROR "Could not find cURL on your system")
ENDIF(CURL_FOUND)
find_path(CURL_INCLUDE_DIR
               NAMES curl.h
               ONLY_CMAKE_FIND_ROOT_PATH)

include_directories(${CURL_INCLUDE_DIR})

SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configs" FORCE)
IF(DEFINED CMAKE_BUILD_TYPE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES} )
ENDIF()

IF(NOT CMAKE_BUILD_TYPE )
   SET( CMAKE_BUILD_TYPE "Debug" )
ENDIF()

#############################################################################################
# Create a folder and mount via sshfs the raspberry partition in order to copy binary to rpi with
# make install. If you don't want this feature, comment or delete lines: 16-22
IF(NOT EXISTS remote-folder AND EXISTS ${CMAKE_TOOLCHAIN_FILE})
	MESSAGE("Creating remote-folder for mounting raspberry pi partition with sshfs")
	execute_process(COMMAND mkdir ${PROJECT_SOURCE_DIR}/remote-folder
					COMMAND sshfs pi@rpi:/home/pi ${PROJECT_SOURCE_DIR}/remote-folder
					WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
ENDIF()
SET(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/remote-folder)
#############################################################################################

OPTION(WARNINGS_ARE_ERRORS	"Treat warnings as errors"										OFF)
OPTION(WARNINGS_ANSI_ISO    "Issue all the mandatory diagnostics Listed in C standard"		ON)
OPTION(ENABLE_PROFILING     "Enable profiling in Valgrind (Add flags: -g -fno_inline)"		OFF)

INCLUDE(cmake_utils/compileroptions.cmake REQUIRED)

ADD_SUBDIRECTORY(KiiThingSDK)

MESSAGE( STATUS "C++ flags :       ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
MESSAGE( STATUS "WARNINGS_ANSI_ISO =            ${WARNINGS_ANSI_ISO}" )
MESSAGE( STATUS "WARNINGS_ARE_ERRORS =          ${WARNINGS_ARE_ERRORS}" )
MESSAGE( STATUS "ENABLE_PROFILING =             ${ENABLE_PROFILING}" )
MESSAGE( STATUS "CMAKE_INSTALL_PREFIX =         ${CMAKE_INSTALL_PREFIX}" )
MESSAGE( STATUS "CMAKE_TOOLCHAIN_FILE =         ${CMAKE_TOOLCHAIN_FILE}" )
