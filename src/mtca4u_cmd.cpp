/**
 * @file mtca4u_cmd.cpp
 *
 * @brief Main file of the MicroTCA 4 You Matlab Library
 *
 * In this file the main entry point of the mex file is declared
 */

/*
 * Copyright (c) 2014 michael.heuer@desy.de
 *
 */

#include <vector>
#include <map>
#include <string.h>
#include <sstream>
#include <stdexcept>

#include <MtcaMappedDevice/dmapFilesParser.h>
#include <MtcaMappedDevice/devMap.h>

#include "version.h"

using namespace mtca4u;
using namespace std;

// Functions declaration

devMap<devPCIE> getDevice(const string& deviceName, const string &dmapFileName);

// Command Function declarations and stuff

typedef void (*CmdFnc)(unsigned int, const char **);

struct Command {
  string Name; CmdFnc pCallback; string Description; string Example;
  Command(string n,CmdFnc p, string d, string e) : Name(n), pCallback(p), Description(d), Example(e) {};
};

void PrintHelp(unsigned int, const char **);
void getInfo(unsigned int, const char **);
void getDeviceInfo(unsigned int, const char **);
void getRegisterInfo(unsigned int, const char **);
void readRegisterFromDevice(unsigned int, const char **);
void writeRegisterToDevice(unsigned int, const char **);
void readRawDmaDataFromDevice(unsigned int, const char **);
void readDmaChannelFromDevice(unsigned int, const char **);

vector<Command> vectorOfCommands = {
  Command("help",&PrintHelp,"Prints the help text","\t\t\t\t"),
  Command("info",&getInfo,"Prints all devices","\t\t\t\t"),
  Command("device_info",&getDeviceInfo,"Prints the register of devices","BoardName\t\t"),
  Command("register_info",&getRegisterInfo,"Prints the register infos","BoardName Reg\t\t"),
  Command("read",&readRegisterFromDevice,"Read data from Board", "\tBoardName Reg_A [Reg B] ..."),
  Command("write",&writeRegisterToDevice,"Write data to Board", "\tBoardName Reg Value\t"),
  Command("read_dma",&readDmaChannelFromDevice,"Read DMA Channel from Board", "\tBoardName Channel\t"),
  Command("read_dma_raw",&readDmaChannelFromDevice,"Read DMA Area from Board", "\tBoardName\t\t")
};

/**
 * @brief Main Entry Function
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
int main(int argc, const char* argv[])
{
  if(argc < 2) { cerr << "Not enough input arguments. Use mtca4u help to show some help information." << endl; return 1;}
    
  string cmd = argv[1];
  transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

  try {
    vector<Command>::iterator it = vectorOfCommands.begin();
		
    // Look for the right command
    for (; it != vectorOfCommands.end(); ++it)
    {
      if (it->Name == cmd)
        break;
    }

    // Check if search was successfull
    if(it == vectorOfCommands.end()) {
      cerr << "Unknown command. Use mtca4u help to show some help information." << endl;
      return 1;
    }

    // Check if the method is implemented
    else if (NULL == it->pCallback) {
      cerr << "Command not implemented yet." << endl;
      return 1;
    }

    // Ok run method
    else it->pCallback(argc - 2, &argv[2]);
  }

  catch ( exBase &e )
  {
    cerr << e.what() << endl;
    return 1;
  }
  
  return 0;
}

/**
 * @brief loadDevices loads a dmap file
 *
 * @param[in] dmapFileName File to be loaded or all in the current directory if empty
 *
 */
devMap<devPCIE> getDevice(const string& deviceName, const string &dmapFileName = "")
{
  dmapFilesParser filesParser(".");
  
  if (dmapFileName.empty())
    filesParser.parse_dir(".");
  else
    filesParser.parse_file(dmapFileName);
  
  dmapFilesParser::iterator it = filesParser.begin();
  
  for (; it != filesParser.end(); ++it)
  {
    if (deviceName == it->first.dev_name)
      break;    
  }
  
  if(it == filesParser.end())
    throw exBase("Unknown device '" + deviceName + "'.", 2);
    
  devMap<devPCIE> tempDevice;
  tempDevice.openDev(it->first.dev_file, it->first.map_file_name);
  return tempDevice;        
}

/**
 * @brief PrintHelp shows the help text on the console
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void PrintHelp(unsigned int /*argc*/, const char* /*argv*/ [])
{
  cout << endl << "mtca4u command line tools, version " << command_line_tools::VERSION << "\n" << endl;
  cout << "Available commands are:" << endl << endl;

  for (vector<Command>::iterator it = vectorOfCommands.begin(); it != vectorOfCommands.end(); ++it)
  {
    cout << "\t" << it->Name << "\t" << it->Example << "\t" << it->Description << endl;
  }    
  cout << endl << endl << "For further help or bug reports please contact michael.heuer@desy.de" << endl << endl; 
}

/**
 * @brief getInfo shows the device information
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void getInfo(unsigned int /*argc*/, const char* /*argv*/[])
{ 
  dmapFilesParser filesParser(".");
  filesParser.parse_dir(".");
  
  cout << endl << "Available devices: " << endl << endl;
  cout << "Name\t\tSlot\t\tDevice\t\tDriver\t\tFirmware\t\tRevision" << endl;
  
  dmapFilesParser::iterator it = filesParser.begin();
  
  for (; it != filesParser.end(); ++it)
  {
    //devMap<devPCIE> tempDevice;
    
    //try 
    //tempDevice.openDev(it->first.dev_file, it->first.map_file_name);
    
    int firmware = 0;
    //it->second.readReg("WORD_FIRMWARE", &firmware);
     
    // ToDo: Print also other stuff, e.g. Slot Number etc...

    cout << it->first.dev_name << "\t\t\t\t\t\t\t\t" << firmware << endl;
  }

	cout << endl;
 }


/**
 * @brief getDeviceInfo shows the device information
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void getDeviceInfo(unsigned int argc, const char* argv[])
{
  if(argc < 1)
    throw exBase("Not enough input arguments.", 1);
  
  devMap<devPCIE> device = getDevice(argv[0]);

  // ToDo: Implement!
  cout << "Not Implemented yet." << endl;
 }

/**
 * @brief getRegisterInfo shows the register information
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void getRegisterInfo(unsigned int argc, const char *argv[])
{
  if(argc < 2)
    throw exBase("Not enough input arguments.", 1);

  devMap<devPCIE> device = getDevice(argv[0]);
  devMap<devPCIE>::RegisterAccessor reg = device.getRegisterAccessor(argv[1]);
  
  mapFile::mapElem regInfo = reg.getRegisterInfo();

  cout << "Name\t\tElements\tSigned\t\tBits\t\tFractional_Bits\t\tDescription" << endl;
  cout << regInfo.reg_name.c_str() << "\t" << regInfo.reg_elem_nr << "\t\t" << regInfo.reg_signed << "\t\t";
  cout << regInfo.reg_width << "\t\t" << regInfo.reg_frac_bits << "\t\t\t" << "_" << endl;
 }

/**
 * @brief readRegisterFromDevice
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void readRegisterFromDevice(unsigned int argc, const char* argv[])
{
  if(argc < 2)
    throw exBase("Not enough input arguments.", 1);
  
  devMap<devPCIE> device = getDevice(argv[0]);

  // Skip the first arguments which specifies the board
  // and parse all other parameter as register names
  for(unsigned int i = 1; i < argc; i++) 
  {
    devMap<devPCIE>::RegisterAccessor reg = device.getRegisterAccessor(argv[i]);
    mapFile::mapElem regInfo = reg.getRegisterInfo();

    // Use the std::vector to dyamically allocate the memory. (Cleanup is done automatically)
    // It is important to add the size information to allocate sufficient!
    vector<double> values(regInfo.reg_elem_nr);  
      
    // std::vector should throw an exception if there is not enough memory available
    // For bestpractice we add the min function.
    //reg.read(&(values[0]), min(regInfo.reg_elem_nr, (uint32)values.capacity()));
    reg.read(&(values[0]), regInfo.reg_elem_nr);

    for(unsigned int d = 0; (d < regInfo.reg_elem_nr) && (d < values.size()) ; d++)
    {
      cout << values[d] << endl;
    }
  }
}

/**
 * @brief writeRegisterToDevice
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void writeRegisterToDevice(unsigned int argc, const char *argv[])
{
  if(argc < 3)
    throw exBase("Not enough input arguments.", 1);
    
  devMap<devPCIE> device = getDevice(argv[0]);

  string registerName = argv[1];
  const unsigned int valueElements = argc - 2;

  devMap<devPCIE>::RegisterAccessor reg = device.getRegisterAccessor(registerName);
  mapFile::mapElem regInfo = reg.getRegisterInfo();
    
  vector<double> values(regInfo.reg_elem_nr);  

  for(unsigned int d = 0; d < regInfo.reg_elem_nr; d++)
  {      
    try {
      values[d] = (d < valueElements) ? stod(argv[d+2], NULL) : 0;
    }
    catch(invalid_argument &ex) {
      throw exBase("Could not convert parameter to double.",3);// + d + " to double: " + ex.what(), 3);
    }
    catch(out_of_range &ex) {
      throw exBase("Could not convert parameter to double.",4);// + d + " to double: " + ex.what(), 3);
    }
  }
  reg.write(&(values[0]), regInfo.reg_elem_nr);
}

 /**
  * @brief readDmaChannelFromDevice
  *
  * @param[in] nlhs Number of left hand side parameter
  * @param[inout] phls Pointer to the left hand side parameter
  *
  */
void readDmaChannelFromDevice(unsigned int argc, const char *argv[])
{
  if(argc < 2)
    throw exBase("Not enough input arguments.", 1);
    
  devMap<devPCIE> device = getDevice(argv[0]);
  
  try {
    const int32_t sample = stoi(argv[1],NULL);
  
    if (sample <= 0) // Prevent invalid inputs
      throw exBase("Invalid input argument.", 5);
    
    const int32_t totalChannels = 8;
    const uint32_t size = sample*totalChannels;
    vector<int32_t> values(size);
    
    int32_t Channel = -1;
  
    if (argc >= 3) {
      Channel = stoi(argv[2],NULL);
      if ((Channel >= totalChannels) || (Channel < 0))
        throw exBase("Invalid input argument.", 5);
    }

    device.readDMA("AREA_DMA", &(values[0]), size*sizeof(int32_t), 0); // ToDo: add offset for different daq blocks

    // it is safe to cast sample to int because we checked the range before
    for(unsigned int is = 0; is < static_cast<unsigned int>(sample); is++)
    {
      if (Channel == -1) // Print all channel if none is specified
      { 
        cout << values[is*8+0] << " " << values[is*8+1] << " " << values[is*8+2] << " " << values[is*8+3] << " ";
        cout << values[is*8+4] << " " << values[is*8+5] << " " << values[is*8+6] << " " << values[is*8+7] << endl;
      }
      else {
        cout << values[is*8+Channel] << endl;
      } 
    }
  }
  catch(invalid_argument &ex) {
    throw exBase("Could not convert parameter.",3);// + d + " to double: " + ex.what(), 3);
  }
  catch(out_of_range &ex) {
    throw exBase("Could not convert parameter.",4);// + d + " to double: " + ex.what(), 3);
  }
}
