#!/bin/bash


# command usage:
# 'mtca4u register_info <Board_name> <Module_name> <Register_name>'
# This prints out the map file informatoion for the register


# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_mtca4uRegInfo.txt"
expected_console_output="./referenceTexts/referenceMtca4uRegInfo.txt"



# Get Information from a card that has no modules described in its mapfile
# DUMMY1 which uses mapfile ./mtcadummywithoutModules.map is such a board
# in case of such devices the module name parameter is represented by 
# an empty string "" (<- indicating that register is not part of any module)
bash -c '$0 register_info DUMMY1 ""  WORD_FIRMWARE > $1  2>&1' $mtca4u_executable $actual_console_output #<- This is actually a brittle thing find a better way to do things
if [ $? -ne 0 ] ; then # 0 is the exit status for a successful command
    exit -1
fi 


# log  displayed information for a register belonging to a module. DUMMY2 is a
# card that has modules in it (it is described by the mapfile mtcadummy.map)
bash -c '$0 register_info DUMMY2 ADC  AREA_DMAABLE_FIXEDPOINT16_3 >> $1  2>&1' $mtca4u_executable $actual_console_output
if [ $? -ne 0 ] ; then # 0 is the exit status for a successful command
    exit -1
fi 

# log display for invalid command syntax

mtca4u_command="$mtca4u_executable register_info DUMMY1"
$mtca4u_command >> $actual_console_output  2>&1 # redirect
# This command is not successful; Error message gets printed to stderr, and
# consequently return type is not 0
if [ $? -ne 1 ] ; then 
    exit -1
fi 

diff $actual_console_output $expected_console_output

