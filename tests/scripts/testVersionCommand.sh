#!/bin/bash

# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_versionCommand.txt"
expected_console_output="./referenceTexts/referenceVersionCommand.txt"

bash -c '$0 version > $1 2>&1' $mtca4u_executable $actual_console_output

if [ $? -ne 0 ]; then # will not be equal to 0 if abv bash command fails
	exit -1 # make the test case return a failure code if 
	        # above bash command fails
fi

diff $actual_console_output $expected_console_output