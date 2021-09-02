#!/bin/bash


# command usage:
# 'mtca4u read_dma_raw <Board_name> <Module_name> <Register_name> [offset], [elements], [cmode]'
# 

# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_ReadDMARawCommand.txt"
expected_console_output="./referenceTexts/referenceReadDMARawCommand.txt"

mkdir -p /var/run/lock/mtcadummy
( flock 9 # lock for mtcadummys0

    # write to the adc enable bit to set the parabolic values inside the dma region 
    #---------------------------------
    bash -c '$0 write DUMMY1 ""  WORD_ADC_ENA 1 > $1  2>&1' $mtca4u_executable $actual_console_output 

    # Normal read_dma_raw to a DMA region
    echo "read_dma_raw DMA region - AREA_DMA_VIA_DMA" >> $actual_console_output 
    bash -c '$0 read_dma_raw DUMMY1 ""  AREA_DMA_VIA_DMA 0 20 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "read_dma_raw DMA region - AREA_MULTIPLEXED_SEQUENCE_DMA" >> $actual_console_output 
    bash -c '$0 read_dma_raw DUMMY1 ""  AREA_MULTIPLEXED_SEQUENCE_DMA >> $1  2>&1' $mtca4u_executable $actual_console_output 


    # check offset functionality
    echo "read_dma_raw AREA_MULTIPLEXED_SEQUENCE_DMA from offset 10" >> $actual_console_output
    bash -c '$0 read_dma_raw DUMMY1 ""  AREA_MULTIPLEXED_SEQUENCE_DMA 10 10 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "read_dma_raw AREA_MULTIPLEXED_SEQUENCE_DMA from offset 0" >> $actual_console_output
    bash -c '$0 read_dma_raw DUMMY1 ""  AREA_MULTIPLEXED_SEQUENCE_DMA 0 >> $1  2>&1' $mtca4u_executable $actual_console_output 

    # invalid offsets:
    # FIXME: Should DeviceAccess throw here? Currently it sends 0, which is wrong. Test deactivated, it's just a corner case
    # echo "read_dma_raw AREA_MULTIPLEXED_SEQUENCE_DMA from offset 15" >> $actual_console_output
    #bash -c '$0 read_dma_raw DUMMY1 ""  AREA_MULTIPLEXED_SEQUENCE_DMA 15 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "invalid AREA_MULTIPLEXED_SEQUENCE_DMA offset: 20" >> $actual_console_output
    bash -c '$0 read_dma_raw DUMMY1 ""  AREA_MULTIPLEXED_SEQUENCE_DMA 20 1 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "invalid AREA_MULTIPLEXED_SEQUENCE_DMA: 26" >> $actual_console_output
    bash -c '$0 read_dma_raw DUMMY1 ""  AREA_MULTIPLEXED_SEQUENCE_DMA 21 1 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "invalid AREA_MULTIPLEXED_SEQUENCE_DMA offset: -5" >> $actual_console_output
    bash -c '$0 read_dma_raw DUMMY1 ""  AREA_MULTIPLEXED_SEQUENCE_DMA -5 1 >> $1  2>&1' $mtca4u_executable $actual_console_output 


    # read_dma_raw with offset, [numelem - valid]
    # read_dma_raw with offset, [numelem - invalid]
    echo "read_dma_raw first 10 Elem from AREA_MULTIPLEXED_SEQUENCE_DMA" >> $actual_console_output
    bash -c '$0 read_dma_raw DUMMY1 ""  AREA_MULTIPLEXED_SEQUENCE_DMA 0 10 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "read_dma_raw 0 Elem from AREA_MULTIPLEXED_SEQUENCE_DMA" >> $actual_console_output
    bash -c '$0 read_dma_raw DUMMY1 ""  AREA_MULTIPLEXED_SEQUENCE_DMA 0 0 >> $1  2>&1' $mtca4u_executable $actual_console_output
    echo "read_dma_raw first 26 Elem from AREA_MULTIPLEXED_SEQUENCE_DMA <- invalid case" >> $actual_console_output
    bash -c '$0 read_dma_raw DUMMY1 ""  AREA_MULTIPLEXED_SEQUENCE_DMA 3 26 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "read_dma_raw first -5 Elem from AREA_MULTIPLEXED_SEQUENCE_DMA <- invalid case" >> $actual_console_output
    bash -c '$0 read_dma_raw DUMMY1 ""  AREA_MULTIPLEXED_SEQUENCE_DMA 3 -5 >> $1  2>&1' $mtca4u_executable $actual_console_output 


    # read_dma_raw - hexrepresentation for raw 
    # read_dma_raw - uint representation for raw
    echo "hex representation for Raw Value" >> $actual_console_output
    bash -c '$0 read_dma_raw DUMMY1 ""  AREA_MULTIPLEXED_SEQUENCE_DMA 0 20 hex >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "uint representation for Raw Value" >> $actual_console_output
    bash -c '$0 read_dma_raw DUMMY1 ""  AREA_MULTIPLEXED_SEQUENCE_DMA 0 20 raw >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "uint representation for Raw Value when num elem is 0 -> This was causing aseg fault before" >> $actual_console_output
    bash -c '$0 read_dma_raw DUMMY1 "" AREA_MULTIPLEXED_SEQUENCE_DMA 0 0 raw >> $1  2>&1' $mtca4u_executable $actual_console_output 

) 9>/var/run/lock/mtcadummy/mtcadummys0

# not enough arguments
echo "Command called with not enough arguments" >> $actual_console_output
bash -c '$0 read_dma_raw DUMMY2 "" >> $1  2>&1' $mtca4u_executable $actual_console_output 


# bad parameters
echo "bad num elements" >> $actual_console_output
$mtca4u_executable read_dma_raw  DUMMY1 "" "AREA_MULTIPLEXED_SEQUENCE_DMA" 1 hsg >> $actual_console_output 2>&1
echo "bad offset Value" >> $actual_console_output
$mtca4u_executable read_dma_raw  DUMMY1 "" "AREA_MULTIPLEXED_SEQUENCE_DMA" hj  >> $actual_console_output 2>&1
echo "bad display mode" >> $actual_console_output
$mtca4u_executable read_dma_raw  DUMMY1 "" "AREA_MULTIPLEXED_SEQUENCE_DMA" 0 20 invalid_dispaly_option >> $actual_console_output 2>&1

diff $actual_console_output $expected_console_output
