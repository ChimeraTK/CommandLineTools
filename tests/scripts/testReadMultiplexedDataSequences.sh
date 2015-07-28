#!/bin/bash


# command usage:
# 'mtca4u read_dma <Board_name> <Module_name> <Register_name> [channel_number], [offset], [elements]'
# 


# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_ReadMultiplexedDataSequences.txt"
expected_console_output="./referenceTexts/referenceReadMultiplexedDataSequences.txt"


# Make sure the AREA_DMA_VIA_DMA region is set to parabolic values
$mtca4u_executable write  DUMMY1 "" "WORD_ADC_ENA" 1 > $actual_console_output 2>&1

# Normal command usage
echo "reading the Muxed DMA region -> print all sequences" >> $actual_console_output
$mtca4u_executable read_seq  DUMMY1 "" "DMA" >> $actual_console_output 2>&1
echo "reading the Muxed DMA region -> print selected sequence" >> $actual_console_output
$mtca4u_executable read_seq  DUMMY1 "" "DMA" 1 >> $actual_console_output 2>&1

# Special cases to be covered
echo "reading invalid sequence" >> $actual_console_output
$mtca4u_executable read_seq  DUMMY1 "" "DMA" 9 >> $actual_console_output 2>&1

# Read from a valid offset
echo "reading from offset 2 of seq# 1" >> $actual_console_output
$mtca4u_executable read_seq  DUMMY1 "" "DMA" 1 2 >> $actual_console_output 2>&1
# Read using invalid offset in the seq
echo "reading from invalid offset  in seq# 1" >> $actual_console_output
$mtca4u_executable read_seq  DUMMY1 "" "DMA" 1 50 >> $actual_console_output 2>&1

# Elements more than register size with and without offset 
echo "reading specific number of elemnts from seq# 1 offset 1" >> $actual_console_output
$mtca4u_executable read_seq  DUMMY1 "" "DMA" 1 1 3 >> $actual_console_output 2>&1

echo "reading more elemnts than supported seq# 1" >> $actual_console_output
$mtca4u_executable read_seq  DUMMY1 "" "DMA" 1 3 18 >> $actual_console_output 2>&1

# bad parameters
echo "bad parameters" >> $actual_console_output
$mtca4u_executable read_seq  DUMMY1 "" "DMA" 1 hsg 18 >> $actual_console_output 2>&1

echo "insufficient arguments" >> $actual_console_output
$mtca4u_executable read_seq  DUMMY1 "" >> $actual_console_output 2>&1


echo "Using sequence list to print selected sequences (3, 2, 1) - valid case" >> $actual_console_output
$mtca4u_executable read_seq  DUMMY1 "" DMA "3 2 1">> $actual_console_output 2>&1


echo "Using sequence list to print selected sequences (3, 2, 1) - offset = 2, numelements = 1" >> $actual_console_output
bash -c '$0 read_seq DUMMY1 ""  DMA "3 2 1" 2 1 >> $1  2>&1' $mtca4u_executable $actual_console_output 

echo "Using sequence list bad seq num" >> $actual_console_output
$mtca4u_executable read_seq  DUMMY1 "" DMA "3 12 1">> $actual_console_output 2>&1

echo "Using sequence list: Conv error in seq num" >> $actual_console_output
# tried "3 2jh2 1" in the list. seems std::stoul interprets 2jh2 as 2
$mtca4u_executable read_seq  DUMMY1 "" DMA "3 jhjg2 1">> $actual_console_output 2>&1

echo "Using sequence list empty list" >> $actual_console_output
$mtca4u_executable read_seq  DUMMY1 "" DMA "">> $actual_console_output 2>&1

diff $actual_console_output $expected_console_output

