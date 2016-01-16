cmake_minimum_required(VERSION 2.8.0)

project(CommandLineTools)

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 08)
set(${PROJECT_NAME}_PATCH_VERSION 01)
set(${PROJECT_NAME}_VERSION
  ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})
set(${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION})
 
set(mtca4u-deviceaccess_MIN_VERSION 00.03)
FIND_PACKAGE(mtca4u-deviceaccess ${mtca4u-deviceaccess_MIN_VERSION} REQUIRED)
include_directories(${mtca4u-deviceaccess_INCLUDE_DIRS})

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

FIND_PACKAGE(Boost COMPONENTS system filesystem)
#The boost header-only development files are required
FIND_PACKAGE(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(${CMAKE_SOURCE_DIR}/include)
#put the version number into the version file and add the include directory in the build tree, where we put it
configure_file(cmake/version.h.in
  "${PROJECT_BINARY_DIR}/include/version.h" @ONLY)
include_directories(${PROJECT_BINARY_DIR}/include)

# update revferenceVersionCommand.txt (used for testing) with the current version
configure_file(cmake/referenceVersionCommand.txt.in
  "${PROJECT_BINARY_DIR}/referenceTexts/referenceVersionCommand.txt")



add_executable(mtca4u ${CMAKE_SOURCE_DIR}/src/mtca4u_cmd.cpp)
target_link_libraries(mtca4u ${mtca4u-deviceaccess_LIBRARIES} ${Boost_LIBRARIES})    

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

MACRO (COPY_CONTENT_TO_BUILD_DIR directories)
    foreach( directory ${directories} )
        SETUP_CONTENT_IN_BUILD_DIRECTORY( "${directory}" )
    endforeach( directory )
ENDMACRO(COPY_CONTENT_TO_BUILD_DIR)



# For directory specified in the directory parameter, the macro creates
#  a sub-directory in the project build directory. It populates this new
# sub-directory with the content of the source directory. Eg:
# <CMAKE_SOURCE_DIR>/a/b/source_dir as input creates
# <project_build_dir>/source_dir and will have .txt, .sh, .py, .dmap and .map
# files from the source directory. 
MACRO( SETUP_CONTENT_IN_BUILD_DIRECTORY directory )
    get_filename_component(parent_directory ${directory} NAME) # Kind of a hack
    # as we are actually picking the directory name and not the filename.
    # (because ${directory} contains path to a directory and not a file)
    set(source_directory "${CMAKE_SOURCE_DIR}/${directory}" )
    set(target_directory "${PROJECT_BINARY_DIR}/${parent_directory}")
    file( MAKE_DIRECTORY "${target_directory}" )
    COPY_SOURCE_TO_TARGET( ${source_directory} ${target_directory} )
ENDMACRO( SETUP_CONTENT_IN_BUILD_DIRECTORY )


# The macro currently filters out the editor back up files that end with ~ .
# The macro picks up only these specified formats from the
# source directory : .dmap, .map, .txt, .py, .sh. New formats formats may be added by 
# modifying the globbing expression
MACRO( COPY_SOURCE_TO_TARGET source_directory target_directory)
    FILE( GLOB list_of_files_to_copy
        "${source_directory}/*[!~].sh" # <- filter out abc~.sh
        "${source_directory}/*[!~].py" # <- filter out abc~.py
        "${source_directory}/*[!~].dmap" 
        "${source_directory}/*[!~].map" 
        "${source_directory}/*[!~].txt") 
    foreach( file ${list_of_files_to_copy} )
        configure_file( ${file} ${target_directory} copyonly )
    endforeach( file )
ENDMACRO( COPY_SOURCE_TO_TARGET )

MACRO( ADD_SCRIPTS_AS_TESTS list_of_script_files )
    foreach( script_path ${list_of_script_files} )
        get_filename_component(test_name ${script_path} NAME_WE)
        add_test( ${test_name} ${script_path} )
    endforeach( script_path )
ENDMACRO( ADD_SCRIPTS_AS_TESTS )

##################### END MACRO DEFENITIONS #####################################

COPY_CONTENT_TO_BUILD_DIR("tests/referenceTexts;tests/scripts")

# Not copying the dmap files to its own sub directory, as the
# mtca4u  executable currently requires the dMap file to be present in the
# executable directory.  Might reconsider copying dMap content into its own
# subdirectory, once code can load a dMapfile from any location. 
set(dmap_source_directory "${CMAKE_SOURCE_DIR}/tests/dMapFiles" ) 
COPY_SOURCE_TO_TARGET(${dmap_source_directory} ${PROJECT_BINARY_DIR})


# prepare a list with names of scripts
FILE(GLOB location_of_script_files 
    "${PROJECT_BINARY_DIR}/scripts/*.py"
    "${PROJECT_BINARY_DIR}/scripts/*.sh")
ADD_SCRIPTS_AS_TESTS( "${location_of_script_files}" )


#
# Enable code coverage when cmake build configured -DCMAKE_BUILD_TYPE=Debug.
# lcov gets run by the target defined in Makefile.coverage
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
  CONFIGURE_FILE(cmake/Makefile.coverage.in
    ${PROJECT_BINARY_DIR}/Makefile.coverage @ONLY)
  ADD_CUSTOM_TARGET(coverage
    make -f Makefile.coverage
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating test coverage documentation" VERBATIM
  )
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")


# The instructions to create a debian package are loaded from a separate file
include(${CMAKE_SOURCE_DIR}/cmake/prepare_debian_package.cmake)
