#!/bin/bash -e


# command usage:
# 'mtca4u read <Board_name> <Module_name> <Register_name> [offset], [elements], [cmode]'
#

# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_ReadRegisterCommand.txt"
expected_console_output="./referenceTexts/referenceReadRegisterCommand.txt"

{

  mkdir -p /var/run/lock/mtcadummy
  ( flock 9 # lock for mtcadummys0
    ( flock 8 # lock for mtcadummys1

      # write to the adc enable bit to set the values inside the dma region + populate multiword register
      # WORD_CLK_MUX
      #-----------------------------------------------------------------------------------------------------------------
      $mtca4u_executable write DUMMY1 ""  WORD_ADC_ENA 1
      $mtca4u_executable write DUMMY2 ADC  WORD_ADC_ENA 1
      $mtca4u_executable write  DUMMY1 "" WORD_CLK_MUX 7$'\t'8$'\t'9$'\t'10
      $mtca4u_executable write  DUMMY2 ADC WORD_CLK_MUX 11$'\t'12$'\t'13$'\t'14
      #-----------------------------------------------------------------------------------------------------------------

      # Read elements from AREA_DMAABLE register. If this works in returning the parabolic
      # values (provided card in a sanitized state), basic read works. Remaing tests
      # on WORD_CLK_MUX are for testing the other aspects of read
      echo "read DMA region - board_withoutModules"
      $mtca4u_executable read DUMMY1 ""  AREA_DMAABLE 0 25

      echo "read DMA region - board_withModules"
      $mtca4u_executable read DUMMY2 ADC  AREA_DMAABLE 0 25

      # read with no default arguments in board without/with modules
      # write to WORF_CLK_MUX and read it in
      # ("!" is used to invert the return code)
      echo "read with no default arguments in board without modules"
      ! $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX

      echo "read with no default arguments in board with modules"
      $mtca4u_executable read DUMMY2 ADC WORD_CLK_MUX


      # read with offset - valid/invalid
      echo "read WORD_CLK_MUX from offset 2"
      $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX 2 2
      # invalid: offset, but full register length
      ! $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX 2 4
      echo "read WORD_CLK_MUX from offset 0"
      $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX 0

      # invalid offsets:
      echo "invalid WORD_CLK_MUX offset: 4"
      ! $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX 4
      echo "invalid WORD_CLK_MUX offset: 5"
      ! $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX 5
      echo "invalid WORD_CLK_MUX offset: -5 this is treated as uint and hence data bigger than reg size"
      ! $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX -5

      # read with offset, [numelem - valid]
      # read with offset, [numelem - invalid]
      echo "read first 4 Elem from WORD_CLK_MUX"
      $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX 0 4
      echo "read 0 Elem from WORD_CLK_MUX"
      $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX 0 0
      echo "read first 5 Elem from WORD_CLK_MUX <- invalid case"
      ! $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX 3 5
      echo "read first -5 Elem from WORD_CLK_MUX <- invalid case"
      ! $mtca4u_executable read DUMMY1 ""  WORD_CLK_MUX 3 -5

      # read - hexrepresentation for raw
      # read - uint representation for raw
      echo "hex representation for Raw Value"
      $mtca4u_executable read DUMMY2 ADC  WORD_CLK_MUX 0 4 hex
      echo "uint representation for Raw Value"
      $mtca4u_executable read DUMMY2 ADC  WORD_CLK_MUX 0 4 raw
      echo "uint representation for Raw Value when num elem is 0 -> This was causing aseg fault before"
      $mtca4u_executable read DUMMY2 ADC  WORD_CLK_MUX 0 0 raw

      # not enough arguments
      echo "Command called with not enough arguments"
      ! $mtca4u_executable read DUMMY2

      # Bad Device name
      echo "Command called with bad device name"
      ! $mtca4u_executable read NON_EXISTENT_DEVICE ""  WORD_CLK_MUX 2

      # Not using dmap and map file, but cdd and numerical address
      echo "Command called with SDM URI / CDD and numerical address"
      $mtca4u_executable read "(pci:mtcadummys0)" "" "#/0/60" 0 0 hex
      $mtca4u_executable read "(pci:mtcadummys0)" "" "#/0/60"
      # Check reading more than one word, in raw and normal mode
      $mtca4u_executable read "(pci:mtcadummys0)" "" "#/2/0*16"
      $mtca4u_executable read "(pci:mtcadummys0)" "" "#/2/0*16" 0 0 raw
      # Test reading only two words with offet
      $mtca4u_executable read "(pci:mtcadummys0)" "" "#/2/4*16" 1 2
      $mtca4u_executable read "(pci:mtcadummys0)" "" "#/2/4*16" 1 2 hex

    ) 8>/var/run/lock/mtcadummy/mtcadummys1
  ) 9>/var/run/lock/mtcadummy/mtcadummys0

} &> $actual_console_output

grep -v "gcda:Merge mismatch" $actual_console_output > ${actual_console_output}-filtered
diff ${actual_console_output}-filtered $expected_console_output
