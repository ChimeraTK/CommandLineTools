#!/bin/bash


# command usage:
# 'mtca4u read_dma <Board_name> <Module_name> <Register_name> [channel_number], [offset], [elements]'
# 


# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_ReadDMAChannelCommand.txt"
expected_console_output="./referenceTexts/referenceReadDMA_ChannelCommand.txt"


# Make sure the AREA_DMA_VIA_DMA region is set to parabolic values
$mtca4u_executable write  DUMMY1 "" "WORD_ADC_ENA" 1 >> $actual_console_output 2>&1

# Normal command usage
echo "reading the Muxed DMA region" >> $actual_console_output
$mtca4u_executable read_dma  DUMMY1 "" 0 >> $actual_console_output 2>&1

# Special cases to be covered

# read an invalid channel
# Read from a valid offset
# Read using invalid offset in the seq
# Elements more than register size with and without offset 

diff $actual_console_output $expected_console_output


# Things to do:
# 1. Set up the dmap file -> Done
# 2. We have 25 values? so how many sequences->5 of 5 values each  -> Done
# All sequences are supposed to be similar? <- for now yes -> Done
# 3. Add a part which describes the DMA region in terms of the above scheme -> Done
# 4. use the command to query the channels 


# What is the mode?
# We should nt be requiring this because we would be describing things in the map file
# mode would not be req... the accessor can figure it out from the mapping scheme