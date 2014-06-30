GET_FILENAME_COMPONENT(DIR_RD_EXT_INT_ABSOLUTE ${DIR_RD_EXT_INT} ABSOLUTE)

SET(RD_EXT_INT_FILES
	${DIR_RD_EXT_INT}/ripdraw.h
	${DIR_RD_EXT_INT}/ripdraw.c
	${DIR_RD_EXT_INT}/ripdraw-spi.c
	${DIR_RD_EXT_INT}/ripdraw-serial.c
)
SOURCE_GROUP("extint" FILES ${RD_EXT_INT_FILES})

INCLUDE_DIRECTORIES(${DIR_RD_EXT_INT})

IF(WIN32)
LINK_DIRECTORIES("${DIR_RD_EXT_INT_ABSOLUTE}/windows")
LINK_LIBRARIES(ftd2xx)

ELSE()
IF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
LINK_DIRECTORIES("${DIR_RD_EXT_INT_ABSOLUTE}/linux/x64")
ELSE()
LINK_DIRECTORIES("${DIR_RD_EXT_INT_ABSOLUTE}/linux/x86")
ENDIF()
LINK_LIBRARIES(ftd2xx pthread dl)

ENDIF()