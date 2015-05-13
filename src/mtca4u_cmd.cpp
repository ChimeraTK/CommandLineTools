/**
 * @file mtca4u_cmd.cpp
 *
 * @brief Main file of the MicroTCA 4 You Command Line Tools
 *
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
#include <cstdlib>

#include <MtcaMappedDevice/dmapFilesParser.h>
#include <MtcaMappedDevice/devMap.h>

#include <boost/algorithm/string.hpp>

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
void getVersion(unsigned int, const char **);
void getInfo(unsigned int, const char **);
void getDeviceInfo(unsigned int, const char **);
void getRegisterInfo(unsigned int, const char **);
void getRegisterSize(unsigned int, const char **);
void readRegister(unsigned int, const char **);
void writeRegister(unsigned int, const char **);
void readDmaRawData(unsigned int, const char **);
void readDmaChannel(unsigned int, const char **);

vector<Command> vectorOfCommands = {
  Command("help",&PrintHelp,"Prints the help text","\t\t\t\t"),
  Command("version",&getVersion,"Prints the tools version","\t\t\t\t"),
  Command("info",&getInfo,"Prints all devices","\t\t\t\t"),
  Command("device_info",&getDeviceInfo,"Prints the register of devices","Board Module\t\t"),
  Command("register_info",&getRegisterInfo,"Prints the register infos","Board Module Register \t\t"),
  Command("register_size",&getRegisterSize,"Prints the register infos","Board Module Register \t\t"),
  Command("read",&readRegister,"Read data from Board", "\tBoard Module Register [raw | hex]"),
  Command("write",&writeRegister,"Write data to Board", "\tBoard Module Register Value\t"),
  Command("read_dma_raw",&readDmaRawData,"Read DMA Area from Board", "Board Module Register [Sample] [Offset] [Mode] [Singed] [Bit] [FracBit]\t"),
  Command("read_dma",&readDmaChannel,"Read DMA Channel from Board", "Board Module Register Channel [Sample] [Offset] [Mode] [Singed] [Bit] [FracBit]")
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
  if(argc < 2) {
    cerr << "Not enough input arguments. Please find usage instructions below." << endl;
    PrintHelp(argc, argv);
    return 1;
  }
    
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
      cerr << "Unknown command. Please find usage instructions below." << endl;
      PrintHelp(argc, argv);
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
  cout << endl << "mtca4u command line tools, version " << 0 << "\n" << endl;
  cout << "Available commands are:" << endl << endl;

  for (vector<Command>::iterator it = vectorOfCommands.begin(); it != vectorOfCommands.end(); ++it)
  {
    cout << "  " << it->Name << "\t" << it->Example << "\t" << it->Description << endl;
  }    
  cout << endl << endl << "For further help or bug reports please contact michael.heuer@desy.de" << endl << endl; 
}

/**
 * @brief getVersion shows the command line tools version
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void getVersion(unsigned int /*argc*/, const char* /*argv*/[])
{ 
  cout << 0 << std::endl;
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
  cout << "Name\tDevice\t\t\tMap-File\t\t\tFirmware\tRevision" << endl;
  
  dmapFilesParser::iterator it = filesParser.begin();
  
  for (; it != filesParser.end(); ++it)
  {
    devMap<devPCIE> tempDevice;
	
	bool available = false;
	int32_t firmware = 0;
    int32_t revision = 0;
		
    try { 
      tempDevice.openDev(it->first.dev_file, it->first.map_file_name);
	  tempDevice.readReg("WORD_FIRMWARE",&firmware);
	  tempDevice.readReg("WORD_REVISION",&revision);
	  tempDevice.closeDev();
	  available = true;
    }
	catch(...) {}

    cout << it->first.dev_name << "\t" << it->first.dev_file << "\t\t" << it->first.map_file_name << "\t";
	if (available)
	  cout << firmware << "\t\t" << revision << endl;
	else cout << "na" << "\t\t" << "na" << endl;
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

  boost::shared_ptr<const mtca4u::mapFile> map = device.getRegisterMap();

  cout << "Name\t\tElements\tSigned\t\tBits\t\tFractional_Bits\t\tDescription" << endl;

  unsigned int index = 0;
  for (std::vector<mapFile::mapElem>::const_iterator cit = map->begin(); cit != map->end(); ++cit, ++index) {
    cout << cit->reg_name.c_str() << "\t" << cit->reg_elem_nr << "\t\t" << cit->reg_signed << "\t\t";
    cout << cit->reg_width << "\t\t" << cit->reg_frac_bits << "\t\t\t" << " " << endl; // ToDo: Add Description
  }
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
  if(argc < 3)
    throw exBase("Not enough input arguments.", 1);

  devMap<devPCIE> device = getDevice(argv[0]);
  devMap<devPCIE>::RegisterAccessor reg = device.getRegisterAccessor(argv[2], argv[1]);
  
  mapFile::mapElem regInfo = reg.getRegisterInfo();

  cout << "Name\t\tElements\tSigned\t\tBits\t\tFractional_Bits\t\tDescription" << endl;
  cout << regInfo.reg_name.c_str() << "\t" << regInfo.reg_elem_nr << "\t\t" << regInfo.reg_signed << "\t\t";
  cout << regInfo.reg_width << "\t\t" << regInfo.reg_frac_bits << "\t\t\t" << " " << endl; // ToDo: Add Description
}

/**
 * @brief getRegisterInfo shows the register information
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void getRegisterSize(unsigned int argc, const char *argv[])
{
  if(argc < 3)
    throw exBase("Not enough input arguments.", 1);

  devMap<devPCIE> device = getDevice(argv[0]);
  devMap<devPCIE>::RegisterAccessor reg = device.getRegisterAccessor(argv[2], argv[1]);

  cout << reg.getRegisterInfo().reg_elem_nr << std::endl;
}

/**
 * @brief readRegister
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 * Parameter: device, module, register, [offset], [elements], [cmode]
 */
void readRegister(unsigned int argc, const char* argv[])
{
  const unsigned int pp_device = 0, pp_module = 1, pp_register = 2, pp_offset = 3, pp_elements = 4, pp_cmode = 5;
  
  if(argc < 3)
    throw exBase("Not enough input arguments.", 1);
  
  devMap<devPCIE> device = getDevice(argv[pp_device]);
  devMap<devPCIE>::RegisterAccessor reg = device.getRegisterAccessor(argv[pp_register], argv[pp_module]);
  mapFile::mapElem regInfo = reg.getRegisterInfo();
  
  const uint32_t offset = (argc > pp_offset) ? stoul(argv[pp_offset]) : 0;
  // Check the offset
  if (regInfo.reg_elem_nr <= offset)
   throw exBase("Offset exceed register size.", 1);
  
  const int32_t elements = (argc > pp_elements) ? stoll(argv[pp_elements]) : regInfo.reg_elem_nr - offset;
  if(elements < 0) {
      throw exBase("numberOfelements to read out cannot be a negative value", 1);
  } else if(static_cast<uint32_t>(elements) >  regInfo.reg_elem_nr){
      // Checking for user entered size at this point, because we do not want to
      // run vector<int32_t/double> values(elements) with a huge value in
      // elements (can lead to mem allocation failure if too big).
      throw exBase("Data size exceed register size.", 1);
  }

  string cmode = (argc > pp_cmode) ? argv[pp_cmode] : "double";
  // Read as raw values
  if((cmode == "raw") || (cmode == "hex"))
  {
    vector<int32_t> values(elements);  
    reg.readReg(&(values[0]), elements*4, offset*4);
    if (cmode == "hex") cout << std::hex; else cout << std::fixed;
    for(unsigned int d = 0; (d < regInfo.reg_elem_nr) && (d < values.size()) ; d++)
      cout << static_cast<uint32_t>(values[d]) << endl;
  }
  else { // Read with automatic conversion to double
    vector<double> values(elements);  
    reg.read(&(values[0]), elements, offset*4);
    cout << std::scientific << std::setprecision(8);
    for(unsigned int d = 0; (d < regInfo.reg_elem_nr) && (d < values.size()) ; d++)
      cout << values[d] << endl;
  }
}

/**
 * @brief writeRegister
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 * Parameter: device, module, register, value, [offset]
 */
void writeRegister(unsigned int argc, const char *argv[])
{
  const unsigned int pp_device = 0, pp_module = 1, pp_register = 2, pp_value = 3, pp_offset = 4;
  
  if(argc < 4)
    throw exBase("Not enough input arguments.", 1);
    
  devMap<devPCIE> device = getDevice(argv[pp_device]);
  devMap<devPCIE>::RegisterAccessor reg = device.getRegisterAccessor(argv[pp_register],argv[pp_module]);
  mapFile::mapElem regInfo = reg.getRegisterInfo();

  // TODO: Consider extracting this snippet to a helper method as we use the
  // same check in read command as well
  const uint32_t offset = (argc > pp_offset) ? stoul(argv[pp_offset]) : 0;
  if (regInfo.reg_elem_nr <= offset){
      throw exBase("Offset exceed register size.", 1);
  }

  std::vector<string> vS;
  boost::split(vS, argv[pp_value], boost::is_any_of("\t "));



  vector<double> vD(vS.size());
  try {
    std::transform(vS.begin(), vS.end(), vD.begin(), [](const string& s){ return stod(s); });
  }
  catch(invalid_argument &ex) {
    throw exBase("Could not convert parameter to double.",3);// + d + " to double: " + ex.what(), 3);
  }
  catch(out_of_range &ex) {
    throw exBase("Could not convert parameter to double.",4);// + d + " to double: " + ex.what(), 3);
  }

  reg.write(&(vD[0]), vD.size(), offset*4);
}

 /**
  * @brief readRawDmaData
  *
  * @param[in] nlhs Number of left hand side parameter
  * @param[inout] phls Pointer to the left hand side parameter
  *
  * Parameter: device, register, [offset], [elements], [mode], [singed], [bit], [fracbit]
  */
void readDmaRawData(unsigned int argc, const char *argv[])
{
  const unsigned int pp_device = 0, pp_module = 1, pp_register = 2, pp_offset = 3, pp_elements = 4;
  const unsigned int pp_mode = 5, pp_signed = 6, pp_bit = 7, pp_fracbit = 8;
  
  uint paramCount = 1; // used to count the valid converted Parameter
  
  if(argc < 3)
    throw exBase("Not enough input arguments.", 1);

  try {
    devMap<devPCIE> device = getDevice(argv[pp_device]);
    devMap<devPCIE>::RegisterAccessor reg = device.getRegisterAccessor(argv[pp_module], argv[pp_register]);
    mapFile::mapElem regInfo = reg.getRegisterInfo();
  
    const uint32_t offset = (argc > pp_offset) ? stoul(argv[pp_offset]) : 0;
    paramCount++;
    
    const uint32_t elements = (argc > pp_elements) ? stoul(argv[pp_elements]) : (regInfo.reg_elem_nr - offset);
    paramCount++;
        
    const uint32_t dmode = (argc > pp_mode) ? stoul(argv[pp_mode]) : 32;
    paramCount++;
    
    const uint32_t signedFlag = (argc > pp_signed) ? stoul(argv[pp_signed]) > 0 : false;
    paramCount++;
        
    const uint32_t bits = (argc > pp_bit) ? stoul(argv[pp_bit]) : dmode;
    paramCount++;
    
    const uint32_t fracBits = (argc > pp_fracbit) ? stoul(argv[pp_fracbit]) : 0;
    paramCount++;
    
    if ((dmode != 32) && (dmode != 16))
      throw exBase("Invalid data mode.", 5);
      
    FixedPointConverter conv(bits, fracBits, signedFlag);
	
    if (dmode == 16)
    {
      vector<int16_t> values(elements);
      reg.readDMA(reinterpret_cast<int32_t*>(&values[0]), elements*sizeof(int16_t), offset); // ToDo: add offset for different daq blocks
      // it is safe to cast sample to int because we checked the range before
      for(unsigned int is = 0; is < static_cast<unsigned int>(elements); is++)
    {
	    //if (cmode == "hex")
	    //  cout << std::hex << values[is] << endl;
	    //else if (cmode == "raw")
		//  cout << std::fixed << values[is] << endl;
		//else
          cout << conv.toDouble(values[is]) << endl;
	  }
	}
	else {
      vector<int32_t> values(elements);
      reg.readDMA(&(values[0]), elements*sizeof(int32_t), offset); // ToDo: add offset for different daq blocks
      // it is safe to cast sample to int because we checked the range before
      for(unsigned int is = 0; is < static_cast<unsigned int>(elements); is++)
	  {
	    //if (cmode == "hex")
	    //  cout << std::hex << values[is] << endl;
	    //else if (cmode == "raw")
		//  cout << std::fixed << values[is] << endl;
		//else
          cout << conv.toDouble(values[is]) << endl;
	  }
	}
  }
  catch(invalid_argument &ex) {
    std::stringstream ss;
    ss << "Could not convert parameter " << paramCount << ".";
    throw exBase(ss.str(),3);// + d + " to double: " + ex.what(), 3);
  }
  catch(out_of_range &ex) {
    std::stringstream ss;
    ss << "Could not convert parameter " << paramCount << ".";
    throw exBase(ss.str(),4);// + d + " to double: " + ex.what(), 3);
  }
}

 /**
  * @brief readDmaChannel
  *
  * @param[in] nlhs Number of left hand side parameter
  * @param[inout] phls Pointer to the left hand side parameter
  *
  * Parameter: device, register, channel, [sample], [offset], [dmode], [singed], [bit], [fracbit]
  */
void readDmaChannel(unsigned int argc, const char *argv[])
{
  const unsigned int pp_device = 0, pp_module = 1, pp_register = 2, pp_channel = 3, pp_offset = 4, pp_elements = 5;
  const unsigned int pp_channel_cnt = 6, pp_mode = 7, pp_signed = 8, pp_bit = 9, pp_fracbit = 10;
  
  uint paramCount = 1; // used to count the valid converted Parameter
  
  if(argc < 4)
    throw exBase("Not enough input arguments.", 1);
    
  devMap<devPCIE> device = getDevice(argv[pp_device]);
  devMap<devPCIE>::RegisterAccessor reg = device.getRegisterAccessor(argv[pp_module],argv[pp_register]);
  mapFile::mapElem regInfo = reg.getRegisterInfo();
  
  try {
	  const uint32_t channel = stoul(argv[pp_channel]);
    paramCount++;
    
    const uint32_t offset = (argc > pp_offset) ? stoul(argv[pp_offset]) : 0;
    paramCount++;

    const uint32_t elements = (argc > pp_elements) ? stoul(argv[pp_elements]) : (regInfo.reg_elem_nr - offset);
    paramCount++;

    const uint32_t channel_cnt = (argc > pp_channel_cnt) ? stoul(argv[pp_channel_cnt]) : 8;
    paramCount++;

    const uint32_t mode = (argc > pp_mode) ? stoul(argv[pp_mode]) : 32;
    paramCount++;

    const uint32_t signedFlag = (argc > pp_signed) ? stoul(argv[pp_signed]) : 0;
    paramCount++;

    const uint32_t bits = (argc > pp_bit) ? stoul(argv[pp_bit]) : mode;
    paramCount++;

    const uint32_t fracBits = (argc > pp_fracbit) ? stoul(argv[pp_fracbit]) : 0;

    if ((signedFlag != 0) && (signedFlag != 1))
      throw exBase("Invalid signed Flag.", 5);

    if ((mode != 32) && (mode != 16))
      throw exBase("Invalid data mode.", 5);

    FixedPointConverter conv(bits, fracBits, signedFlag);

    // FIXME
    const uint32_t sample = elements / channel_cnt;
    paramCount++;

    if (sample <= 0) // Prevent invalid inputs
      throw exBase("Invalid number of sample.", 5);

    if (channel >= channel_cnt)
        throw exBase("Invalid channel.", 5);

    const uint32_t size = sample*channel_cnt;
    
	
	if (mode == 16)
	{
      vector<int16_t> values(size);
      reg.readDMA(reinterpret_cast<int32_t*>(&values[0]), size*sizeof(int16_t), 0); // ToDo: add offset for different daq blocks
      // it is safe to cast size to int because we checked the range before
      for(unsigned int is = channel; is < static_cast<unsigned int>(size); is += channel_cnt)
          cout << conv.toDouble(values[is]) << endl;
	}
	else {
      vector<int32_t> values(size);
      reg.readDMA(&(values[0]), size*sizeof(int32_t), 0); // ToDo: add offset for different daq blocks
      // it is safe to cast size to int because we checked the range before
      for(unsigned int is = channel; is < static_cast<unsigned int>(size); is += channel_cnt)
          cout << conv.toDouble(values[is]) << endl;
	}
  }
  catch(invalid_argument &ex) {
    std::stringstream ss;
    ss << "Could not convert parameter " << paramCount << ".";
    throw exBase(ss.str(),3);// + d + " to double: " + ex.what(), 3);
  }
  catch(out_of_range &ex) {
    std::stringstream ss;
    ss << "Could not convert parameter " << paramCount << ".";
    throw exBase(ss.str(),4);// + d + " to double: " + ex.what(), 3);
  }
}
