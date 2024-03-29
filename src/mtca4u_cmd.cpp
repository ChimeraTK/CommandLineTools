#include <cstdlib>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string.h>
#include <vector>

#include <ChimeraTK/Device.h>
#include <ChimeraTK/DMapFileParser.h>
#include <ChimeraTK/OneDRegisterAccessor.h>
#include <ChimeraTK/TwoDRegisterAccessor.h>
#include <ChimeraTK/NumericAddressedRegisterCatalogue.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "version.h"

using namespace ChimeraTK;
using namespace std;

// typedefs and Functions declarations
typedef TwoDRegisterAccessor<double> dma_Accessor_t;

boost::shared_ptr<ChimeraTK::Device> getDevice(const string& deviceName, const string& dmapFileName);
dma_Accessor_t createOpenedMuxDataAccesor(const string& deviceName, const string& module, const string& regionName);
void printSeqList(const dma_Accessor_t& deMuxedData, std::vector<uint> const& seqList, uint offset, uint elements);
std::vector<string> createArgList(uint argc, const char* argv[], uint maxArgs);
std::vector<uint> extractSequenceList(string const& list, const dma_Accessor_t& deMuxedData, uint maxSeq);
uint extractOffset(string const& userEnteredOffset, uint maxOffset = std::numeric_limits<uint>::max());
uint extractNumElements(
    string const& userEnteredValue, uint offset, uint maxElements = std::numeric_limits<uint>::max());
std::string extractDisplayMode(const string& displayMode);
std::vector<uint> createListWithAllSequences(const dma_Accessor_t& deMuxedData);
// converts a string to uint, catches and replaces the conversion exception, and
// returns 0 if the string is empty
uint stringToUIntWithZeroDefault(const string& userEnteredValue);

typedef void (*CmdFnc)(unsigned int, const char**);

struct Command {
  string Name;
  CmdFnc pCallback;
  string Description;
  string Example;
  Command(string n, CmdFnc p, string d, string e) : Name(n), pCallback(p), Description(d), Example(e) {}
};

void PrintHelp(unsigned int, const char**);
void getVersion(unsigned int, const char**);
void getInfo(unsigned int, const char**);
void getDeviceInfo(unsigned int, const char**);
void getRegisterInfo(unsigned int, const char**);
void getRegisterSize(unsigned int, const char**);
void readRegister(unsigned int, const char**);
void readRegisterInternal(std::vector<string> argList);
void writeRegister(unsigned int, const char**);
void readDmaRawData(unsigned int, const char**);
void readMultiplexedData(unsigned int, const char**);

static vector<Command> vectorOfCommands = {Command("help", &PrintHelp, "Prints the help text", "\t\t\t\t\t"),
    Command("version", &getVersion, "Prints the tools version", "\t\t\t\t"),
    Command("info", &getInfo, "Prints all devices", "\t\t\t\t\t"),
    Command("device_info", &getDeviceInfo, "Prints the register list of a device", "Board\t\t\t"),
    Command("register_info", &getRegisterInfo, "Prints the info of a register", "Board Module Register \t\t"),
    Command("register_size", &getRegisterSize, "Prints the size of a register", "Board Module Register \t\t"),
    Command("read", &readRegister, "Read data from Board", "\tBoard Module Register [offset] [elements] [raw | hex]"),
    Command("write", &writeRegister, "Write data to Board", "\tBoard Module Register Value [offset]\t"),
    Command("read_dma_raw", &readDmaRawData,
        "Read raw 32 bit values from DMA registers without Fixed point "
        "conversion",
        "Board Module Register [offset] [elements] [raw | hex]\t"),
    Command("read_seq", &readMultiplexedData,
        "Get demultiplexed data sequences from a memory region (containing "
        "muxed data sequences)",
        "Board Module DataRegionName [\"sequenceList\"] [Offset] "
        "[numElements]")};

/**
 * @brief Main Entry Function
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
int main(int argc, const char* argv[]) {
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
    for(; it != vectorOfCommands.end(); ++it) {
      if(it->Name == cmd) break;
    }

    // Check if search was successfull
    if(it == vectorOfCommands.end()) {
      cerr << "Unknown command. Please find usage instructions below." << endl;
      PrintHelp(argc, argv);
      return 1;
    }

    // Check if the method is implemented
    else if(nullptr == it->pCallback) {
      cerr << "Command not implemented yet." << endl;
      return 1;
    }

    // Ok run method
    else
      it->pCallback(argc - 2, &argv[2]);
  }

  catch(ChimeraTK::logic_error& e) {
    cerr << e.what() << endl;
    return 1;
  }

  return 0;
}

// Try to find a dmap file in the current directory.
// Returns an empty string if not found.
std::string findDMapFile() {
  std::vector<boost::filesystem::path> dmapFileNames;
  for(auto& dirEntry : boost::filesystem::directory_iterator(".")) {
    if(dirEntry.path().extension() == ".dmap") {
      dmapFileNames.push_back(dirEntry.path());
    }
  }
  // No DMap file found. Do not throw here but return an empty string.
  // We can print a much nicer error message in the contect where we know the device alias.
  if(dmapFileNames.empty()) {
    return "";
  }
  if(dmapFileNames.size() > 1) {
    // search for a file named CommandLineTools.dmap and return it. Only throw if not found.
    for(auto dmapFileName : dmapFileNames) {
      if(dmapFileName.stem() == "CommandLineTools") {
        return dmapFileName.string();
      }
    }

    throw ChimeraTK::logic_error("Found more than one dmap file. Name one of them 'CommandLineTools.dmap' (or create a "
                                 "symlink) so I know which one to take.");
  }

  return dmapFileNames.front().string();
}

/**
 * Gets an opened device from the factory.
 *
 * @param dmapFileName File to be loaded or all in the current directory if
 * empty
 * @todo FIXME: This has the old behaviour that it scans all dmap files in the
 * current directory, which is deprecated. Come up with a new, proper mechanism.
 */
// we intentinally use the copy argument so we can safely modify the argument
// inside the function
boost::shared_ptr<ChimeraTK::Device> getDevice(const string& deviceName) {
  bool isSdm = (deviceName.substr(0, 6) == "sdm://");                       // starts with sdm://
  bool isCdd = ((deviceName.front() == '(') && (deviceName.back() == ')')); // starts with '(' and end with ')' =
                                                                            // Chimera Device Descriptor

  if(!isSdm && !isCdd) {
    /* If the device name is not an sdm and not a cdd, the dmap file path has to
       be set. Try to determine it if not given.
       For SDM URIs and CDDs the dmap file name can be empty. */
    std::string dmapFileName = findDMapFile();
    if(dmapFileName.empty()) {
      throw ChimeraTK::logic_error("No dmap file found to resolve alias name '" + deviceName +
          "'. Provide a dmap file or use a ChimeraTK Device Descriptor!");
    }

    ChimeraTK::setDMapFilePath(dmapFileName);
  }

  boost::shared_ptr<ChimeraTK::Device> tempDevice(new Device());
  tempDevice->open(deviceName);
  return tempDevice;
}

/**
 * @brief PrintHelp shows the help text on the console
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void PrintHelp(unsigned int /*argc*/, const char* /*argv*/[]) {
  cout << endl << "mtca4u command line tools, version " << command_line_tools::VERSION << "\n" << endl;
  cout << "Available commands are:" << endl << endl;

  for(vector<Command>::iterator it = vectorOfCommands.begin(); it != vectorOfCommands.end(); ++it) {
    cout << "  " << it->Name << "\t" << it->Example << "\t" << it->Description << endl;
  }
  cout << endl << endl << "For further help or bug reports please contact chimeratk_support@desy.de" << endl << endl;
}

/**
 * @brief getVersion shows the command line tools version
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void getVersion(unsigned int /*argc*/, const char* /*argv*/[]) {
  cout << command_line_tools::VERSION << std::endl;
}

/**
 * @brief getInfo shows the device information
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void getInfo(unsigned int /*argc*/, const char* /*argv*/[]) {
  auto dmapFileName = findDMapFile();

  if(dmapFileName.empty()) {
    cout << "No dmap file found. No device information available." << endl;
    return;
  }

  ChimeraTK::setDMapFilePath(dmapFileName);
  auto deviceInfoMap = DMapFileParser().parse(dmapFileName);

  cout << endl << "Available devices: " << endl << endl;
  cout << "Name\tDevice\t\t\tMap-File\t\t\tFirmware\tRevision" << endl;

  for(auto& deviceInfo : *deviceInfoMap) {
    cout << deviceInfo.deviceName << "\t" << deviceInfo.uri
         << "\t\t"
         // mapFileName might be empty
         << (deviceInfo.mapFileName.empty() ? "na" : deviceInfo.mapFileName)
         << "\t"
         // For compatibility: print na. The registers WORD_FIRMWARE and WORD_REVISION
         // don't exist in map files any more, so no one can really have used this feature.
         // It breaks abstraction anyway, so we just disable it, but keep the format for compatibility.
         << "na"
         << "\t\t"
         << "na" << endl;
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
void getDeviceInfo(unsigned int argc, const char* argv[]) {
  if(argc < 1) throw ChimeraTK::logic_error("Not enough input arguments.");

  boost::shared_ptr<ChimeraTK::Device> device = getDevice(argv[0]);

  auto catalog = device->getRegisterCatalogue();

  cout << "Name\t\tElements\tSigned\t\tBits\t\tFractional_Bits\t\tDescription" << endl;

  unsigned int n2DChannels = 0;
  for(auto& reg : catalog) {
    if(reg.getNumberOfDimensions() == 2) {
      ++n2DChannels;
      continue;
    }
    cout << reg.getRegisterName().getWithAltSeparator() << "\t";
    cout << reg.getNumberOfElements() << "\t\t";
    auto* reg_casted = dynamic_cast<const ChimeraTK::NumericAddressedRegisterInfo*>(&reg);
    if(reg_casted) {
      // ToDo: Add Description and handle multiple channels properly
      cout << reg_casted->channels.front().signedFlag << "\t\t";
      cout << reg_casted->channels.front().width << "\t\t" << reg_casted->channels.front().nFractionalBits << "\t\t\t ";
    }
    cout << endl;
  }

  if(n2DChannels > 0) {
    cout << "\n2D registers\n"
         << "Name\tnChannels\tnElementsPerChannel\n";
    for(auto& reg : catalog) {
      if(reg.getNumberOfDimensions() != 2) continue;
      cout << reg.getRegisterName().getWithAltSeparator() << "\t";
      cout << reg.getNumberOfChannels() << "\t\t";
      cout << reg.getNumberOfElements() << endl;
    }
  }
}

/**
 * @brief getRegisterInfo shows the register information
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void getRegisterInfo(unsigned int argc, const char* argv[]) {
  if(argc < 3) throw ChimeraTK::logic_error("Not enough input arguments.");

  boost::shared_ptr<ChimeraTK::Device> device = getDevice(argv[0]);
  auto catalog = device->getRegisterCatalogue();

  auto regInfo = catalog.getRegister(std::string(argv[1]) + "/" + argv[2]);

  cout << "Name\t\tElements\tSigned\t\tBits\t\tFractional_Bits\t\tDescription" << endl;
  cout << regInfo.getRegisterName().getWithAltSeparator() << "\t" << regInfo.getNumberOfElements();

  auto* regInfo_casted = dynamic_cast<const ChimeraTK::NumericAddressedRegisterInfo*>(&regInfo.getImpl());
  if(regInfo_casted) {
    // ToDo: Add Description and handle multiple channels properly
    cout << "\t\t" << regInfo_casted->channels.front().signedFlag << "\t\t";
    cout << regInfo_casted->channels.front().width << "\t\t" << regInfo_casted->channels.front().nFractionalBits
         << "\t\t\t " << endl;
  }
}

/**
 * getRegisterInfo prints the size of a register (number of elements).
 * \todo FIXME: For 2D- registers it is the size of one channel.
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void getRegisterSize(unsigned int argc, const char* argv[]) {
  if(argc < 3) throw ChimeraTK::logic_error("Not enough input arguments.");

  boost::shared_ptr<ChimeraTK::Device> device = getDevice(argv[0]);
  auto catalog = device->getRegisterCatalogue();

  auto regInfo = catalog.getRegister(std::string(argv[1]) + "/" + argv[2]);

  cout << regInfo.getNumberOfElements() << std::endl;
}

/**
 * @brief readRegister
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 * Parameter: device, module, register, [offset], [elements], [cmode]
 */
void readRegister(unsigned int argc, const char* argv[]) {
  const unsigned int maxCmdArgs = 6;

  if(argc < 3) {
    throw ChimeraTK::logic_error("Not enough input arguments.");
  }
  // validate argc
  argc = (argc > maxCmdArgs) ? maxCmdArgs : argc;
  std::vector<string> argList = createArgList(argc, argv, maxCmdArgs);

  readRegisterInternal(argList);
}

void readRegisterInternal(std::vector<string> argList) {
  const unsigned int pp_device = 0, pp_module = 1, pp_register = 2, pp_offset = 3, pp_elements = 4, pp_cmode = 5;

  boost::shared_ptr<ChimeraTK::Device> device = getDevice(argList[pp_device]);

  auto registerPath = RegisterPath(argList[pp_module]) / argList[pp_register];

  uint offset = stringToUIntWithZeroDefault(argList[pp_offset]);
  uint numElements = stringToUIntWithZeroDefault(argList[pp_elements]);
  string cmode = extractDisplayMode(argList[pp_cmode]);

  // Read as raw values
  if((cmode == "raw") || (cmode == "hex")) {
    auto accessor = device->getOneDRegisterAccessor<int32_t>(registerPath, numElements, offset, {AccessMode::raw});
    accessor.read();
    if(cmode == "hex")
      cout << std::hex;
    else
      cout << std::fixed;
    for(auto value : accessor) {
      cout << static_cast<uint32_t>(value) << "\n";
    }
  }
  else { // Read with automatic conversion to double
    auto accessor = device->getOneDRegisterAccessor<double>(registerPath, numElements, offset);
    accessor.read();
    cout << std::scientific << std::setprecision(8);
    for(auto value : accessor) {
      cout << value << "\n";
    }
  }
  std::cout << std::flush;
}

/**
 * @brief writeRegister
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 * Parameter: device, module, register, value, [offset]
 */
void writeRegister(unsigned int argc, const char* argv[]) {
  const unsigned int pp_device = 0, pp_module = 1, pp_register = 2, pp_value = 3, pp_offset = 4;

  if(argc < 4) {
    throw ChimeraTK::logic_error("Not enough input arguments.");
  }

  boost::shared_ptr<ChimeraTK::Device> device = getDevice(argv[pp_device]);
  auto registerPath = RegisterPath(argv[pp_module]) / argv[pp_register];

  const uint32_t offset = (argc > pp_offset) ? stoul(argv[pp_offset]) : 0;

  std::vector<string> vS;
  boost::split(vS, argv[pp_value], boost::is_any_of("\t "));

  size_t numElements = vS.size();

  auto accessor = device->getOneDRegisterAccessor<double>(registerPath, numElements, offset);

  try {
    std::transform(vS.begin(), vS.end(), accessor.begin(), [](const string& s) { return stod(s); });
  }
  catch(invalid_argument&) {
    throw ChimeraTK::logic_error("Could not convert parameter to double."); // + d + " to double: " +
                                                                            // ex.what(), 3);
  }
  catch(out_of_range&) {
    throw ChimeraTK::logic_error("Could not convert parameter to double."); // + d + " to double: " +
                                                                            // ex.what(), 3);
  }

  accessor.write();
}

/**
 * @brief readRawDmaData
 *
 * @param[in] nlhs Number of left hand side parameter
 * @param[inout] phls Pointer to the left hand side parameter
 *
 * Parameter: device, register, [offset], [elements], [display_mode]
 */
void readDmaRawData(unsigned int argc, const char* argv[]) {
  const unsigned int pp_cmode = 5;
  const unsigned int maxCmdArgs = 6;

  if(argc < 3) {
    throw ChimeraTK::logic_error("Not enough input arguments.");
  }
  // validate argc
  argc = (argc > maxCmdArgs) ? maxCmdArgs : argc;
  std::vector<string> argList = createArgList(argc, argv, maxCmdArgs);

  if(argList.size() <= pp_cmode || argList[pp_cmode] == "") {
    argList.resize(pp_cmode + 1);
    argList[pp_cmode] = "raw";
  }

  readRegisterInternal(argList);
}

void readMultiplexedData(unsigned int argc, const char* argv[]) {
  const unsigned int maxCmdArgs = 6;
  const unsigned int pp_deviceName = 0, pp_module = 1, pp_register = 2, pp_seqList = 3, pp_offset = 4, pp_elements = 5;
  if(argc < 3) {
    throw ChimeraTK::logic_error("Not enough input arguments.");
  }

  // validate argc
  argc = (argc > maxCmdArgs) ? maxCmdArgs : argc;
  std::vector<string> argList = createArgList(argc, argv, maxCmdArgs);

  dma_Accessor_t deMuxedData =
      createOpenedMuxDataAccesor(argList[pp_deviceName], argList[pp_module], argList[pp_register]);
  uint sequenceLength = deMuxedData.getNElementsPerChannel();
  uint numSequences = deMuxedData.getNChannels();
  std::vector<uint> seqList = extractSequenceList(argList[pp_seqList], deMuxedData, numSequences);
  uint maxOffset = sequenceLength - 1;
  uint offset = extractOffset(argList[pp_offset], maxOffset);

  uint numElements = extractNumElements(argList[pp_elements], offset, sequenceLength);
  if(numElements == 0) {
    return;
  }

  printSeqList(deMuxedData, seqList, offset, numElements);
}

dma_Accessor_t createOpenedMuxDataAccesor(const string& deviceName, const string& module, const string& regionName) {
  boost::shared_ptr<ChimeraTK::Device> device = getDevice(deviceName);
  auto deMuxedData = device->getTwoDRegisterAccessor<double>(module + "/" + regionName);
  deMuxedData.read();
  return deMuxedData;
}

// expects valid offset and num elements not exceeding sequence length
void printSeqList(const dma_Accessor_t& deMuxedData, std::vector<uint> const& seqList, uint offset, uint elements) {
  uint elemIndexToStopAt = (offset + elements);
  for(auto i = offset; i < elemIndexToStopAt; i++) {
    for(auto it = seqList.begin(); it != seqList.end(); it++) {
      std::cout << deMuxedData[*it][i] << "\t";
    }
    std::cout << "\n";
  }
  std::cout << std::flush;
}

std::vector<uint> extractSequenceList(string const& list, const dma_Accessor_t& deMuxedData, uint numSequences) {
  if(list.empty()) {
    return createListWithAllSequences(deMuxedData);
  }

  std::stringstream listOfSeqNumbers(list);
  std::string tmpString;
  std::vector<uint> seqList;
  seqList.reserve(numSequences);

  uint tmpSeqNum;
  try {
    while(std::getline(listOfSeqNumbers, tmpString, ' ')) {
      tmpSeqNum = std::stoul(tmpString);

      if(tmpSeqNum >= numSequences) {
        std::stringstream ss;
        ss << "seqNum invalid. Valid seqNumbers are in the range [0, " << (numSequences - 1) << "]";
        throw ChimeraTK::logic_error(ss.str());
      }

      seqList.push_back(tmpSeqNum);
    }
    return seqList;
  }
  catch(invalid_argument&) {
    std::stringstream ss;
    ss << "Could not convert sequence List";
    throw ChimeraTK::logic_error(ss.str()); // + d + " to double: " + ex.what(), 3);
  }
}

std::vector<string> createArgList(uint argc, const char* argv[], uint maxArgs) {
  // pre-condition argc <= maxArgs is assumed when invoking this method.
  std::vector<string> listOfCmdArguments;
  listOfCmdArguments.reserve(maxArgs);

  for(size_t i = 0; i < argc; i++) {
    listOfCmdArguments.push_back(argv[i]);
  }

  // rest of the arguments provided represented as empty strings
  for(size_t i = argc; i < maxArgs; i++) {
    listOfCmdArguments.push_back("");
  }
  return listOfCmdArguments;
}

uint extractOffset(const string& userEnteredOffset, uint maxOffset) {
  // TODO: try avoid code duplication with extractNumElements
  uint offset;
  if(userEnteredOffset.empty()) {
    offset = 0;
  }
  else {
    try {
      offset = std::stoul(userEnteredOffset);
    }
    catch(invalid_argument&) {
      throw ChimeraTK::logic_error("Could not convert Offset");
    }
  }

  if(offset > maxOffset) {
    throw ChimeraTK::logic_error("Offset exceed register size.");
  }

  return offset;
}

uint extractNumElements(const string& userEnteredValue, uint validOffset, uint maxElements) {
  uint numElements;
  try {
    if(userEnteredValue.empty()) {
      numElements = maxElements - validOffset;
    }
    else {
      numElements = std::stoul(userEnteredValue);
    }
  }
  catch(invalid_argument&) {
    throw ChimeraTK::logic_error("Could not convert numElements to return");
  }
  if(numElements > (maxElements - validOffset)) {
    throw ChimeraTK::logic_error("Data size exceed register size.");
  }
  return numElements;
}

uint stringToUIntWithZeroDefault(const string& userEnteredValue) {
  // return 0 if the string is empty (0 means the whole register or no offset)
  if(userEnteredValue.empty()) {
    return 0;
  }

  // Just extract the number and convert a possible conversion exception to
  // an ChimeraTK::logic_error with proper error message
  uint numElements;
  try {
    numElements = std::stoul(userEnteredValue);
  }
  catch(invalid_argument&) {
    throw ChimeraTK::logic_error("Could not convert numElements or offset to a valid number.");
  }

  return numElements;
}

std::string extractDisplayMode(const string& displayMode) {
  if(displayMode.empty()) {
    return "double";
  } // default

  if((displayMode != "raw") && (displayMode != "hex") && (displayMode != "double")) {
    throw ChimeraTK::logic_error("Invalid display mode; Use raw | hex");
  }
  return displayMode;
}

std::vector<uint> createListWithAllSequences(const dma_Accessor_t& deMuxedData) {
  uint numSequences = deMuxedData.getNChannels();
  std::vector<uint> seqList(numSequences);
  for(uint index = 0; index < numSequences; ++index) {
    seqList[index] = index;
  }
  return seqList;
}
