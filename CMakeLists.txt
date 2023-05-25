cmake_minimum_required(VERSION 2.8.0)

project(CommandLineTools)

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 16)
set(${PROJECT_NAME}_PATCH_VERSION 01)

include(cmake/set_version_numbers.cmake)

include(cmake/add_dependency.cmake)

set(ChimeraTK-DeviceAccess_MIN_VERSION 03.00)

add_dependency(ChimeraTK-DeviceAccess ${ChimeraTK-DeviceAccess_MIN_VERSION} REQUIRED)
include_directories(${ChimeraTK-DeviceAccess_INCLUDE_DIRS})

include(cmake/set_default_build_to_release.cmake)
include(cmake/set_default_flags.cmake)

add_dependency(Boost COMPONENTS system filesystem REQUIRED)

# put the version number into the version file and add the include directory in the build tree, where we put it
configure_file(cmake/version.h.in
  "${PROJECT_BINARY_DIR}/include/version.h" @ONLY)

# update revferenceVersionCommand.txt (used for testing) with the current version
configure_file(cmake/referenceVersionCommand.txt.in
  "${PROJECT_BINARY_DIR}/referenceTexts/referenceVersionCommand.txt")
configure_file(cmake/referenceReadRegisterCommand.txt.in
  "${PROJECT_BINARY_DIR}/referenceTexts/referenceReadRegisterCommand.txt")

add_executable(mtca4u ${CMAKE_SOURCE_DIR}/src/mtca4u_cmd.cpp)
target_include_directories(mtca4u PRIVATE include ${PROJECT_BINARY_DIR}/include)
set_target_properties(mtca4u PROPERTIES VERSION ${${PROJECT_NAME}_SOVERSION})
set_target_properties(mtca4u PROPERTIES LINK_FLAGS "${ChimeraTK-DeviceAccess_LINK_FLAGS}")
set_target_properties(mtca4u PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(mtca4u ${ChimeraTK-DeviceAccess_LIBRARIES} ${Boost_LIBRARIES})

# change the install prefix to the source directory in case the user has not specified a destination
# i. e. CMAKE_INSTALL_PREFIX is not set manually
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR} CACHE PATH "Install directory
                 prefix" FORCE)
ENDIF()

# Install the library and the executables
install(TARGETS mtca4u
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
)

ENABLE_TESTING()

# ############################################################################
# MACRO Defenitions:
# ############################################################################
MACRO(COPY_CONTENT_TO_BUILD_DIR directories)
  foreach(directory ${directories})
    SETUP_CONTENT_IN_BUILD_DIRECTORY("${directory}")
  endforeach(directory)
ENDMACRO()

# For directory specified in the directory parameter, the macro creates
# a sub-directory in the project build directory. It populates this new
# sub-directory with the content of the source directory. Eg:
# <CMAKE_SOURCE_DIR>/a/b/source_dir as input creates
# <project_build_dir>/source_dir and will have .txt, .sh, .py, .dmap and .map
# files from the source directory.
MACRO(SETUP_CONTENT_IN_BUILD_DIRECTORY directory)
  get_filename_component(parent_directory ${directory} NAME) # Kind of a hack

  # as we are actually picking the directory name and not the filename.
  # (because ${directory} contains path to a directory and not a file)
  set(source_directory "${CMAKE_SOURCE_DIR}/${directory}")
  set(target_directory "${PROJECT_BINARY_DIR}/${parent_directory}")
  file(MAKE_DIRECTORY "${target_directory}")
  COPY_SOURCE_TO_TARGET(${source_directory} ${target_directory})
ENDMACRO()

# The macro currently filters out the editor back up files that end with ~ .
# The macro picks up only these specified formats from the
# source directory : .dmap, .map, .txt, .py, .sh. New formats formats may be added by
# modifying the globbing expression
MACRO(COPY_SOURCE_TO_TARGET source_directory target_directory)
  FILE(GLOB list_of_files_to_copy
    "${source_directory}/*[!~].sh" # <- filter out abc~.sh
    "${source_directory}/*[!~].py" # <- filter out abc~.py
    "${source_directory}/*[!~].dmap"
    "${source_directory}/*[!~].map"
    "${source_directory}/*[!~].txt")

  foreach(file ${list_of_files_to_copy})
    FILE(COPY ${file} DESTINATION ${target_directory})
  endforeach(file)
ENDMACRO()

MACRO(ADD_SCRIPTS_AS_TESTS list_of_script_files)
  foreach(script_path ${list_of_script_files})
    get_filename_component(test_name ${script_path} NAME_WE)
    add_test(${test_name} ${script_path})
  endforeach(script_path)
ENDMACRO()

# #################### END MACRO DEFENITIONS #####################################
COPY_CONTENT_TO_BUILD_DIR("tests/referenceTexts;tests/scripts")

# special files for dmap testing:
# No dmap in directory, just map file
FILE(COPY tests/dMapFiles/mtcadummy.map DESTINATION ${PROJECT_BINARY_DIR}/testNoDmapFile)

# Two dmap files. Copy everything, than the map file again
FILE(COPY tests/dMapFiles/ DESTINATION ${PROJECT_BINARY_DIR}/testTwoDmapFilesBroken)
configure_file(tests/dMapFiles/dummies.dmap ${PROJECT_BINARY_DIR}/testTwoDmapFilesBroken/second.dmap)
FILE(COPY tests/dMapFiles/ DESTINATION ${PROJECT_BINARY_DIR}/testTwoDmapFilesOk)
configure_file(tests/dMapFiles/dummies.dmap ${PROJECT_BINARY_DIR}/testTwoDmapFilesOk/CommandLineTools.dmap)

# Not copying the dmap files to its own sub directory, as the
# mtca4u  executable currently requires the dMap file to be present in the
# executable directory.  Might reconsider copying dMap content into its own
# subdirectory, once code can load a dMapfile from any location.
set(dmap_source_directory "${CMAKE_SOURCE_DIR}/tests/dMapFiles")
COPY_SOURCE_TO_TARGET(${dmap_source_directory} ${PROJECT_BINARY_DIR})

# prepare a list with names of scripts
FILE(GLOB location_of_script_files
  "${PROJECT_BINARY_DIR}/scripts/*.py"
  "${PROJECT_BINARY_DIR}/scripts/*.sh")
ADD_SCRIPTS_AS_TESTS("${location_of_script_files}")

include(cmake/enable_code_coverage_report.cmake)

# The instructions to create a debian package are loaded from a separate file
include(cmake/prepare_debian_package.cmake)
