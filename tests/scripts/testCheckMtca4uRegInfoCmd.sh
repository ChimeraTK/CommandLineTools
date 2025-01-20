#!/bin/bash -e


# command usage:
# 'mtca4u register_info <Board_name> <Module_name> <Register_name>'
# This prints out the map file informatoion for the register


# NOTE: Paths specified below, assume the working directory is the build directory
mtca4u_executable=./mtca4u
actual_console_output="./output_mtca4uRegInfo.txt"
expected_console_output="./referenceTexts/referenceMtca4uRegInfo.txt"

{

  mkdir -p /var/run/lock/mtcadummy
  ( flock 9 # lock for mtcadummys0

    # Get Information from a card that has no modules described in its mapfile
    # DUMMY1 which uses mapfile ./mtcadummywithoutModules.map is such a board
    # in case of such devices the module name parameter is represented by
    # an empty string "" (<- indicating that register is not part of any module)
    $mtca4u_executable register_info DUMMY1 "" WORD_FIRMWARE

  ) 9>/var/run/lock/mtcadummy/mtcadummys0


  ( flock 9 # lock for mtcadummys1

    # log  displayed information for a register belonging to a module. DUMMY2 is a
    # card that has modules in it (it is described by the mapfile mtcadummy.map)
    $mtca4u_executable register_info DUMMY2 ADC AREA_DMAABLE_FIXEDPOINT16_3

  ) 9>/var/run/lock/mtcadummy/mtcadummys1


  # log display for invalid command syntax ("!" is used to invert the return code)
  ! $mtca4u_executable register_info DUMMY1

} &> $actual_console_output

grep -v "gcda:Merge mismatch" $actual_console_output > ${actual_console_output}-filtered
diff ${actual_console_output}-filtered $expected_console_output
