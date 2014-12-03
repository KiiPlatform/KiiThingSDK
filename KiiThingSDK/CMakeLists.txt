PROJECT(KiiThingSDK)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
if (NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

FILE(GLOB KiiThingSDK_files *.c *.h)
add_custom_target(
   Jansson
   COMMAND make
   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jansson
)

SET(KiiThingSDK_src
    ${KiiThingSDK_files}
    )

ADD_LIBRARY(kii STATIC ${KiiThingSDK_src})

TARGET_LINK_LIBRARIES(kii ${CURL_LIBRARIES}
${Jansson})
add_dependencies(kii ${CURL_LIBRARIES}
${Jansson})