#!/bin/bash

# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_mtca4uInfo.txt"
expected_console_output="./referenceTexts/referenceMtca4uInfo.txt"

mtca4u_command="$mtca4u_executable info"

$mtca4u_command > $actual_console_output  2>&1 # redirect
                                               # both stderr and stdout to file 
if [ $? -ne 0 ] ; then 
    exit -1
fi 

diff $actual_console_output $expected_console_output

