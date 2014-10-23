cmake_minimum_required(VERSION 2.8.0)

project(CommandLineTools)

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 00)
set(${PROJECT_NAME}_PATCH_VERSION 00)
set(${PROJECT_NAME}_VERSION
  ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})
set(${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION})
 
#use -DCMAKE_BUILD_TYPE=Debug then configuring to turn on the coverage tests (and debug symbols)

#set(CMAKE_CXX_FLAGS "-Wall -g -fPIC -Wextra -Wshadow -Weffc++ -pedantic -Wuninitialized")
#skip -Weffc++ for the moment. It gives too many false positives
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -fPIC -Wextra -Wshadow -pedantic -Wuninitialized -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 --coverage")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

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
set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include/)
#aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)

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

# The instructions to create a debian package are loaded from a separate file
#include(${CMAKE_SOURCE_DIR}/cmake/prepare_debian_package.cmake)
