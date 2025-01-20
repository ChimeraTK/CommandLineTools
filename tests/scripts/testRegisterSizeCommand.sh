#!/bin/bash

# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_RegisterSizeCommand.txt"
expected_console_output="./referenceTexts/referenceRegisterSizeCommand.txt"

mkdir -p /var/run/lock/mtcadummy
( flock 9 # lock for mtcadummys0

    bash -c '$0 register_size DUMMY1 "" WORD_FIRMWARE > $1 2>&1' $mtca4u_executable $actual_console_output

    if [ $? -ne 0 ]; then # will not be equal to 0 if abv bash command fails
	    exit -1 # make the test case return a failure code if
	            # above bash command fails
    fi

) 9>/var/run/lock/mtcadummy/mtcadummys0

( flock 9 # lock for mtcadummys1

    bash -c '$0 register_size DUMMY2 ADC AREA_DMA_VIA_DMA >> $1 2>&1' $mtca4u_executable $actual_console_output

    if [ $? -ne 0 ]; then # will not be equal to 0 if abv bash command fails
	    exit -1 # make the test case return a failure code if
	            # above bash command fails
    fi

    # check the case where the command is entered incorrectly
    bash -c '$0 register_size DUMMY2 AREA_DMA_VIA_DMA >> $1 2>&1' $mtca4u_executable $actual_console_output

    if [ $? -eq 0 ]; then # will not be equal to 0 if abv bash command fails
	    exit -1 # The test fails if the bash command does not return in error
    fi

) 9>/var/run/lock/mtcadummy/mtcadummys1

grep -v "gcda:Merge mismatch" $actual_console_output > ${actual_console_output}-filtered
diff ${actual_console_output}-filtered $expected_console_output
