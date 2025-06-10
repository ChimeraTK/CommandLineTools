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
#include <boost/program_options.hpp>

#include <cstdlib>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// typedefs and Functions declarations
using DmaAccessor = ChimeraTK::TwoDRegisterAccessor<double>;

namespace po = boost::program_options;

boost::shared_ptr<ChimeraTK::Device> getDevice(const std::string& deviceName, const std::string& dmapFileName);
DmaAccessor createOpenedMuxDataAccesor(
    const std::string& deviceName, const std::string& module, const std::string& regionName);
void printSeqList(const DmaAccessor& deMuxedData, std::vector<uint> const& seqList, uint offset, uint elements);
std::vector<uint> extractSequenceList(std::string const& list, const DmaAccessor& deMuxedData, uint numSequences);
std::vector<uint> createListWithAllSequences(const DmaAccessor& deMuxedData);
// converts a std::string to uint, catches and replaces the conversion exception, and
// returns 0 if the std::string is empty
void readRegisterInternal(const po::variables_map& args);

using CmdFnc = std::function<void(po::variables_map& map)>;

struct Command {
  CmdFnc callback;
  std::string description;
  std::pair<po::options_description, po::positional_options_description> options;
  std::optional<std::string> help;
};

/**********************************************************************************************************************/

// Only allow long options to prevent it interpreting negative numbers as short options
// NOLINTNEXTLINE(hicpp-signed-bitwise)
constexpr auto style = po::command_line_style::allow_long | po::command_line_style::long_allow_adjacent |
    po::command_line_style::long_allow_next | po::command_line_style::allow_guessing;

static void doHelp(po::variables_map& map);
static void doVersion(po::variables_map& map);
static void doWrite(po::variables_map& map);
static void doInfo(po::variables_map& args);
static void doDeviceInfo(po::variables_map& args);
static void doRegisterInfo(po::variables_map& args);
static void doRegisterSize(po::variables_map& args);
static void doMultiplexedData(po::variables_map& args);

/**********************************************************************************************************************/
std::map<std::string, Command> commands;

/**
 * @brief Main Entry Function
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
int main(int argc, const char* argv[]) {
  commands = {
      {"help", {doHelp, "Print the help text", {}, {}}},
      {"version", {doVersion, "Print the tool's version", {}, {}}},
      {"info", {doInfo, "Prints all devices", {}, {}}},
      {"device_info",
          {doDeviceInfo, "Prints the register list of a device",
              []() {
                po::options_description desc("device-info options");
                desc.add_options()("help", "Print help for read")(
                    "device", po::value<std::string>()->required(), "CDD or alias in DMAP file");

                po::positional_options_description pos;
                pos.add("device", 1);
                return std::make_pair(desc, pos);
              }(),
              {"device"}}},
      {"register_info",
          {doRegisterInfo, "Prints the info of a register",
              []() {
                po::options_description desc("register-info options");
                desc.add_options()("help", "Print help for read")(
                    "device", po::value<std::string>()->required(), "CDD or alias in DMAP file")(
                    "module", po::value<std::string>()->required(), "Name of the module in the device")(
                    "register", po::value<std::string>()->required(), "Name of the register");

                po::positional_options_description pos;
                pos.add("device", 1).add("module", 1).add("register", 1);
                return std::make_pair(desc, pos);
              }(),
              {"device module register"}}},
      {"register_size",
          {doRegisterSize, "Prints the size of a register",
              []() {
                po::options_description desc("register-size options");
                desc.add_options()("help", "Print help for read")(
                    "device", po::value<std::string>()->required(), "CDD or alias in DMAP file")(
                    "module", po::value<std::string>()->required(), "Name of the module in the device")(
                    "register", po::value<std::string>()->required(), "Name of the register");

                po::positional_options_description pos;
                pos.add("device", 1).add("module", 1).add("register", 1);
                return std::make_pair(desc, pos);
              }(),
              {"device module register"}}},
      {"read",
          {readRegisterInternal, "Read data from board",
              []() {
                po::options_description desc("read options");
                desc.add_options()("help", "Print help for read")("device", po::value<std::string>()->required(),
                    "CDD or alias in DMAP file")("module", po::value<std::string>()->required(),
                    "Name of the module in the device")("register", po::value<std::string>()->required(),
                    "Name of the register")("offset", po::value<uint32_t>()->default_value(0), "Offset in register")(
                    "elements", po::value<uint32_t>()->default_value(0), "Number of elements to read")("display-mode",
                    po::value<std::string>()->default_value("double"), "Read-out format (hex, raw or double)");

                po::positional_options_description pos;
                pos.add("device", 1)
                    .add("module", 1)
                    .add("register", 1)
                    .add("offset", 1)
                    .add("elements", 1)
                    .add("display-mode", 1);
                return std::make_pair(desc, pos);
              }(),
              {"device module register [offset] [elements] [hex|raw|double]"}}},
      {"read_dma_raw",
          {readRegisterInternal, "Read raw 32 bit values from DMA registers without fixed point conversion",
              []() {
                po::options_description desc("read_dma_raw options");
                desc.add_options()("help", "Print help for read_dma_raw")(
                    "device", po::value<std::string>()->required(), "CDD or alias in DMAP file")(
                    "module", po::value<std::string>()->required(), "Name of the module in the device")("register",
                    po::value<std::string>()->required(),
                    "Name of the register")("offset", po::value<uint32_t>()->default_value(0), "Offset in register")(
                    "elements", po::value<uint32_t>()->default_value(0), "Number of elements to read")(
                    "display-mode", po::value<std::string>()->default_value("raw"), "Read-out format (hex, raw)");

                po::positional_options_description pos;
                pos.add("device", 1)
                    .add("module", 1)
                    .add("register", 1)
                    .add("offset", 1)
                    .add("elements", 1)
                    .add("display-mode", 1);
                return std::make_pair(desc, pos);
              }(),
              {"device module register [offset] [elements] [hex|raw]"}}},
      {"read_seq",
          {doMultiplexedData, "Get demultiplexed data sequences from a memory region (containing muxed data sequences)",
              []() {
                po::options_description desc("read_seq options");
                desc.add_options()("help", "Print help for read_seq")("device", po::value<std::string>()->required(),
                    "CDD or alias in DMAP file")("module", po::value<std::string>()->required(),
                    "Name of the module in the device")("region-name", po::value<std::string>()->required(),
                    "Name of the register")("sequence-list", po::value<std::string>()->default_value(""),
                    "Space-separated list of channels to print")("offset", po::value<uint32_t>()->default_value(0),
                    "Offset in register")("elements", po::value<uint32_t>(), "Number of elements to read")(
                    "display-mode", po::value<std::string>()->default_value("raw"), "Read-out format (hex, raw)");

                po::positional_options_description pos;
                pos.add("device", 1)
                    .add("module", 1)
                    .add("region-name", 1)
                    .add("sequence-list", 1)
                    .add("offset", 1)
                    .add("elements", 1);
                return std::make_pair(desc, pos);
              }(),
              {"device module region-name [sequence-list] [offset] [elements]"}}},
      {
          "write",
          {doWrite, "Write data to board",
              []() {
                po::options_description desc("write options");
                desc.add_options()("help", "Print help for write")(
                    "device", po::value<std::string>()->required(), "CDD or alias in DMAP file")(
                    "module", po::value<std::string>()->required(), "Name of the module in the device")(
                    "register", po::value<std::string>()->required(), "Name of the register")(
                    "value", po::value<std::string>()->required(), "Values to write to the register")(
                    "offset", po::value<uint32_t>()->default_value(0), "Offset in register");

                po::positional_options_description pos;
                pos.add("device", 1).add("module", 1).add("register", 1).add("value", 1).add("offset", 1);
                return std::make_pair(desc, pos);
              }(),
              {"device module register value [offset]"}},
      },
  };
  namespace po = boost::program_options;

  po::options_description global("Global options");
  global.add_options()("json", "Output data in machine-readable JSON format")("command", po::value<std::string>(),
      "command to execute")("subargs", po::value<std::vector<std::string>>(), "Arguments for command");

  po::positional_options_description subcommandCollector;
  subcommandCollector.add("command", 1).add("subargs", -1);

  po::variables_map vm;

  po::parsed_options parsed = po::command_line_parser(argc, argv)
                                  .options(global)
                                  .positional(subcommandCollector)
                                  .style(style)
                                  .allow_unregistered()
                                  .run();

  po::store(parsed, vm);

  if(vm.count("command") == 0) {
    doHelp(vm);
    return 1;
  }
  std::string cmd = vm["command"].as<std::string>();

  if(auto it = commands.find(cmd); it != commands.end()) {
    auto& command = it->second;
    auto opts = po::collect_unrecognized(parsed.options, po::include_positional);
    opts.erase(opts.begin());
    try {
      // Get the subcommand-specific commandline options from the command entry and parse again
      auto [desc, pos] = command.options;
      po::store(po::command_line_parser(opts).options(desc).positional(pos).style(style).run(), vm);

      // If the user requested help for the command, print it and just exit
      if(vm.count("help") > 0) {
        if(command.help) {
          std::cout << "mtca4u [--json] " << cmd << " " << *command.help << std::endl;
        }
        std::cout << global << std::endl;
        std::cout << desc << std::endl;
        return 0;
      }

      po::notify(vm);

      command.callback(vm);
    }
    catch(std::exception& ex) {
      std::cout << ex.what() << std::endl;
      return 1;
    }
  }
  else {
    doHelp(vm);
  }

  return 0;
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

// Implementations

/**********************************************************************************************************************/

static void doHelp(po::variables_map& /*map*/) {
  std::cout << std::endl
            << "mtca4u command line tools, version " << ChimeraTK::command_line_tools::VERSION << "\n"
            << std::endl;
  std::cout << "Available commands:" << std::endl;

  auto keys = std::views::keys(commands);
  std::vector<std::string> foo = {keys.begin(), keys.end()};
  std::ranges::sort(foo, {}, &std::string::length);
  auto maxLength = foo.rbegin()->length();

  for(auto& [name, cmd] : commands) {
    auto pattern = std::format("\t{{: <{}}}\t{{}}", maxLength);
    std::cout << std::vformat(pattern, std::make_format_args(name, cmd.description)) << std::endl;
  }

  std::cout << "\nFor details on a command, run mtca4u command --help" << std::endl;

  std::cout << std::endl
            << std::endl
            << "For further help or bug reports please contact chimeratk_support@desy.de" << std::endl
            << std::endl;
}

/**********************************************************************************************************************/

static void doVersion(po::variables_map& /* */) {
  std::cout << ChimeraTK::command_line_tools::VERSION << std::endl;
}

/**********************************************************************************************************************/

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
 * @brief getInfo shows the device information
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void doInfo([[maybe_unused]] po::variables_map& args) {
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
void doDeviceInfo(po::variables_map& args) {
  boost::shared_ptr<ChimeraTK::Device> device = getDevice(args["device"].as<std::string>());

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
void doRegisterInfo(po::variables_map& args) {
  boost::shared_ptr<ChimeraTK::Device> device = getDevice(args["device"].as<std::string>());
  auto catalog = device->getRegisterCatalogue();

  auto regInfo = catalog.getRegister(args["module"].as<std::string>() + "/" + args["register"].as<std::string>());

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
 * doRegisterSize prints the size of a register (number of elements).
 * \todo FIXME: For 2D- registers it is the size of one channel.
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
void doRegisterSize(po::variables_map& args) {
  boost::shared_ptr<ChimeraTK::Device> device = getDevice(args["device"].as<std::string>());
  auto catalog = device->getRegisterCatalogue();

  auto regInfo = catalog.getRegister(args["module"].as<std::string>() + "/" + args["register"].as<std::string>());

  std::cout << regInfo.getNumberOfElements() << std::endl;
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

void readRegisterInternal(const po::variables_map& args) {
  boost::shared_ptr<ChimeraTK::Device> device = getDevice(args["device"].as<std::string>());

  auto registerPath = ChimeraTK::RegisterPath(args["module"].as<std::string>()) / args["register"].as<std::string>();

  auto offset = args["offset"].as<uint32_t>();
  auto numElements = args["elements"].as<uint32_t>();
  auto displayMode = args["display-mode"].as<std::string>();

  // Read as raw values
  if(displayMode == "raw" || displayMode == "hex") {
    auto accessor =
        device->getOneDRegisterAccessor<int32_t>(registerPath, numElements, offset, {ChimeraTK::AccessMode::raw});
    accessor.read();
    if(displayMode == "hex") {
      std::cout << std::hex;
    }
    else {
      std::cout << std::fixed;
    }
    for(auto value : accessor) {
      std::cout << static_cast<uint32_t>(value) << "\n";
    }
  }
  else if(displayMode == "double") {
    auto accessor = device->getOneDRegisterAccessor<double>(registerPath, numElements, offset);
    accessor.read();
    std::cout << std::scientific << std::setprecision(8);
    for(auto value : accessor) {
      std::cout << value << "\n";
    }
  }
  else {
    throw ChimeraTK::logic_error("Invalid display mode " + displayMode);
  }

  std::cout << std::flush;
}

/**********************************************************************************************************************/

/**
 * @brief doWrite
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 * Parameter: device, module, register, value, [offset]
 */
void doWrite(po::variables_map& map) {
  boost::shared_ptr<ChimeraTK::Device> device = getDevice(map["device"].as<std::string>());
  auto registerPath = ChimeraTK::RegisterPath(map["module"].as<std::string>()) / map["register"].as<std::string>();

  auto offset = map["offset"].as<uint32_t>();

  std::vector<std::string> vS;
  boost::split(vS, map["value"].as<std::string>(), boost::is_any_of("\t "));

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

void doMultiplexedData(po::variables_map& args) {
  auto deMuxedData = createOpenedMuxDataAccesor(
      args["device"].as<std::string>(), args["module"].as<std::string>(), args["region-name"].as<std::string>());
  auto sequenceLength = deMuxedData.getNElementsPerChannel();
  auto numSequences = deMuxedData.getNChannels();
  auto seqList = extractSequenceList(args["sequence-list"].as<std::string>(), deMuxedData, numSequences);
  auto maxOffset = sequenceLength - 1;
  auto offset = args["offset"].as<std::uint32_t>();
  if(offset > maxOffset) {
    auto v = po::invalid_option_value(std::to_string(offset));
    v.set_substitute("option", "--offset");
    throw po::validation_error(v);
  }
  uint32_t elements = sequenceLength - offset;
  if(args.count("elements") > 0) {
    elements = args["elements"].as<std::uint32_t>();
  }

  if(elements > sequenceLength - offset) {
    auto v = po::invalid_option_value(std::to_string(elements));
    v.set_substitute("option", "--elements");
    throw po::validation_error(v);
  }

  if(elements == 0) {
    return;
  }

  printSeqList(deMuxedData, seqList, offset, elements);
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

std::vector<uint> createListWithAllSequences(const DmaAccessor& deMuxedData) {
  uint numSequences = deMuxedData.getNChannels();
  std::vector<uint> seqList(numSequences);
  for(uint index = 0; index < numSequences; ++index) {
    seqList[index] = index;
  }
  return seqList;
}
