cmake_minimum_required(VERSION 2.8.0)

project(CommandLineTools)

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 02)
set(${PROJECT_NAME}_PATCH_VERSION 00)
set(${PROJECT_NAME}_VERSION
  ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})
set(${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION})
 
FIND_PACKAGE(MtcaMappedDevice 00.11 REQUIRED)
include_directories(${MtcaMappedDevice_INCLUDE_DIRS})

#use -DCMAKE_BUILD_TYPE=Debug then configuring to turn on the coverage tests (and debug symbols)

#set(CMAKE_CXX_FLAGS "-Wall -g -fPIC -Wextra -Wshadow -Weffc++ -pedantic -Wuninitialized")
#skip -Weffc++ for the moment. It gives too many false positives
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -Wextra -Wshadow -pedantic -Wuninitialized")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 --coverage")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

#enable C++11 support. Unfortunately the flag is c++0x in gcc4.6, which we still have to support which makes
#it a bit lengthy
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()


include_directories(${CMAKE_SOURCE_DIR}/include)
#put the version number into the version file and add the include directory in the build tree, where we put it
configure_file(cmake/version.h.in
  "${PROJECT_BINARY_DIR}/include/version.h" @ONLY)
include_directories(${PROJECT_BINARY_DIR}/include)

add_executable(mtca4u ${CMAKE_SOURCE_DIR}/src/mtca4u_cmd.cpp)
target_link_libraries(mtca4u ${MtcaMappedDevice_LIBRARIES})    

#change the install prefix to the source directory in case the user has not specified a destination 
#i. e. CMAKE_INSTALL_PREFIX is not set manually
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR} CACHE PATH "Install directory
                 prefix" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

#Install the library and the executables
install(TARGETS mtca4u
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  )

ENABLE_TESTING()

#############################################################################
# MACRO Defenitions:
#############################################################################

# This macro is currently written to copy files with extensions from the
# provided source directories 
# into the project build directory. The source directories can be specified as a
# ; seperated list. The macro currently filters out editor back up files (eg.
# abc~.txt. More formats that need to be filtered may be added by modifying the
# globbiong expression
MACRO (COPY_CONTENT_TO_BUILD_DIR directories)
    # Here we should iterate through the directories provided and copy all
    # contents to the ${PROJECT_BINARY_DIR}. PLUS take care not to select backup
    # files in the script folder?
    foreach( directory ${directories} )

        FILE( GLOB list_of_files_to_copy
            "${CMAKE_SOURCE_DIR}/${directory}/*.sh"
            "${CMAKE_SOURCE_DIR}/${directory}/*.py" 
            "${CMAKE_SOURCE_DIR}/${directory}/*.dmap" 
            "${CMAKE_SOURCE_DIR}/${directory}/*.map" 
            "${CMAKE_SOURCE_DIR}/${directory}/*.txt") 
            #"${CMAKE_SOURCE_DIR}/${directory}/*[!~].*" ) <- this was the
            # globbing expression  before. Had to not use this because it was
            # letting in vim's .swp file (another back up file). For now
            # explicitly defining extentions we allow
            COPY_FILES_TO_BUILD_DIRECTORY( "${list_of_files_to_copy}" )
    endforeach( directory )
ENDMACRO(COPY_CONTENT_TO_BUILD_DIR)

#
# Helper macro for COPY_CONTENT_TO_BUILD_DIR.
# The list_of_files is expected to be filenames with absolute paths each
# (entry) seperated by a ;i in case of multiple entries.
MACRO( COPY_FILES_TO_BUILD_DIRECTORY list_of_files )
    foreach( file ${list_of_files} )
        configure_file( ${file} ${PROJECT_BINARY_DIR} copyonly )
    endforeach( file )
ENDMACRO( COPY_FILES_TO_BUILD_DIRECTORY )

# TODO: Rework structure later
MACRO( ADD_SCRIPTS_AS_TESTS list_of_scripts )
    foreach( script_path ${list_of_scripts} )
        get_filename_component(script_name_with_extension ${script_path} NAME)
        get_filename_component(test_name ${script_path} NAME_WE)
        add_test( ${test_name}
            ${PROJECT_BINARY_DIR}/${script_name_with_extension} ) # TODO: Rework this
        # later to allow macro to accept script location as an argument
    endforeach( script_path )
ENDMACRO( ADD_SCRIPTS_AS_TESTS )

##################### END MACRO DEFENITIONS #####################################

COPY_CONTENT_TO_BUILD_DIR("tests/referenceTexts;tests/scripts;tests/dMapFiles")
# prepare a list with names of scripts
FILE(GLOB location_of_script_files 
    "${CMAKE_SOURCE_DIR}/tests/scripts/*.py"
    "${CMAKE_SOURCE_DIR}/tests/scripts/*.sh")
ADD_SCRIPTS_AS_TESTS( "${location_of_script_files}" )


#
# Enable code coverage when cmake build configured -DCMAKE_BUILD_TYPE=Debug.
# lcov gets run by the target defined in Makefile.coverage
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
  CONFIGURE_FILE(cmake/Makefile.coverage
    ${PROJECT_BINARY_DIR}/Makefile.coverage @ONLY)
  ADD_CUSTOM_TARGET(coverage
    make -f Makefile.coverage
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating test coverage documentation" VERBATIM
  )
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")


# The instructions to create a debian package are loaded from a separate file
include(${CMAKE_SOURCE_DIR}/cmake/prepare_debian_package.cmake)
