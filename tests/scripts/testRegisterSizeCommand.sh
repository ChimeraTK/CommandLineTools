#!/bin/bash

# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_RegisterSizeCommand.txt"
expected_console_output="./referenceTexts/referenceRegisterSizeCommand.txt"

bash -c '$0 register_size DUMMY1 "" WORD_FIRMWARE > $1 2>&1' $mtca4u_executable $actual_console_output

if [ $? -ne 0 ]; then # will not be equal to 0 if abv bash command fails
	exit -1 # make the test case return a failure code if 
	        # above bash command fails
fi

bash -c '$0 register_size DUMMY2 ADC AREA_DMA_VIA_DMA >> $1 2>&1' $mtca4u_executable $actual_console_output

if [ $? -ne 0 ]; then # will not be equal to 0 if abv bash command fails
	exit -1 # make the test case return a failure code if 
	        # above bash command fails
fi

# check the case where the command is entered incorrectly
bash -c '$0 register_size DUMMY2 AREA_DMA_VIA_DMA >> $1 2>&1' $mtca4u_executable $actual_console_output

if [ $? -e 0 ]; then # will not be equal to 0 if abv bash command fails
	exit -1 # The test fails if the bash command does not return in error
fi


diff $actual_console_output $expected_console_output