CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
# for cmake debug
SET(CMAKE_VERBOSE_MAKEFILE OFF)
# project
PROJECT(root_file_name_project)

#################################################################
INCLUDE(../common.cmake)

#################################################################
SET(DIR_RD_root_file_name ./1_root_file_name/)
FILE(GLOB RD_root_file_name_FILES
	${DIR_RD_root_file_name}/*.h
	${DIR_RD_root_file_name}/*.c
	${DIR_RD_root_file_name}/*.cpp
)
SOURCE_GROUP("root_file_name" FILES ${RD_root_file_name_FILES})

#################################################################
# EXECUTABLE
ADD_EXECUTABLE(
	root_file_name
	${RD_root_file_name_FILES} ${RD_EXT_INT_FILES}
)

