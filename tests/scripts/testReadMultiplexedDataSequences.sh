#!/bin/bash -e


# command usage:
# 'mtca4u read_dma <Board_name> <Module_name> <Register_name> [channel_number], [offset], [elements]'
#


# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_ReadMultiplexedDataSequences.txt"
expected_console_output="./referenceTexts/referenceReadMultiplexedDataSequences.txt"

{

    mkdir -p /var/run/lock/mtcadummy
    ( flock 9 # lock for mtcadummys0

        # Make sure the AREA_DMA_VIA_DMA region is set to parabolic values
        $mtca4u_executable write  DUMMY1 "" "WORD_ADC_ENA" 1

        # Normal command usage
        echo "reading the Muxed DMA region -> print all sequences"
        $mtca4u_executable read_seq  DUMMY1 "" "DMA"
        echo "reading the Muxed DMA region -> print selected sequence"
        $mtca4u_executable read_seq  DUMMY1 "" "DMA" 1

        # Special cases to be covered
        # ("!" is used to invert the return code)
        echo "reading invalid sequence"
        ! $mtca4u_executable read_seq  DUMMY1 "" "DMA" 9

        # Read from a valid offset
        echo "reading from offset 2 of seq# 1"
        $mtca4u_executable read_seq  DUMMY1 "" "DMA" 1 2
        # Read using invalid offset in the seq
        echo "reading from invalid offset  in seq# 1"
        ! $mtca4u_executable read_seq  DUMMY1 "" "DMA" 1 50

        # Elements more than register size with and without offset
        echo "reading specific number of elemnts from seq# 1 offset 1"
        ! $mtca4u_executable read_seq  DUMMY1 "" "DMA" 1 1 3

        echo "reading more elemnts than supported seq# 1"
        ! $mtca4u_executable read_seq  DUMMY1 "" "DMA" 1 3 18

        # bad parameters
        echo "bad parameters"
        ! $mtca4u_executable read_seq  DUMMY1 "" "DMA" 1 hsg 18

        echo "insufficient arguments"
        ! $mtca4u_executable read_seq  DUMMY1 ""


        echo "Using sequence list to print selected sequences (3, 2, 1) - valid case"
        $mtca4u_executable read_seq  DUMMY1 "" DMA "3 2 1"


        echo "Using sequence list to print selected sequences (3, 2, 1) - offset = 2, numelements = 1"
        $mtca4u_executable read_seq DUMMY1 ""  DMA "3 2 1" 2 1

        echo "Using sequence list bad seq num"
        ! $mtca4u_executable read_seq  DUMMY1 "" DMA "3 12 1"

        echo "Using sequence list: Conv error in seq num"
        # tried "3 2jh2 1" in the list. seems std::stoul interprets 2jh2 as 2
        ! $mtca4u_executable read_seq  DUMMY1 "" DMA "3 jhjg2 1"

        echo "Using sequence list empty list"
        $mtca4u_executable read_seq  DUMMY1 "" DMA ""

    ) 9>/var/run/lock/mtcadummy/mtcadummys0

} &> $actual_console_output

grep -v "gcda:Merge mismatch" $actual_console_output > ${actual_console_output}-filtered
diff ${actual_console_output}-filtered $expected_console_output
