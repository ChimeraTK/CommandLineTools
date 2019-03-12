#!/bin/bash


# command usage:
# 'mtca4u read <Board_name> <Module_name> <Register_name> [offset], [elements], [cmode]'
# 

# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_ReadRegisterCommand.txt"
expected_console_output="./referenceTexts/referenceReadRegisterCommand.txt"

mkdir -p /var/run/lock/mtcadummy
( flock 9 # lock for mtcadummys0
  ( flock 8 # lock for mtcadummys1

    # write to the adc enable bit to set the values inside the dma region + populate multiword register
    # WORD_CLK_MUX
    #---------------------------------
    bash -c '$0 write DUMMY1 ""  WORD_ADC_ENA 1 > $1  2>&1' $mtca4u_executable $actual_console_output 
    bash -c '$0 write DUMMY2 ADC  WORD_ADC_ENA 1 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    $mtca4u_executable write  DUMMY1 "" WORD_CLK_MUX 7$'\t'8$'\t'9$'\t'10 >> $actual_console_output 2>&1
    $mtca4u_executable write  DUMMY2 ADC WORD_CLK_MUX 11$'\t'12$'\t'13$'\t'14 >> $actual_console_output 2>&1
    #---------------------------------

    # Read elements from AREA_DMAABLE register. If this works in returning the parabolic
    # values (provided card in a sanitized state), basic read works. Remaing tests
    # on WORD_CLK_MUX are for testing the other aspects of read
    echo "read DMA region - board_withoutModules" >> $actual_console_output 
    bash -c '$0 read DUMMY1 ""  AREA_DMAABLE 0 25 >> $1  2>&1' $mtca4u_executable $actual_console_output 

    echo "read DMA region - board_withModules" >> $actual_console_output
    bash -c '$0 read DUMMY2 ADC  AREA_DMAABLE 0 25 >> $1  2>&1' $mtca4u_executable $actual_console_output 

    # read with no default arguments in board without/with modules
    # write to WORF_CLK_MUX and read it in
    echo "read with no default arguments in board without modules" >> $actual_console_output 
    bash -c '$0 read DUMMY1 ""  WORD_CLK_MUX >> $1  2>&1' $mtca4u_executable $actual_console_output 
    #if [ $? -ne 0 ] ; then # 0 is the exit status for a successful command
    #    exit -1
    #fi
     
    echo "read with no default arguments in board with modules" >> $actual_console_output 
    bash -c '$0 read DUMMY2 ADC WORD_CLK_MUX >> $1  2>&1' $mtca4u_executable $actual_console_output 




    # read with offset - valid/invalid
    echo "read WORD_CLK_MUX from offset 2" >> $actual_console_output
    bash -c '$0 read DUMMY1 ""  WORD_CLK_MUX 2 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    # invalid: offset, but full register length 
    bash -c '$0 read DUMMY1 ""  WORD_CLK_MUX 2 4 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "read WORD_CLK_MUX from offset 0" >> $actual_console_output
    bash -c '$0 read DUMMY1 ""  WORD_CLK_MUX 0 >> $1  2>&1' $mtca4u_executable $actual_console_output 

    # invalid offsets:
    echo "invalid WORD_CLK_MUX offset: 4" >> $actual_console_output
    bash -c '$0 read DUMMY1 ""  WORD_CLK_MUX 4 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "invalid WORD_CLK_MUX offset: 5" >> $actual_console_output
    bash -c '$0 read DUMMY1 ""  WORD_CLK_MUX 5 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "invalid WORD_CLK_MUX offset: -5 this is treated as uint and hence data bigger than reg size" >> $actual_console_output
    bash -c '$0 read DUMMY1 ""  WORD_CLK_MUX -5 >> $1  2>&1' $mtca4u_executable $actual_console_output 

    # read with offset, [numelem - valid]
    # read with offset, [numelem - invalid]
    echo "read first 4 Elem from WORD_CLK_MUX" >> $actual_console_output
    bash -c '$0 read DUMMY1 ""  WORD_CLK_MUX 0 4 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "read 0 Elem from WORD_CLK_MUX" >> $actual_console_output
    bash -c '$0 read DUMMY1 ""  WORD_CLK_MUX 0 0 >> $1  2>&1' $mtca4u_executable $actual_console_output
    echo "read first 5 Elem from WORD_CLK_MUX <- invalid case" >> $actual_console_output
    bash -c '$0 read DUMMY1 ""  WORD_CLK_MUX 3 5 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "read first -5 Elem from WORD_CLK_MUX <- invalid case" >> $actual_console_output
    bash -c '$0 read DUMMY1 ""  WORD_CLK_MUX 3 -5 >> $1  2>&1' $mtca4u_executable $actual_console_output 

    # read - hexrepresentation for raw 
    # read - uint representation for raw
    echo "hex representation for Raw Value" >> $actual_console_output
    bash -c '$0 read DUMMY2 ADC  WORD_CLK_MUX 0 4 hex >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "uint representation for Raw Value" >> $actual_console_output
    bash -c '$0 read DUMMY2 ADC  WORD_CLK_MUX 0 4 raw >> $1  2>&1' $mtca4u_executable $actual_console_output 
    echo "uint representation for Raw Value when num elem is 0 -> This was causing aseg fault before" >> $actual_console_output
    bash -c '$0 read DUMMY2 ADC  WORD_CLK_MUX 0 0 raw >> $1  2>&1' $mtca4u_executable $actual_console_output 

    # not enough arguments
    echo "Command called with not enough arguments" >> $actual_console_output
    bash -c '$0 read DUMMY2 >> $1  2>&1' $mtca4u_executable $actual_console_output 

    # Bad Device name
    echo "Command called with bad device name" >> $actual_console_output
    bash -c '$0 read NON_EXISTENT_DEVICE ""  WORD_CLK_MUX 2 >> $1  2>&1' $mtca4u_executable $actual_console_output 

    # Not using dmap and map file, but sdm uri and numerical address
    echo "Command called with SDM URI / CDD and numerical address" >> $actual_console_output
    bash -c '$0 read sdm://./pci:mtcadummys0 "" "#/0/60" 0 0 hex >> $1  2>&1' $mtca4u_executable $actual_console_output 
    bash -c '$0 read "(pci:mtcadummys0)" "" "#/0/60" >> $1  2>&1' $mtca4u_executable $actual_console_output 
    # Check reading more than one word, in raw and normal mode 
    bash -c '$0 read sdm://./pci:mtcadummys0 "" "#/2/0*16" >> $1  2>&1' $mtca4u_executable $actual_console_output 
    bash -c '$0 read "(pci:mtcadummys0)" "" "#/2/0*16" 0 0 raw >> $1 2>&1' $mtca4u_executable $actual_console_output 
    # Test reading only two words with offet
    bash -c '$0 read sdm://./pci:mtcadummys0 "" "#/2/4*16" 1 2 >> $1  2>&1' $mtca4u_executable $actual_console_output 
    bash -c '$0 read "(pci:mtcadummys0)" "" "#/2/4*16" 1 2 hex >> $1  2>&1' $mtca4u_executable $actual_console_output 

  ) 8>/var/run/lock/mtcadummy/mtcadummys1
) 9>/var/run/lock/mtcadummy/mtcadummys0

diff $actual_console_output $expected_console_output
