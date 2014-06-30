#################################################################
SET(DIR_RD_EXT_INT ../extint/)
INCLUDE(../extint/extint.cmake)

#################################################################
# Additional deps
IF(WIN32)
	SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
 
ELSE()
ENDIF()

IF(WIN32)

ELSE()

ENDIF()
