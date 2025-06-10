// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "version.h"

#include <ChimeraTK/Device.h>
#include <ChimeraTK/DMapFileParser.h>
#include <ChimeraTK/NumericAddressedRegisterCatalogue.h>
#include <ChimeraTK/OneDRegisterAccessor.h>
#include <ChimeraTK/TwoDRegisterAccessor.h>

#include <nlohmann/json.hpp>

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

struct Command {
  Command() = default;
  explicit Command(std::pair<po::options_description, po::positional_options_description> opts) : options(opts) {}
  virtual ~Command() = default;
  std::string description;
  std::pair<po::options_description, po::positional_options_description> options;
  std::optional<std::string> help;
  nlohmann::json output{{"version", 1}, {"program_version", ChimeraTK::command_line_tools::VERSION}};

  static std::string findDMapFile() {
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

      throw ChimeraTK::logic_error(
          "Found more than one dmap file. Name one of them 'CommandLineTools.dmap' (or create a "
          "symlink) so I know which one to take.");
    }

    return dmapFileNames.front().string();
  }

  static boost::shared_ptr<ChimeraTK::Device> getDevice(const std::string& deviceName) {
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

  virtual void operator()([[maybe_unused]] std::ostream& stream, [[maybe_unused]] po::variables_map& map) = 0;

  void setError(const std::string& error) { output["error-message"] = error; }
};

std::map<std::string, std::shared_ptr<Command>> commands;

/**********************************************************************************************************************/

struct Version final : Command {
  Version() { description = "Print the tool's version"; };
  ~Version() final = default;
  void operator()(std::ostream& stream, [[maybe_unused]] po::variables_map& args) final {
    stream << ChimeraTK::command_line_tools::VERSION << std::endl;
  }
};

/**********************************************************************************************************************/

struct Help final : Command {
  Help() { description = "Print the help text"; }
  ~Help() final = default;
  void operator()([[maybe_unused]] std::ostream& stream, [[maybe_unused]] po::variables_map& args) final {
    std::cout << std::endl
              << "mtca4u command line tools, version " << ChimeraTK::command_line_tools::VERSION << "\n"
              << std::endl;
    std::cout << "Available commands:" << std::endl;

    auto keys = std::views::keys(commands);
    std::vector<std::string> foo = {keys.begin(), keys.end()};
    std::ranges::sort(foo, {}, &std::string::length);
    auto maxLength = foo.rbegin()->length();

    for(auto& [name, cmd] : commands) {
      std::cout << std::format("\t{: <{}}\t{}", name, maxLength, cmd->description) << std::endl;
    }

    std::cout << "\nFor details on a command, run mtca4u command --help" << std::endl;

    std::cout << std::endl
              << std::endl
              << "For further help or bug reports please contact chimeratk_support@desy.de" << std::endl
              << std::endl;
  }
};
/**********************************************************************************************************************/

struct Info final : Command {
  Info() { description = "Prints all devices"; }
  ~Info() final = default;

  void operator()(std::ostream& stream, [[maybe_unused]] po::variables_map& args) final {
    auto dmapFileName = findDMapFile();

    if(dmapFileName.empty()) {
      stream << "No dmap file found. No device information available." << std::endl;
      setError("No dmap file found. No device information available.");
      return;
    }

    ChimeraTK::setDMapFilePath(dmapFileName);
    auto deviceInfoMap = ChimeraTK::DMapFileParser::parse(dmapFileName);

    stream << std::endl << "Available devices: " << std::endl << std::endl;
    stream << "Name\tDevice\t\t\tMap-File\t\t\tFirmware\tRevision" << std::endl;

    auto devices = nlohmann::json::array();

    for(auto& deviceInfo : *deviceInfoMap) {
      nlohmann::json device = {{"deviceName", deviceInfo.deviceName}, {"uri", deviceInfo.uri},
          {"mapFileName", (deviceInfo.mapFileName.empty() ? "na" : deviceInfo.mapFileName)}};
      devices.push_back(device);
      stream << deviceInfo.deviceName << "\t" << deviceInfo.uri
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
    output["devices"] = devices;
    stream << std::endl;
  }
};

/**********************************************************************************************************************/

struct DeviceInfo final : Command {
  DeviceInfo()
  : Command([]() {
      po::options_description desc("device-info options");
      desc.add_options()("help", "Print help for read")(
          "device", po::value<std::string>()->required(), "CDD or alias in DMAP file");

      po::positional_options_description pos;
      pos.add("device", 1);
      return std::make_pair(desc, pos);
    }()) {
    description = "Prints the register list of a device";
    help = "device";
  }
  ~DeviceInfo() final = default;

  void operator()(std::ostream& stream, [[maybe_unused]] po::variables_map& args) final {
    auto device = getDevice(args["device"].as<std::string>());

    auto catalog = device->getRegisterCatalogue();

    stream << "Name\t\tElements\tSigned\t\tBits\t\tFractional_Bits\t\tDescription" << std::endl;

    nlohmann::json registers = nlohmann::json::array();
    unsigned int n2DChannels = 0;
    for(const auto& reg : catalog) {
      nlohmann::json registerDescription;
      const auto* reg_casted = dynamic_cast<const ChimeraTK::NumericAddressedRegisterInfo*>(&reg);

      registerDescription = {
          {"name", reg.getRegisterName().getWithAltSeparator()},
          {"numberOfElements", reg.getNumberOfElements()},
          {"channels", reg.getNumberOfChannels()},
      };
      if(reg_casted) {
        registerDescription["signed"] = reg_casted->channels.front().signedFlag;
        registerDescription["width"] = reg_casted->channels.front().width;
        registerDescription["fractionalBits"] = reg_casted->channels.front().nFractionalBits;
      }

      registers.push_back(registerDescription);
      if(reg.getNumberOfDimensions() == 2) {
        ++n2DChannels;
        continue;
      }
      stream << reg.getRegisterName().getWithAltSeparator() << "\t";
      stream << reg.getNumberOfElements() << "\t\t";
      if(reg_casted) {
        // ToDo: Add Description and handle multiple channels properly
        stream << reg_casted->channels.front().signedFlag << "\t\t";
        stream << reg_casted->channels.front().width << "\t\t" << reg_casted->channels.front().nFractionalBits
               << "\t\t\t ";
      }
      stream << std::endl;
    }

    if(n2DChannels > 0) {
      stream << "\n2D registers\n"
             << "Name\tnChannels\tnElementsPerChannel\n";
      for(const auto& reg : catalog) {
        if(reg.getNumberOfDimensions() != 2) {
          continue;
        }
        stream << reg.getRegisterName().getWithAltSeparator() << "\t";
        stream << reg.getNumberOfChannels() << "\t\t";
        stream << reg.getNumberOfElements() << std::endl;
      }
    }

    output["registers"] = registers;
  }
};

/**********************************************************************************************************************/

struct RegisterInfo final : Command {
  RegisterInfo()
  : Command([]() {
      po::options_description desc("register-info options");
      desc.add_options()("help", "Print help for register_info")("device", po::value<std::string>()->required(),
          "CDD or alias in DMAP file")("module", po::value<std::string>()->required(),
          "Name of the module in the device")("register", po::value<std::string>()->required(), "Name of the register");

      po::positional_options_description pos;
      pos.add("device", 1).add("module", 1).add("register", 1);
      return std::make_pair(desc, pos);
    }()) {
    description = "Prints the info of a register";
    help = "device module register";
  };
  ~RegisterInfo() final = default;

  void operator()(std::ostream& stream, [[maybe_unused]] po::variables_map& args) final {
    auto device = getDevice(args["device"].as<std::string>());
    auto catalog = device->getRegisterCatalogue();

    auto regInfo = catalog.getRegister(args["module"].as<std::string>() + "/" + args["register"].as<std::string>());

    nlohmann::json registers = nlohmann::json::array();
    nlohmann::json registerDescription;

    stream << "Name\t\tElements\tSigned\t\tBits\t\tFractional_Bits\t\tDescription" << std::endl;
    stream << regInfo.getRegisterName().getWithAltSeparator() << "\t" << regInfo.getNumberOfElements();

    registerDescription = {
        {"name", regInfo.getRegisterName().getWithAltSeparator()},
        {"numberOfElements", regInfo.getNumberOfElements()},
        {"channels", regInfo.getNumberOfChannels()},
    };

    const auto* regInfo_casted = dynamic_cast<const ChimeraTK::NumericAddressedRegisterInfo*>(&regInfo.getImpl());
    if(regInfo_casted) {
      registerDescription["signed"] = regInfo_casted->channels.front().signedFlag;
      registerDescription["width"] = regInfo_casted->channels.front().width;
      registerDescription["fractionalBits"] = regInfo_casted->channels.front().nFractionalBits;
      // ToDo: Add Description and handle multiple channels properly
      stream << "\t\t" << regInfo_casted->channels.front().signedFlag << "\t\t";
      stream << regInfo_casted->channels.front().width << "\t\t" << regInfo_casted->channels.front().nFractionalBits
             << "\t\t\t " << std::endl;
    }
    registers.push_back(registerDescription);
    output["registers"] = registers;
  }
};

/**********************************************************************************************************************/

struct RegisterSize final : Command {
  RegisterSize()
  : Command([]() {
      po::options_description desc("register-size options");
      desc.add_options()("help", "Print help for register_size")("device", po::value<std::string>()->required(),
          "CDD or alias in DMAP file")("module", po::value<std::string>()->required(),
          "Name of the module in the device")("register", po::value<std::string>()->required(), "Name of the register");

      po::positional_options_description pos;
      pos.add("device", 1).add("module", 1).add("register", 1);
      return std::make_pair(desc, pos);
    }()) {
    description = "Prints the size of a register";
    help = "device module register";
  }

  ~RegisterSize() final = default;

  void operator()(std::ostream& stream, po::variables_map& args) final {
    auto device = getDevice(args["device"].as<std::string>());
    auto catalog = device->getRegisterCatalogue();

    auto regInfo = catalog.getRegister(args["module"].as<std::string>() + "/" + args["register"].as<std::string>());

    stream << regInfo.getNumberOfElements() << std::endl;
    output["registerSize"] = regInfo.getNumberOfElements();
  }
};

/**********************************************************************************************************************/

struct Read final : Command {
  Read()
  : Command([]() {
      po::options_description desc("read options");
      desc.add_options()("help", "Print help for read")("device", po::value<std::string>()->required(),
          "CDD or alias in DMAP file")("module", po::value<std::string>()->required(),
          "Name of the module in the device")("register", po::value<std::string>()->required(), "Name of the register")(
          "offset", po::value<uint32_t>()->default_value(0), "Offset in register")(
          "elements", po::value<uint32_t>()->default_value(0), "Number of elements to read")(
          "display-mode", po::value<std::string>()->default_value("double"), "Read-out format (hex, raw or double)");

      po::positional_options_description pos;
      pos.add("device", 1)
          .add("module", 1)
          .add("register", 1)
          .add("offset", 1)
          .add("elements", 1)
          .add("display-mode", 1);
      return std::make_pair(desc, pos);
    }()) {
    description = "Read data from board";
    help = "device module register [offset] [elements] [hex|raw|double]";
  }
  ~Read() final = default;
  void operator()(std::ostream& stream, po::variables_map& args) final {
    boost::shared_ptr<ChimeraTK::Device> device = getDevice(args["device"].as<std::string>());

    auto registerPath = ChimeraTK::RegisterPath(args["module"].as<std::string>()) / args["register"].as<std::string>();

    auto offset = args["offset"].as<uint32_t>();
    auto numElements = args["elements"].as<uint32_t>();
    auto displayMode = args["display-mode"].as<std::string>();

    nlohmann::json data;
    // Read as raw values
    if(displayMode == "raw" || displayMode == "hex") {
      auto accessor =
          device->getOneDRegisterAccessor<int32_t>(registerPath, numElements, offset, {ChimeraTK::AccessMode::raw});
      accessor.read();
      if(displayMode == "hex") {
        stream << std::hex;
      }
      else {
        stream << std::fixed;
      }
      for(auto value : accessor) {
        if(displayMode == "hex") {
          data.push_back(std::format("{:x}", static_cast<uint32_t>(value)));
        }
        else {
          data.push_back(static_cast<uint32_t>(value));
        }
        stream << static_cast<uint32_t>(value) << "\n";
      }
    }
    else if(displayMode == "double") {
      auto accessor = device->getOneDRegisterAccessor<double>(registerPath, numElements, offset);
      accessor.read();
      stream << std::scientific << std::setprecision(8);
      for(auto value : accessor) {
        data.push_back(std::format("{:8e}", value));
        stream << value << "\n";
      }
    }
    else {
      throw ChimeraTK::logic_error("Invalid display mode " + displayMode);
    }

    output["values"] = data;

    stream << std::flush;
  }
};

/**********************************************************************************************************************/

// Only allow long options to prevent it interpreting negative numbers as short options
// NOLINTNEXTLINE(hicpp-signed-bitwise)
constexpr auto style = po::command_line_style::allow_long | po::command_line_style::long_allow_adjacent |
    po::command_line_style::long_allow_next | po::command_line_style::allow_guessing;

static void readRegisterInternal(const po::variables_map& args);
static void doWrite(po::variables_map& map);
static void doMultiplexedData(po::variables_map& args);

/**********************************************************************************************************************/

/**
 * @brief Main Entry Function
 *
 * @param[in] argc Number of additional parameter
 * @param[in] argv Pointer to additional parameter
 *
 */
int main(int argc, const char* argv[]) {
  commands = {
      {"help", std::make_shared<Help>()},
      {"version", std::make_shared<Version>()},
      {"info", std::make_shared<Info>()},
      {"device_info", std::make_shared<DeviceInfo>()},
      {"register_info", std::make_shared<RegisterInfo>()},
      {"register_size", std::make_shared<RegisterSize>()},
      {"read", std::make_shared<Read>()},
#if 0
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
#endif
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
    commands["help"]->operator()(std::cout, vm);
    return 1;
  }
  std::string cmd = vm["command"].as<std::string>();

  int exitCode = EXIT_SUCCESS;

  if(auto it = commands.find(cmd); it != commands.end()) {
    auto& command = it->second;
    auto opts = po::collect_unrecognized(parsed.options, po::include_positional);

    // drop the command from the options
    opts.erase(opts.begin());

    // Get the subcommand-specific commandline options from the command entry and parse again
    std::stringstream output;
    try {
      auto [desc, pos] = command->options;
      po::store(po::command_line_parser(opts).options(desc).positional(pos).style(style).run(), vm);

      // If the user requested help for the command, print it and just exit
      if(vm.count("help") > 0) {
        if(command->help) {
          std::cout << "mtca4u [--json] " << cmd << " " << *command->help << std::endl;
        }
        std::cout << global << std::endl;
        std::cout << desc << std::endl;
        return 0;
      }

      po::notify(vm);

      command->operator()(output, vm);
    }
    catch(std::exception& ex) {
      command->setError(ex.what());
      exitCode = EXIT_FAILURE;
    }

    if(vm.contains("json")) {
      std::cout << command->output << std::endl;
    }
    else {
      std::cout << output.str() << std::endl;
    }

    return exitCode;
  }

  commands["help"]->operator()(std::cout, vm);

  return EXIT_SUCCESS;
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

// Implementations

/**********************************************************************************************************************/

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

void readRegisterInternal(const po::variables_map& args) {
#if 0
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
#endif
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
#if 0
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
#endif
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
#if 0
  boost::shared_ptr<ChimeraTK::Device> device = getDevice(deviceName);
  auto deMuxedData = device->getTwoDRegisterAccessor<double>(module + "/" + regionName);
  deMuxedData.read();
  return deMuxedData;
#endif
  return {};
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

/**********************************************************************************************************************/
