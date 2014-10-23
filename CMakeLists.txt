cmake_minimum_required(VERSION 2.8.0)

project(CommandLineTools)

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 01)
set(${PROJECT_NAME}_PATCH_VERSION 00)
set(${PROJECT_NAME}_VERSION
  ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})
set(${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION})
 
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

FIND_PACKAGE(MtcaMappedDevice 00.11 REQUIRED)
include_directories(${MtcaMappedDevice_INCLUDE_DIRS})

#FIND_PACKAGE(Boost COMPONENTS unit_test_framework)
#IF(Boost_UNIT_TEST_FRAMEWORK_FOUND)
#  set(TESTING_IS_ENABLED "true")
#  ENABLE_TESTING()
#ELSE(Boost_UNIT_TEST_FRAMEWORK_FOUND)
#  message("Boost unit_test_framework not found, disabling testing")
#ENDIF(Boost_UNIT_TEST_FRAMEWORK_FOUND)

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

#copy the reference output texts needed for testing
FILE(GLOB REFERENCE_TEXTS ${CMAKE_SOURCE_DIR}/tests/referenceTexts/*.txt )
message("referenceTexts ${referenceTexts}")
foreach( REFERENCE_FILE_WITH_PATH ${REFERENCE_TEXTS} )
  get_filename_component( REFERENCE_FILE ${REFERENCE_FILE_WITH_PATH} NAME )
  configure_file( ${REFERENCE_FILE_WITH_PATH} ${PROJECT_BINARY_DIR}/${REFERENCE_FILE} copyonly)
endforeach( REFERENCE_FILE_WITH_PATH )

#add all tests
#Little hack: Each type of script has to be added separately.
#Without .sh it would also pick up backupfiles .sh~ etc.
FILE(GLOB SHELL_SCRIPTS ${CMAKE_SOURCE_DIR}/tests/scripts/*.sh )
FILE(GLOB PYTHON_SCRIPTS ${CMAKE_SOURCE_DIR}/tests/scripts/*.py )
FILE(GLOB TEST_SCRIPTS ${SHELL_SCRIPTS} ${PYTHON_SCRIPTS})
foreach( SCRIPT_WITH_PATH ${TEST_SCRIPTS} )
  get_filename_component( SCRIPT ${SCRIPT_WITH_PATH} NAME )
  configure_file( ${SCRIPT_WITH_PATH} ${PROJECT_BINARY_DIR}/${SCRIPT} copyonly)
  #strip the extension from the scripts name to use it as a test name
  #NAME_WE means the base name without path and (longest) extension
  get_filename_component( TEST_NAME ${SCRIPT} NAME_WE)
  add_test( ${TEST_NAME} ${PROJECT_BINARY_DIR}/${SCRIPT} WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
endforeach( SCRIPT_WITH_PATH )



# The instructions to create a debian package are loaded from a separate file
#include(${CMAKE_SOURCE_DIR}/cmake/prepare_debian_package.cmake)