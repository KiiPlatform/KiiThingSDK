PROJECT(KiiThingSDK)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
if (NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()
add_definitions(-D int32_t=__int32_t)

FILE(GLOB KiiThingSDK_files *.c *.h)
FILE(GLOB Jansson_files ${CMAKE_CURRENT_SOURCE_DIR}/jansson/*.h ${CMAKE_CURRENT_SOURCE_DIR}/jansson/*.c )
SET(Jansson_src
    ${Jansson_files}
    )
add_library(Jansson STATIC ${Jansson_src})

SET(KiiThingSDK_src
    ${KiiThingSDK_files}
    )

ADD_LIBRARY(kii STATIC ${KiiThingSDK_src})

TARGET_LINK_LIBRARIES(kii ${CURL_LIBRARIES}
${Jansson})