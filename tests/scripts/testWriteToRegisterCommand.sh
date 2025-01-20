#!/bin/bash -e

# command usage:
# 'mtca4u write <Board_name> <Module_name> <Register_name> <tab_seperated_values> [offset]'
#

# NOTE: Paths specified below, assume the working directory is the build directory
mtca4u_executable=./mtca4u
actual_console_output="./output_WriteToRegisterCommand.txt"
expected_console_output="./referenceTexts/referenceWriteToRegisterCommand.txt"

{

    mkdir -p /var/run/lock/mtcadummy
    ( flock 9 # lock for mtcadummys0

        # wite to a bad register
        echo "Writing to a bad register NAme"
        # the sed command removed the absolute part of the map file path to be able to compare with the referecene
        # ("!" is used to invert the return code)
        ! $mtca4u_executable write  DUMMY1 "" SOME_NON_EXISTENT_REGISTER 7$'\t'8$'\t'9$'\t'10 2>&1 \
        | sed "{s|: .*\./mtcadummy_withoutModules.map|: \./mtcadummy_withoutModules.map|}"\


        # Write to register with/without modules
        echo "Writing to register without module"
        $mtca4u_executable write  DUMMY1 "" WORD_CLK_MUX 78$'\t'28$'\t'91$'\t'1
        $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX

        echo "Writing to register with module"
        $mtca4u_executable write  DUMMY2 ADC WORD_CLK_MUX 14$'\t'12$'\t'11$'\t'144
        $mtca4u_executable read DUMMY2 ADC WORD_CLK_MUX


        # Write more values than can be accomodated
        echo "Writing 5 elements to WORD_CLK_MUX (which is 4 elements long)"
        ! $mtca4u_executable write  DUMMY1 "" WORD_CLK_MUX 78$'\t'28$'\t'91$'\t'1$'\t'786
        echo "Writing 6 elements to WORD_CLK_MUX (which is 4 elements long)"
        ! $mtca4u_executable write  DUMMY1 "" WORD_CLK_MUX 78$'\t'28$'\t'91$'\t'1$'\t'786'\t'786

        # Good offset
        echo "Writing 2 elements to WORD_CLK_MUX from offset 2"
        $mtca4u_executable write  DUMMY1 "" WORD_CLK_MUX 78$'\t'28 2
        $mtca4u_executable read DUMMY1 "" WORD_CLK_MUX 2 2

        # Give a bad offset
        echo "Writing 1 element to WORD_CLK_MUX from offset 4 <- invalid offset"
        ! $mtca4u_executable write  DUMMY1 "" WORD_CLK_MUX 78 4
        echo "Writing 1 element to WORD_CLK_MUX from offset 5 <- invalid offset"
        ! $mtca4u_executable write  DUMMY1 "" WORD_CLK_MUX 78 5
        echo "Writing 1 element to WORD_CLK_MUX from offset 6 <- invalid offset"
        ! $mtca4u_executable write  DUMMY1 "" WORD_CLK_MUX 78 6
        echo "Writing 1 element to WORD_CLK_MUX from offset -5 <- invalid offset"
        ! $mtca4u_executable write  DUMMY1 "" WORD_CLK_MUX 78 -5

        # invoke write with crappy values
        echo "invoke command with bad values"
        ! $mtca4u_executable write  DUMMY1 "" WORD_CLK_MUX "a"$'\t'"h"$'\t'
        # TODO: could'nt find a way to tigger the out_of_range exception for the std::stod
        # conversion at this point. Commenting out this part for now.
        #echo "invoke command with value that messes up range checking in the string to double conversion"
        #$mtca4u_executable write  DUMMY1 "" WORD_CLK_MUX "5.233365555525556954554452"
        ! $mtca4u_executable write  DUMMY1 "" WORD_CLK_MUX "1e4000"

        # invoke write without enough arguments
        echo "invoke command with no values to write"
        ! $mtca4u_executable write  DUMMY1 "" WORD_CLK_MUX
        echo "invoke command with incomplete parameters"
        ! $mtca4u_executable write  DUMMY1 ""

        # Not using dmap and map file, but sdm uri / cdd and numerical address
        # We keep using the  WORD_CLK_MUX register at address 0x20 = 32
        # Just one argument
        $mtca4u_executable write "(pci:mtcadummys0)" "" "#/0/32" 23
        $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX
        #with offset
        $mtca4u_executable write "(pci:mtcadummys0)" "" "#/0/32*16" 26 1
        $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX
        #multiple arguments with offset
        $mtca4u_executable write "(pci:mtcadummys0)" "" "#/0/32*16" 327$'\t'34 1
        $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX
        # some errors:
        echo "too many values for register"
        ! $mtca4u_executable write "(pci:mtcadummys0)" "" "#/0/32*8" 327$'\t'34$'\t'35
        echo "too many values for this offset"
        ! $mtca4u_executable write "(pci:mtcadummys0)" "" "#/0/32*16" 327$'\t'34$'\t'35 2
        echo "offset too large"
        ! $mtca4u_executable write "(pci:mtcadummys0)" "" "#/0/32*8" 32 2

    ) 9>/var/run/lock/mtcadummy/mtcadummys0

} &> $actual_console_output

grep -v "gcda:Merge mismatch" $actual_console_output > ${actual_console_output}-filtered
diff ${actual_console_output}-filtered $expected_console_output
