// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "version.h"

#include <ChimeraTK/Device.h>
#include <ChimeraTK/DMapFileParser.h>
#include <ChimeraTK/NumericAddressedRegisterCatalogue.h>
#include <ChimeraTK/OneDRegisterAccessor.h>
#include <ChimeraTK/TwoDRegisterAccessor.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <cstdlib>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// typedefs and Functions declarations
using DmaAccessor = ChimeraTK::TwoDRegisterAccessor<double>;

boost::shared_ptr<ChimeraTK::Device> getDevice(const std::string& deviceName, const std::string& dmapFileName);
DmaAccessor createOpenedMuxDataAccesor(
    const std::string& deviceName, const std::string& module, const std::string& regionName);
void printSeqList(const DmaAccessor& deMuxedData, std::vector<uint> const& seqList, uint offset, uint elements);
std::vector<std::string> createArgList(uint argc, const char* argv[], uint maxArgs);
std::vector<uint> extractSequenceList(std::string const& list, const DmaAccessor& deMuxedData, uint numSequences);
uint extractOffset(std::string const& userEnteredOffset, uint maxOffset = std::numeric_limits<uint>::max());
uint extractNumElements(
    std::string const& userEnteredValue, uint offset, uint maxElements = std::numeric_limits<uint>::max());
std::string extractDisplayMode(const std::string& displayMode);
std::vector<uint> createListWithAllSequences(const DmaAccessor& deMuxedData);
// converts a std::string to uint, catches and replaces the conversion exception, and
// returns 0 if the std::string is empty
uint stringToUIntWithZeroDefault(const std::string& userEnteredValue);
void readRegisterInternal(const std::vector<std::string>& argList);

using CmdFnc = std::function<void(unsigned int, const char**)>;

struct Command {
  std::string name;
  CmdFnc callback;
  std::string description;
  std::string example;
};

/**********************************************************************************************************************/

// Forward declarations of subcommands
void printHelp(unsigned int, const char**);
void getVersion(unsigned int, const char**);
void getInfo(unsigned int, const char**);
void getDeviceInfo(unsigned int, const char**);
void getRegisterInfo(unsigned int, const char**);
void getRegisterSize(unsigned int, const char**);
void readRegister(unsigned int, const char**);
void writeRegister(unsigned int, const char**);
void readDmaRawData(unsigned int, const char**);
void readMultiplexedData(unsigned int, const char**);

/**********************************************************************************************************************/

static std::vector<Command> vectorOfCommands = {{"help", printHelp, "Prints the help text", "\t\t\t\t\t"},
    {"version", getVersion, "Prints the tools version", "\t\t\t\t"},
    {"info", getInfo, "Prints all devices", "\t\t\t\t\t"},
    {"device_info", getDeviceInfo, "Prints the register list of a device", "Board\t\t\t"},
    {"register_info", getRegisterInfo, "Prints the info of a register", "Board Module Register \t\t"},
    {"register_size", getRegisterSize, "Prints the size of a register", "Board Module Register \t\t"},
    {"read", readRegister, "Read data from Board", "\tBoard Module Register [offset] [elements] [raw | hex]"},
    {"write", writeRegister, "Write data to Board", "\tBoard Module Register Value [offset]\t"},
    {"read_dma_raw", readDmaRawData,
        "Read raw 32 bit values from DMA registers without Fixed point "
        "conversion",
        "Board Module Register [offset] [elements] [raw | hex]\t"},
    {"read_seq", readMultiplexedData,
        "Get demultiplexed data sequences from a memory region (containing "
        "muxed data sequences)",
        "Board Module DataRegionName [\"sequenceList\"] [Offset] "
        "[numElements]"}};

/**********************************************************************************************************************/

/**
 * @brief Main Entry Function
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
int main(int argc, const char* argv[]) {
  if(argc < 2) {
    std::cerr << "Not enough input arguments. Please find usage instructions below." << std::endl;
    printHelp(argc, argv);
    return 1;
  }

  std::string cmd = argv[1];
  std::ranges::transform(cmd, cmd.begin(), ::tolower);

  try {
    auto it = vectorOfCommands.begin();

    // Look for the right command
    for(; it != vectorOfCommands.end(); ++it) {
      if(it->name == cmd) {
        break;
      }
    }

    // Check if search was successful
    if(it == vectorOfCommands.end()) {
      std::cerr << "Unknown command. Please find usage instructions below." << std::endl;
      printHelp(argc, argv);
      return 1;
    }

    // Ok run method
    it->callback(argc - 2, &argv[2]);
  }

  catch(ChimeraTK::logic_error& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

// Implementations

// Try to find a dmap file in the current directory.
// Returns an empty std::string if not found.
std::string findDMapFile() {
  std::vector<boost::filesystem::path> dmapFileNames;
  for(auto& dirEntry : boost::filesystem::directory_iterator(".")) {
    if(dirEntry.path().extension() == ".dmap") {
      dmapFileNames.push_back(dirEntry.path());
    }
  }
  // No DMap file found. Do not throw here but return an empty std::string.
  // We can print a much nicer error message in the context where we know the device alias.
  if(dmapFileNames.empty()) {
    return "";
  }
  if(dmapFileNames.size() > 1) {
    // search for a file named CommandLineTools.dmap and return it. Only throw if not found.
    for(auto& dmapFileName : dmapFileNames) {
      if(dmapFileName.stem() == "CommandLineTools") {
        return dmapFileName.string();
      }
    }

    throw ChimeraTK::logic_error("Found more than one dmap file. Name one of them 'CommandLineTools.dmap' (or create a "
                                 "symlink) so I know which one to take.");
  }

  return dmapFileNames.front().string();
}

/**********************************************************************************************************************/

/**
 * Gets an opened device from the factory.
 *
 * @param dmapFileName File to be loaded or all in the current directory if
 * empty
 * @todo FIXME: This has the old behaviour that it scans all dmap files in the
 * current directory, which is deprecated. Come up with a new, proper mechanism.
 */
// we intentionally use the copy argument so we can safely modify the argument
// inside the function
boost::shared_ptr<ChimeraTK::Device> getDevice(const std::string& deviceName) {
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

  boost::shared_ptr<ChimeraTK::Device> tempDevice(new ChimeraTK::Device());
  tempDevice->open(deviceName);
  return tempDevice;
}

/**********************************************************************************************************************/

/**
 * @brief PrintHelp shows the help text on the console
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void printHelp(unsigned int /*argc*/, const char* /*argv*/[]) {
  std::cout << std::endl
            << "mtca4u command line tools, version " << ChimeraTK::command_line_tools::VERSION << "\n"
            << std::endl;
  std::cout << "Available commands are:" << std::endl << std::endl;

  for(auto& command : vectorOfCommands) {
    std::cout << "  " << command.name << "\t" << command.example << "\t" << command.description << std::endl;
  }
  std::cout << std::endl
            << std::endl
            << "For further help or bug reports please contact chimeratk_support@desy.de" << std::endl
            << std::endl;
}

/**********************************************************************************************************************/

/**
 * @brief getVersion shows the command line tools version
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void getVersion(unsigned int /*argc*/, const char* /*argv*/[]) {
  std::cout << ChimeraTK::command_line_tools::VERSION << std::endl;
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
    std::cout << "No dmap file found. No device information available." << std::endl;
    return;
  }

  ChimeraTK::setDMapFilePath(dmapFileName);
  auto deviceInfoMap = ChimeraTK::DMapFileParser::parse(dmapFileName);

  std::cout << std::endl << "Available devices: " << std::endl << std::endl;
  std::cout << "Name\tDevice\t\t\tMap-File\t\t\tFirmware\tRevision" << std::endl;

  for(auto& deviceInfo : *deviceInfoMap) {
    std::cout << deviceInfo.deviceName << "\t" << deviceInfo.uri
              << "\t\t"
              // mapFileName might be empty
              << (deviceInfo.mapFileName.empty() ? "na" : deviceInfo.mapFileName)
              << "\t"
              // For compatibility: print na. The registers WORD_FIRMWARE and WORD_REVISION
              // don't exist in map files any more, so no one can really have used this feature.
              // It breaks abstraction anyway, so we just disable it, but keep the format for compatibility.
              << "na"
              << "\t\t"
              << "na" << std::endl;
  }
  std::cout << std::endl;
}

/**********************************************************************************************************************/

/**
 * @brief getDeviceInfo shows the device information
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void getDeviceInfo(unsigned int argc, const char* argv[]) {
  if(argc < 1) {
    throw ChimeraTK::logic_error("Not enough input arguments.");
  }

  boost::shared_ptr<ChimeraTK::Device> device = getDevice(argv[0]);

  auto catalog = device->getRegisterCatalogue();

  std::cout << "Name\t\tElements\tSigned\t\tBits\t\tFractional_Bits\t\tDescription" << std::endl;

  unsigned int n2DChannels = 0;
  for(const auto& reg : catalog) {
    if(reg.getNumberOfDimensions() == 2) {
      ++n2DChannels;
      continue;
    }
    std::cout << reg.getRegisterName().getWithAltSeparator() << "\t";
    std::cout << reg.getNumberOfElements() << "\t\t";
    const auto* reg_casted = dynamic_cast<const ChimeraTK::NumericAddressedRegisterInfo*>(&reg);
    if(reg_casted) {
      // ToDo: Add Description and handle multiple channels properly
      std::cout << reg_casted->channels.front().signedFlag << "\t\t";
      std::cout << reg_casted->channels.front().width << "\t\t" << reg_casted->channels.front().nFractionalBits
                << "\t\t\t ";
    }
    std::cout << std::endl;
  }

  if(n2DChannels > 0) {
    std::cout << "\n2D registers\n"
              << "Name\tnChannels\tnElementsPerChannel\n";
    for(const auto& reg : catalog) {
      if(reg.getNumberOfDimensions() != 2) {
        continue;
      }
      std::cout << reg.getRegisterName().getWithAltSeparator() << "\t";
      std::cout << reg.getNumberOfChannels() << "\t\t";
      std::cout << reg.getNumberOfElements() << std::endl;
    }
  }
}

/**********************************************************************************************************************/

/**
 * @brief getRegisterInfo shows the register information
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void getRegisterInfo(unsigned int argc, const char* argv[]) {
  if(argc < 3) {
    throw ChimeraTK::logic_error("Not enough input arguments.");
  }

  boost::shared_ptr<ChimeraTK::Device> device = getDevice(argv[0]);
  auto catalog = device->getRegisterCatalogue();

  auto regInfo = catalog.getRegister(std::string(argv[1]) + "/" + argv[2]);

  std::cout << "Name\t\tElements\tSigned\t\tBits\t\tFractional_Bits\t\tDescription" << std::endl;
  std::cout << regInfo.getRegisterName().getWithAltSeparator() << "\t" << regInfo.getNumberOfElements();

  const auto* regInfo_casted = dynamic_cast<const ChimeraTK::NumericAddressedRegisterInfo*>(&regInfo.getImpl());
  if(regInfo_casted) {
    // ToDo: Add Description and handle multiple channels properly
    std::cout << "\t\t" << regInfo_casted->channels.front().signedFlag << "\t\t";
    std::cout << regInfo_casted->channels.front().width << "\t\t" << regInfo_casted->channels.front().nFractionalBits
              << "\t\t\t " << std::endl;
  }
}

/**********************************************************************************************************************/

/**
 * getRegisterInfo prints the size of a register (number of elements).
 * \todo FIXME: For 2D- registers it is the size of one channel.
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void getRegisterSize(unsigned int argc, const char* argv[]) {
  if(argc < 3) {
    throw ChimeraTK::logic_error("Not enough input arguments.");
  }

  boost::shared_ptr<ChimeraTK::Device> device = getDevice(argv[0]);
  auto catalog = device->getRegisterCatalogue();

  auto regInfo = catalog.getRegister(std::string(argv[1]) + "/" + argv[2]);

  std::cout << regInfo.getNumberOfElements() << std::endl;
}

/**********************************************************************************************************************/

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
  std::vector<std::string> argList = createArgList(argc, argv, maxCmdArgs);

  readRegisterInternal(argList);
}

/**********************************************************************************************************************/

void readRegisterInternal(const std::vector<std::string>& argList) {
  const unsigned int pp_device = 0, pp_module = 1, pp_register = 2, pp_offset = 3, pp_elements = 4, pp_cmode = 5;

  boost::shared_ptr<ChimeraTK::Device> device = getDevice(argList[pp_device]);

  auto registerPath = ChimeraTK::RegisterPath(argList[pp_module]) / argList[pp_register];

  uint offset = stringToUIntWithZeroDefault(argList[pp_offset]);
  uint numElements = stringToUIntWithZeroDefault(argList[pp_elements]);
  std::string cmode = extractDisplayMode(argList[pp_cmode]);

  // Read as raw values
  if((cmode == "raw") || (cmode == "hex")) {
    auto accessor =
        device->getOneDRegisterAccessor<int32_t>(registerPath, numElements, offset, {ChimeraTK::AccessMode::raw});
    accessor.read();
    if(cmode == "hex") {
      std::cout << std::hex;
    }
    else {
      std::cout << std::fixed;
    }
    for(auto value : accessor) {
      std::cout << static_cast<uint32_t>(value) << "\n";
    }
  }
  else { // Read with automatic conversion to double
    auto accessor = device->getOneDRegisterAccessor<double>(registerPath, numElements, offset);
    accessor.read();
    std::cout << std::scientific << std::setprecision(8);
    for(auto value : accessor) {
      std::cout << value << "\n";
    }
  }
  std::cout << std::flush;
}

/**********************************************************************************************************************/

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
  auto registerPath = ChimeraTK::RegisterPath(argv[pp_module]) / argv[pp_register];

  const uint32_t offset = (argc > pp_offset) ? std::stoul(argv[pp_offset]) : 0;

  std::vector<std::string> vS;
  boost::split(vS, argv[pp_value], boost::is_any_of("\t "));

  size_t numElements = vS.size();

  auto accessor = device->getOneDRegisterAccessor<double>(registerPath, numElements, offset);

  try {
    std::ranges::transform(vS, accessor.begin(), [](const std::string& s) { return stod(s); });
  }
  catch(std::invalid_argument&) {
    throw ChimeraTK::logic_error("Could not convert parameter to double."); // + d + " to double: " +
                                                                            // ex.what(), 3);
  }
  catch(std::out_of_range&) {
    throw ChimeraTK::logic_error("Could not convert parameter to double."); // + d + " to double: " +
                                                                            // ex.what(), 3);
  }

  accessor.write();
}

/**********************************************************************************************************************/

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
  std::vector<std::string> argList = createArgList(argc, argv, maxCmdArgs);

  if(argList.size() <= pp_cmode || argList[pp_cmode].empty()) {
    argList.resize(pp_cmode + 1);
    argList[pp_cmode] = "raw";
  }

  readRegisterInternal(argList);
}

/**********************************************************************************************************************/

void readMultiplexedData(unsigned int argc, const char* argv[]) {
  const unsigned int maxCmdArgs = 6;
  const unsigned int pp_deviceName = 0, pp_module = 1, pp_register = 2, pp_seqList = 3, pp_offset = 4, pp_elements = 5;
  if(argc < 3) {
    throw ChimeraTK::logic_error("Not enough input arguments.");
  }

  // validate argc
  argc = (argc > maxCmdArgs) ? maxCmdArgs : argc;
  std::vector<std::string> argList = createArgList(argc, argv, maxCmdArgs);

  DmaAccessor deMuxedData =
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

/**********************************************************************************************************************/

DmaAccessor createOpenedMuxDataAccesor(
    const std::string& deviceName, const std::string& module, const std::string& regionName) {
  boost::shared_ptr<ChimeraTK::Device> device = getDevice(deviceName);
  auto deMuxedData = device->getTwoDRegisterAccessor<double>(module + "/" + regionName);
  deMuxedData.read();
  return deMuxedData;
}

/**********************************************************************************************************************/

// expects valid offset and num elements not exceeding sequence length
void printSeqList(const DmaAccessor& deMuxedData, std::vector<uint> const& seqList, uint offset, uint elements) {
  uint elemIndexToStopAt = (offset + elements);
  for(auto i = offset; i < elemIndexToStopAt; i++) {
    for(const auto& it : seqList) {
      std::cout << deMuxedData[it][i] << "\t";
    }
    std::cout << "\n";
  }
  std::cout << std::flush;
}

std::vector<uint> extractSequenceList(std::string const& list, const DmaAccessor& deMuxedData, uint numSequences) {
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
  catch(std::invalid_argument&) {
    std::stringstream ss;
    ss << "Could not convert sequence List";
    throw ChimeraTK::logic_error(ss.str()); // + d + " to double: " + ex.what(), 3);
  }
}

/**********************************************************************************************************************/

std::vector<std::string> createArgList(uint argc, const char* argv[], uint maxArgs) {
  // pre-condition argc <= maxArgs is assumed when invoking this method.
  std::vector<std::string> listOfCmdArguments;
  listOfCmdArguments.reserve(maxArgs);

  for(size_t i = 0; i < argc; i++) {
    listOfCmdArguments.emplace_back(argv[i]);
  }

  // rest of the arguments provided represented as empty std::strings
  for(size_t i = argc; i < maxArgs; i++) {
    listOfCmdArguments.emplace_back("");
  }
  return listOfCmdArguments;
}

/**********************************************************************************************************************/

uint extractOffset(const std::string& userEnteredOffset, uint maxOffset) {
  // TODO: try avoid code duplication with extractNumElements
  uint offset;
  if(userEnteredOffset.empty()) {
    offset = 0;
  }
  else {
    try {
      offset = std::stoul(userEnteredOffset);
    }
    catch(std::invalid_argument&) {
      throw ChimeraTK::logic_error("Could not convert Offset");
    }
  }

  if(offset > maxOffset) {
    throw ChimeraTK::logic_error("Offset exceed register size.");
  }

  return offset;
}

/**********************************************************************************************************************/

uint extractNumElements(const std::string& userEnteredValue, uint validOffset, uint maxElements) {
  uint numElements;
  try {
    if(userEnteredValue.empty()) {
      numElements = maxElements - validOffset;
    }
    else {
      numElements = std::stoul(userEnteredValue);
    }
  }
  catch(std::invalid_argument&) {
    throw ChimeraTK::logic_error("Could not convert numElements to return");
  }
  if(numElements > (maxElements - validOffset)) {
    throw ChimeraTK::logic_error("Data size exceed register size.");
  }
  return numElements;
}

/**********************************************************************************************************************/

uint stringToUIntWithZeroDefault(const std::string& userEnteredValue) {
  // return 0 if the std::string is empty (0 means the whole register or no offset)
  if(userEnteredValue.empty()) {
    return 0;
  }

  // Just extract the number and convert a possible conversion exception to
  // an ChimeraTK::logic_error with proper error message
  uint numElements;
  try {
    numElements = std::stoul(userEnteredValue);
  }
  catch(std::invalid_argument&) {
    throw ChimeraTK::logic_error("Could not convert numElements or offset to a valid number.");
  }

  return numElements;
}

/**********************************************************************************************************************/

std::string extractDisplayMode(const std::string& displayMode) {
  if(displayMode.empty()) {
    return "double";
  } // default

  if((displayMode != "raw") && (displayMode != "hex") && (displayMode != "double")) {
    throw ChimeraTK::logic_error("Invalid display mode; Use raw | hex");
  }
  return displayMode;
}

/**********************************************************************************************************************/

std::vector<uint> createListWithAllSequences(const DmaAccessor& deMuxedData) {
  uint numSequences = deMuxedData.getNChannels();
  std::vector<uint> seqList(numSequences);
  for(uint index = 0; index < numSequences; ++index) {
    seqList[index] = index;
  }
  return seqList;
}
