#!/bin/bash -e

# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_RegisterSizeCommand.txt"
expected_console_output="./referenceTexts/referenceRegisterSizeCommand.txt"

{

    mkdir -p /var/run/lock/mtcadummy
    ( flock 9 # lock for mtcadummys0

        $mtca4u_executable register_size DUMMY1 "" WORD_FIRMWARE

    ) 9>/var/run/lock/mtcadummy/mtcadummys0

    ( flock 9 # lock for mtcadummys1

        $mtca4u_executable register_size DUMMY2 ADC AREA_DMA_VIA_DMA

        # check the case where the command is entered incorrectly
        ! $mtca4u_executable register_size DUMMY2 AREA_DMA_VIA_DMA

    ) 9>/var/run/lock/mtcadummy/mtcadummys1

} &> $actual_console_output

grep -v "gcda:Merge mismatch" $actual_console_output > ${actual_console_output}-filtered
diff ${actual_console_output}-filtered $expected_console_output
