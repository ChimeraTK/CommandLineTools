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

#include <mtca4u/DMapFilesParser.h>
#include <mtca4u/Device.h>
#include <mtca4u/MultiplexedDataAccessor.h>
#include <mtca4u/BackendFactory.h>


#include <boost/algorithm/string.hpp>

#include "version.h"

using namespace mtca4u;
using namespace std;


// typedefs and Functions declarations
typedef MultiplexedDataAccessor<double> dma_Accessor_t;
typedef boost::shared_ptr<dma_Accessor_t> dma_Accessor_ptr_t;

//typedef boost::shared_ptr<Device<DummyBackend>::RegisterAccessor> RegisterAccessor_t;
typedef boost::shared_ptr<Device::RegisterAccessor> RegisterAccessor_t;
typedef mtca4u::RegisterInfoMap::RegisterInfo RegisterInfo_t;

//boost::shared_ptr< mtca4u::Device< mtca4u::DummyBackend > > getDevice(const string& deviceName, const string &dmapFileName);
boost::shared_ptr< mtca4u::Device> getDevice(const string& deviceName, const string &dmapFileName);
dma_Accessor_ptr_t createOpenedMuxDataAccesor(const string &deviceName, const string &module, const string &regionName);
void printSeqList (const dma_Accessor_ptr_t& deMuxedData, std::vector<uint> const& seqList, uint offset, uint elements);
std::vector<string> createArgList(uint argc, const char* argv[], uint maxArgs);
std::vector<uint> extractSequenceList(string const & list, const dma_Accessor_ptr_t& deMuxedData, uint maxSeq);
uint extractOffset(string const & userEnteredOffset, uint maxOffset);
uint extractNumElements(string const & userEnteredValue, uint offset, uint maxElements);
RegisterAccessor_t getRegisterAccessor(const string &deviceName, const string &module, const string &registerName);
std::string extractDisplayMode(const string &displayMode);
std::vector<uint> createListWithAllSequences(const dma_Accessor_ptr_t& deMuxedData);

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
void readMultiplexedData(unsigned int , const char **);

vector<Command> vectorOfCommands = {
  Command("help",&PrintHelp,"Prints the help text","\t\t\t\t\t"),
  Command("version",&getVersion,"Prints the tools version","\t\t\t\t"),
  Command("info",&getInfo,"Prints all devices","\t\t\t\t\t"),
  Command("device_info",&getDeviceInfo,"Prints the register of devices","Board Module\t\t\t"),
  Command("register_info",&getRegisterInfo,"Prints the register infos","Board Module Register \t\t"),
  Command("register_size",&getRegisterSize,"Prints the register infos","Board Module Register \t\t"),
  Command("read",&readRegister,"Read data from Board", "\tBoard Module Register [offset] [elements] [raw | hex]"),
  Command("write",&writeRegister,"Write data to Board", "\tBoard Module Register Value [offset]\t"),
  Command("read_dma_raw",&readDmaRawData,"Read raw 32 bit values from DMA registers without Fixed point conversion", "Board Module Register [offset] [elements] [raw | hex]\t"),
  Command("read_seq",&readMultiplexedData,"Get demultiplexed data sequences from a memory region (containing muxed data sequences)", "Board Module DataRegionName [\"sequenceList\"] [Offset] [numElements]")
};
static BackendFactory FactoryInstance = BackendFactory::getInstance();
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

  catch ( Exception &e )
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
boost::shared_ptr< mtca4u::Device > getDevice(const string& deviceName, const string &dmapFileName = "")
{
  DMapFilesParser filesParser;//(".");
  
  if (dmapFileName.empty()) // this will have undefined behaviour if factory is using some other dmap file
  {
  	std::string testFilePath = boost::filesystem::initial_path().string() + (std::string)TEST_DMAP_FILE_PATH;
		filesParser.parse_file(testFilePath);
  }
  else
    filesParser.parse_file(dmapFileName);

  DMapFilesParser::iterator it = filesParser.begin();

  for (; it != filesParser.end(); ++it)
  {
    if (deviceName == it->first.dev_name)
      break;    
  }
  if(it == filesParser.end()){
      throw Exception("Unknown device '" + deviceName + "'.", 2);
  }

  //boost::shared_ptr <mtca4u::DummyBackend> pcieDevice (new mtca4u::PcieBackend());
  //pcieDevice->openDev(it->first.dev_file);

  // creating the mapped device and puuting an opened pcie device in it. By
  // using devBase as the template argument, there is a flexibility of putting
  // other kinds of devices inside this mapped device.
  //Device<mtca4u::DummyBackend> tempDevice;
  //tempDevice.openDev(pcieDevice, it->second);
  //boost::shared_ptr< mtca4u::Device< mtca4u::DummyBackend > > tempDevice =
  //FactoryInstance.createDevice(it->first.dev_name);
  boost::shared_ptr< mtca4u::Device > tempDevice (new Device());
  tempDevice->open(it->first.dev_name);
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
  cout << command_line_tools::VERSION << std::endl;
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
  DMapFilesParser filesParser;
  filesParser.parse_file("dummies.dmap");
  cout << endl << "Available devices: " << endl << endl;
  cout << "Name\tDevice\t\t\tMap-File\t\t\tFirmware\tRevision" << endl;
  
  DMapFilesParser::iterator it = filesParser.begin();

  for (; it != filesParser.end(); ++it)
  {
    //Device<PciDevice> tempDevice;
  	 //boost::shared_ptr< mtca4u::Device< mtca4u::DummyBackend > > tempDevice
			//	= FactoryInstance.createDevice(it->first.dev_name);
  	boost::shared_ptr< mtca4u::Device > tempDevice (new Device());
		tempDevice->open(it->first.dev_name);

	bool available = false;
	int32_t firmware = 0;
    int32_t revision = 0;
		
    try { 
      //tempDevice.openDev(it->first.dev_file, it->first.map_file_name);
	  tempDevice->readReg("WORD_FIRMWARE",&firmware);
	  tempDevice->readReg("WORD_REVISION",&revision);
	  tempDevice->close();
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
    throw Exception("Not enough input arguments.", 1);

  //boost::shared_ptr< mtca4u::Device< mtca4u::DummyBackend > > device = getDevice(argv[0]);

  boost::shared_ptr< mtca4u::Device > device = getDevice(argv[0]);


  boost::shared_ptr<const mtca4u::RegisterInfoMap> map = device->getRegisterMap();

  cout << "Name\t\tElements\tSigned\t\tBits\t\tFractional_Bits\t\tDescription" << endl;

  unsigned int index = 0;
  for (std::vector<RegisterInfo_t>::const_iterator cit = map->begin(); cit != map->end(); ++cit, ++index) {
    // print out module name if present
    if(cit->reg_module.empty()){
	cout << cit->reg_name.c_str() << "\t";
    } else {
	cout << cit->reg_module << "." << cit->reg_name.c_str() << "\t";
    }
    cout << cit->reg_elem_nr << "\t\t" << cit->reg_signed << "\t\t";
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
    throw Exception("Not enough input arguments.", 1);

	//boost::shared_ptr< mtca4u::Device< mtca4u::DummyBackend > > device = getDevice(argv[0]);
  boost::shared_ptr< mtca4u::Device > device = getDevice(argv[0]);
  RegisterAccessor_t reg = device->getRegisterAccessor(argv[2], argv[1]);
  
  RegisterInfo_t regInfo = reg->getRegisterInfo();

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
    throw Exception("Not enough input arguments.", 1);

	//boost::shared_ptr< mtca4u::Device< mtca4u::DummyBackend > > device = getDevice(argv[0]);
  boost::shared_ptr< mtca4u::Device > device = getDevice(argv[0]);
  RegisterAccessor_t reg = device->getRegisterAccessor(argv[2], argv[1]);

  cout << reg->getRegisterInfo().reg_elem_nr << std::endl;

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
  const unsigned int maxCmdArgs = 6;
  
  if(argc < 3){
    throw Exception("Not enough input arguments.", 1);
  }
  // validate argc
  argc = (argc > maxCmdArgs) ? maxCmdArgs : argc;
  std::vector<string> argList = createArgList(argc, argv, maxCmdArgs);

  RegisterAccessor_t reg = getRegisterAccessor( argList[pp_device],
                                                argList[pp_module],
                                                argList[pp_register]);
  RegisterInfo_t regInfo = reg->getRegisterInfo();
  
  uint maxElements = regInfo.reg_elem_nr;
  uint maxOffset = maxElements - 1;
  
  uint offset = extractOffset(argList[pp_offset], maxOffset);
  uint numElements =
      extractNumElements(argList[pp_elements], offset, maxElements);
  if (numElements == 0) {
    return;
  }
// TODO: adapt extractDisplayMode and use here
  string cmode = (argc > pp_cmode) ? argv[pp_cmode] : "double";
  // Read as raw values
  if ((cmode == "raw") || (cmode == "hex")) {
    vector<int32_t> values(numElements);
    reg->readRaw(&(values[0]), numElements * 4, offset * 4);
    if (cmode == "hex")
      cout << std::hex;
    else
      cout << std::fixed;
    for (unsigned int d = 0; (d < regInfo.reg_elem_nr) && (d < values.size()); d++) {
      cout << static_cast<uint32_t>(values[d]) << "\n";
    }
    std::cout << std::flush;
  } else { // Read with automatic conversion to double
    vector<double> values(numElements);
    reg->read(&(values[0]), numElements, offset);
    cout << std::scientific << std::setprecision(8);
    for (unsigned int d = 0; (d < regInfo.reg_elem_nr) && (d < values.size()); d++){
      cout << values[d] << "\n";
    }
    std::cout << std::flush;
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
  
  if(argc < 4){
    throw Exception("Not enough input arguments.", 1);
  }

	//boost::shared_ptr< mtca4u::Device< mtca4u::DummyBackend > > device = getDevice(argv[pp_device]);
	boost::shared_ptr< mtca4u::Device > device = getDevice(argv[pp_device]);
  RegisterAccessor_t reg = device->getRegisterAccessor(argv[pp_register],argv[pp_module]);
  RegisterInfo_t regInfo = reg->getRegisterInfo();

  // TODO: Consider extracting this snippet to a helper method as we use the
  // same check in read command as well
  const uint32_t offset = (argc > pp_offset) ? stoul(argv[pp_offset]) : 0;
  if (regInfo.reg_elem_nr <= offset){
      throw Exception("Offset exceed register size.", 1);
  }

  std::vector<string> vS;
  boost::split(vS, argv[pp_value], boost::is_any_of("\t "));



  vector<double> vD(vS.size());
  try {
    std::transform(vS.begin(), vS.end(), vD.begin(), [](const string& s){ return stod(s); });
  }
  catch(invalid_argument &ex) {
    throw Exception("Could not convert parameter to double.",3);// + d + " to double: " + ex.what(), 3);
  }
  catch(out_of_range &ex) {
    throw Exception("Could not convert parameter to double.",4);// + d + " to double: " + ex.what(), 3);
  }

  reg->write(&(vD[0]), vD.size(), offset);

}

 /**
  * @brief readRawDmaData
  *
  * @param[in] nlhs Number of left hand side parameter
  * @param[inout] phls Pointer to the left hand side parameter
  *
  * Parameter: device, register, [offset], [elements], [display_mode]
  */
void readDmaRawData(unsigned int argc, const char *argv[]) {

  const unsigned int pp_device = 0, pp_module = 1, pp_register = 2,
                     pp_offset = 3, pp_elements = 4, pp_dmode = 5;
  const unsigned int maxCmdArgs = 6;

  if (argc < 3) {
    throw Exception("Not enough input arguments.", 1);
  }

  // validate argc
  argc = (argc > maxCmdArgs) ? maxCmdArgs : argc;
  std::vector<string> argList = createArgList(argc, argv, maxCmdArgs);

  RegisterAccessor_t reg = getRegisterAccessor( argList[pp_device],
                                                argList[pp_module],
                                                argList[pp_register]);
  RegisterInfo_t regInfo = reg->getRegisterInfo();

  uint maxElements = regInfo.reg_elem_nr;
  uint maxOffset = maxElements - 1;
  uint offset = extractOffset(argList[pp_offset], maxOffset);
  uint numElements =
      extractNumElements(argList[pp_elements], offset, maxElements);
  if (numElements == 0) {
    return;
  }

  std::string displayMode = extractDisplayMode(argList[pp_dmode]);
  vector<int32_t> values(numElements);
  reg->readDMA(&(values[0]), numElements * 4, offset * 4);

  if (displayMode == "hex") {
    cout << std::hex;
  } else {
    cout << std::fixed;
  }
  for (unsigned int d = 0; (d < regInfo.reg_elem_nr) && (d < values.size());
       d++) {
    cout << static_cast<uint32_t>(values[d]) << "\n";
  }
  std::cout << std::flush;
}

void readMultiplexedData(unsigned int argc, const char *argv[]) {
  const unsigned int maxCmdArgs = 6;
  const unsigned int pp_deviceName = 0, pp_module = 1, pp_register = 2,
                     pp_seqList = 3, pp_offset = 4, pp_elements = 5;
  if (argc < 3) {
    throw Exception("Not enough input arguments.", 1);
  }

  // validate argc
  argc = (argc > maxCmdArgs) ? maxCmdArgs : argc;
  std::vector<string> argList = createArgList(argc, argv, maxCmdArgs);

  dma_Accessor_ptr_t deMuxedData = createOpenedMuxDataAccesor( argList[pp_deviceName],
                                                               argList[pp_module],
                                                               argList[pp_register]);
  uint sequenceLength = (*deMuxedData)[0].size();
  uint numSequences = (*deMuxedData).getNumberOfDataSequences();
  std::vector<uint> seqList = extractSequenceList(argList[pp_seqList],
																									deMuxedData,
																									numSequences);
  uint maxOffset = sequenceLength - 1;
  uint offset = extractOffset(argList[pp_offset], maxOffset);

  uint numElements = extractNumElements(argList[pp_elements],
                                        offset,
                                        sequenceLength);
  if (numElements == 0) {
    return;
  }

  printSeqList(deMuxedData, seqList, offset, numElements);
}

dma_Accessor_ptr_t createOpenedMuxDataAccesor(const string &deviceName, const string &module, const string &regionName) {

	//boost::shared_ptr< mtca4u::Device< mtca4u::DummyBackend > > device = getDevice(deviceName);
	boost::shared_ptr< mtca4u::Device > device = getDevice(deviceName);
  dma_Accessor_ptr_t deMuxedData = device->getCustomAccessor<dma_Accessor_t>(
  		regionName, module);
  deMuxedData->read();
  return deMuxedData;
}

// expects valid offset and num elements not exceeding sequence length
void printSeqList(const dma_Accessor_ptr_t &deMuxedData, std::vector<uint> const &seqList, uint offset,
                  uint elements) {
  uint elemIndexToStopAt = (offset + elements);
  for (auto i = offset; i < elemIndexToStopAt; i++) {
    for (auto it = seqList.begin(); it != seqList.end(); it++) {
      std::cout << (*deMuxedData)[*it][i] << "\t";
    }
    std::cout << "\n";
  }
  std::cout << std::flush;
}

std::vector<uint> extractSequenceList(string const & list, const dma_Accessor_ptr_t& deMuxedData, uint numSequences) {
  if (list.empty()) {
    return createListWithAllSequences(deMuxedData);
  }

  std::stringstream listOfSeqNumbers(list);
  std::string tmpString;
  std::vector<uint> seqList;
  seqList.reserve(numSequences);

  uint tmpSeqNum;
  try {
    while (std::getline(listOfSeqNumbers, tmpString, ' ')) {
      tmpSeqNum = std::stoul(tmpString);

      if (tmpSeqNum >= numSequences) {
        std::stringstream ss;
        ss << "seqNum invalid. Valid seqNumbers are in the range [0, "
           << (numSequences - 1) << "]";
        throw Exception(ss.str(), 1);
      }

      seqList.push_back(tmpSeqNum);
    }
    return seqList;
  }
  catch (invalid_argument &ex) {
    std::stringstream ss;
    ss << "Could not convert sequence List";
    throw Exception(ss.str(), 3); // + d + " to double: " + ex.what(), 3);
  }
}

std::vector<string> createArgList(uint argc, const char *argv[], uint maxArgs) {
  // pre-condition argc <= maxArgs is assumed when invoking this method.
  std::vector<string> listOfCmdArguments;
  listOfCmdArguments.reserve(maxArgs);

  for (size_t i = 0; i < argc; i++) {
    listOfCmdArguments.push_back(argv[i]);
  }

  // rest of the arguments provided represented as empty strings
  for (size_t i = argc; i < maxArgs; i++) {
    listOfCmdArguments.push_back("");
  }
  return listOfCmdArguments;
}

uint extractOffset(const string &userEnteredOffset, uint maxOffset) {
	// TODO: try avoid code duplication with extractNumElements
  uint offset;
  if (userEnteredOffset.empty()) {
    offset = 0;
  } else {
    try {
      offset = std::stoul(userEnteredOffset);
    }
    catch (invalid_argument &ex) {
      throw Exception("Could not convert Offset", 1);
    }
  }

  if (offset > maxOffset) {
    throw Exception("Offset exceed register size.", 1);
  }

  return offset;
}

uint extractNumElements(const string &userEnteredValue,
                        uint validOffset,
                        uint maxElements) {
  uint numElements;
  try {
    if (userEnteredValue.empty()) {
      numElements = maxElements - validOffset;
    } else {
      numElements = std::stoul(userEnteredValue);
    }
  }
  catch (invalid_argument &ex) {
    throw Exception("Could not convert numElements to return", 1);
  }
  if (numElements > (maxElements - validOffset)) {
    throw Exception("Data size exceed register size.", 1);
  }
  return numElements;
}

RegisterAccessor_t getRegisterAccessor(const string &deviceName,
                                       const string &module,
                                       const string &registerName) {
  boost::shared_ptr< mtca4u::Device > device = getDevice(deviceName);
  RegisterAccessor_t reg =
	device->getRegisterAccessor(registerName, module);
  return reg;
}

std::string extractDisplayMode(const string &displayMode) {

  if (displayMode.empty()) {
    return "raw";
  } // default

  if ((displayMode != "raw") && (displayMode != "hex")) {
    throw Exception("Invalid display mode; Use raw | hex", 1);
  }
  return displayMode;
}

std::vector<uint> createListWithAllSequences(const dma_Accessor_ptr_t &deMuxedData) {
	uint numSequences = deMuxedData->getNumberOfDataSequences();
	std::vector<uint> seqList(numSequences);
	for(uint index = 0; index < numSequences; ++index){
			seqList[index] = index;
	}
	return seqList;
}
