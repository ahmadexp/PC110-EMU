#include "PC110Firmware/PC110FirmwareModel.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>

namespace pc110::firmware {

namespace {

std::string twoDigit(unsigned value) {
    char text[8];
    std::snprintf(text, sizeof(text), "%02u", value % 100u);
    return text;
}

std::uint8_t lowByte(std::uint16_t value) {
    return static_cast<std::uint8_t>(value & 0x00FFu);
}

std::uint8_t highByte(std::uint16_t value) {
    return static_cast<std::uint8_t>((value >> 8) & 0x00FFu);
}

std::uint16_t makeWord(std::uint8_t high, std::uint8_t low) {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(high) << 8) | low);
}

std::uint8_t toBCD(unsigned value) {
    value %= 100u;
    return static_cast<std::uint8_t>(((value / 10u) << 4u) | (value % 10u));
}

std::uint32_t checksum(const std::vector<std::uint8_t> &data) {
    std::uint32_t sum = 0;
    for (std::uint8_t byte : data) {
        sum = (sum << 5u) | (sum >> 27u);
        sum ^= byte;
        sum += 0x9E3779B9u;
    }
    return sum;
}

std::uint8_t parsePowerSenseRevision(const std::vector<std::uint8_t> &data) {
    const char marker[] = "Rev ";
    if (data.empty()) return 0;
    for (std::size_t i = 0; i + sizeof(marker) < data.size(); i++) {
        if (std::memcmp(data.data() + i, marker, sizeof(marker) - 1u) == 0) {
            const std::uint8_t c = data[i + sizeof(marker) - 1u];
            if (c >= '0' && c <= '9') return static_cast<std::uint8_t>(c - '0');
        }
    }
    return 0;
}

std::string captureCStringBanner(const std::vector<std::uint8_t> &data) {
    std::string id;
    for (std::uint8_t byte : data) {
        if (byte < 0x20u || byte > 0x7Eu) break;
        id.push_back(static_cast<char>(byte));
        if (id.size() >= 95u) break;
    }

    const std::string company = "LTD.";
    const std::size_t end = id.find(company);
    if (end != std::string::npos) {
        id.resize(end + company.size());
    }
    return id.empty() ? "M3822x power-sense firmware" : id;
}

std::vector<std::uint8_t> readBinaryFile(const std::string &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    return {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
}

std::size_t findBytes(const std::vector<std::uint8_t> &data, const char *needle) {
    const std::size_t needleLength = std::strlen(needle);
    if (needleLength == 0u || data.size() < needleLength) return std::string::npos;
    for (std::size_t i = 0; i + needleLength <= data.size(); i++) {
        if (std::memcmp(data.data() + i, needle, needleLength) == 0) return i;
    }
    return std::string::npos;
}

std::string captureKeyboardBanner(const std::vector<std::uint8_t> &data, std::size_t offset) {
    if (offset == std::string::npos || offset >= data.size()) {
        return "MELPS 740 keyboard firmware";
    }

    std::string id;
    for (std::size_t i = offset; i < data.size() && id.size() < 127u; i++) {
        const std::uint8_t byte = data[i];
        if (byte < 0x20u || byte > 0x7Eu) break;
        id.push_back(static_cast<char>(byte));
    }

    const std::string company = "Co.,Ltd.";
    const std::size_t end = id.find(company);
    if (end != std::string::npos) {
        id.resize(end + company.size());
    }
    return id.empty() ? "MELPS 740 keyboard firmware" : id;
}

void parseKeyboardVersion(const std::string &id, std::uint8_t &major, std::uint8_t &minor) {
    major = 0;
    minor = 0;
    const std::string marker = "Version ";
    const std::size_t pos = id.find(marker);
    if (pos == std::string::npos) return;
    const std::size_t version = pos + marker.size();
    if (version >= id.size() || id[version] < '0' || id[version] > '9') return;
    major = static_cast<std::uint8_t>(id[version] - '0');
    if (version + 2u < id.size() && id[version + 1u] == '.' &&
        id[version + 2u] >= '0' && id[version + 2u] <= '9') {
        minor = static_cast<std::uint8_t>(id[version + 2u] - '0');
    }
}

InterruptResult ok(std::string note) {
    InterruptResult result;
    result.handled = true;
    result.carry = false;
    result.note = std::move(note);
    return result;
}

InterruptResult fail(std::string note) {
    InterruptResult result;
    result.handled = true;
    result.carry = true;
    result.note = std::move(note);
    return result;
}

} // namespace

FirmwareImageInfo loadPowerSenseMCUFirmwareImage(const std::string &path) {
    FirmwareImageInfo info;
    std::vector<std::uint8_t> data = readBinaryFile(path);
    if (data.empty()) return info;

    info.loaded = true;
    info.id = captureCStringBanner(data);
    info.size = static_cast<std::uint32_t>(data.size());
    info.checksum = checksum(data);
    info.revision = parsePowerSenseRevision(data);

    const std::size_t tailCount = std::min<std::size_t>(16u, data.size());
    info.tailBytes.assign(data.end() - static_cast<std::ptrdiff_t>(tailCount), data.end());
    return info;
}

FirmwareImageInfo loadKeyboardControllerMCUFirmwareImage(const std::string &path) {
    FirmwareImageInfo info;
    std::vector<std::uint8_t> data = readBinaryFile(path);
    if (data.empty()) return info;

    static const char banner[] = "MELPS 740 Series Keyboard Firmware";
    const std::size_t bannerOffset = findBytes(data, banner);

    info.loaded = true;
    info.id = captureKeyboardBanner(data, bannerOffset);
    info.size = static_cast<std::uint32_t>(data.size());
    info.checksum = checksum(data);
    parseKeyboardVersion(info.id, info.versionMajor, info.versionMinor);

    const std::size_t tailCount = std::min<std::size_t>(16u, data.size());
    info.tailBytes.assign(data.end() - static_cast<std::ptrdiff_t>(tailCount), data.end());
    return info;
}

FrontLCDModel::FrontLCDModel(double startupLogoSeconds)
    : startupLogoSeconds_(startupLogoSeconds) {}

LCDFrame FrontLCDModel::render(const MachineObservation &observation) const {
    LCDFrame frame;
    if (!observation.biosLoaded && !observation.powerMCULoaded) {
        frame.mode = LCDMode::Off;
        frame.primaryText = "--:--";
        frame.detailText = "NO POWER";
        return frame;
    }

    const bool startupLogo = observation.hostSecondsSincePowerOn >= 0.0 &&
                             observation.hostSecondsSincePowerOn < startupLogoSeconds_;
    frame.mode = startupLogo ? LCDMode::StartupLogo : LCDMode::Status;
    frame.primaryText = startupLogo ? "IBM" : twoDigit(observation.hour) + ":" + twoDigit(observation.minute);
    frame.detailText = observation.easySetupActive ? "SETUP" : observation.bootMediaName;
    frame.indicators = {
        {observation.continuousRun ? (observation.visualBootActive ? "BOOT" : "RUN") : "HOLD", observation.biosLoaded},
        {observation.powerMCULoaded ? "PMCU R" + std::to_string(observation.powerMCURevision) : "PMCU", observation.powerMCULoaded},
        {"KBC", observation.keyboardMCULoaded},
        {"DSK", observation.bootMediaAttached},
        {"SND", observation.speakerActive},
        {"SET", observation.easySetupActive},
    };
    return frame;
}

PowerSenseMCUModel::PowerSenseMCUModel(FirmwareImageInfo image)
    : image_(std::move(image)) {}

void PowerSenseMCUModel::setImage(FirmwareImageInfo image) {
    image_ = std::move(image);
}

const FirmwareImageInfo &PowerSenseMCUModel::image() const {
    return image_;
}

void PowerSenseMCUModel::selectIndex(std::uint8_t index) {
    selectedIndex_ = index;
    indexWrites_++;
}

std::uint8_t PowerSenseMCUModel::readSelectedIndex() {
    indexReads_++;
    return selectedIndex_;
}

std::uint8_t PowerSenseMCUModel::selectedIndex() const {
    return selectedIndex_;
}

std::uint8_t PowerSenseMCUModel::readSelectedValue() {
    dataReads_++;
    const std::uint8_t index = selectedIndex_;

    if (!image_.loaded) return 0x00u;

    if (index >= 0x80u && index < 0xE0u) {
        const std::size_t offset = static_cast<std::size_t>(index - 0x80u);
        return offset < image_.id.size() ? static_cast<std::uint8_t>(image_.id[offset]) : 0x00u;
    }

    if (index >= 0xE0u && index < 0xF0u) {
        const std::size_t offset = static_cast<std::size_t>(index - 0xE0u);
        return offset < image_.tailBytes.size() ? image_.tailBytes[offset] : 0x00u;
    }

    switch (index) {
    case 0xF0u: return 'M';
    case 0xF1u: return 'C';
    case 0xF2u: return 'U';
    case 0xF3u: return image_.revision;
    case 0xF4u: return static_cast<std::uint8_t>(image_.size & 0xFFu);
    case 0xF5u: return static_cast<std::uint8_t>((image_.size >> 8u) & 0xFFu);
    case 0xF6u: return static_cast<std::uint8_t>((image_.size >> 16u) & 0xFFu);
    case 0xF7u: return static_cast<std::uint8_t>(image_.checksum & 0xFFu);
    case 0xF8u: return static_cast<std::uint8_t>((image_.checksum >> 8u) & 0xFFu);
    case 0xF9u: return 0x81u;
    case 0xFAu: return static_cast<std::uint8_t>((image_.checksum >> 16u) & 0xFFu);
    case 0xFBu: return static_cast<std::uint8_t>((image_.checksum >> 24u) & 0xFFu);
    case 0xFCu: return static_cast<std::uint8_t>(std::min<std::size_t>(image_.id.size(), 255u));
    case 0xFEu: return static_cast<std::uint8_t>(dataReads_ & 0xFFu);
    case 0xFFu: return 0xA5u;
    default: return 0x00u;
    }
}

void PowerSenseMCUModel::writeSelectedValue(std::uint8_t) {
    dataWrites_++;
}

std::uint64_t PowerSenseMCUModel::indexReads() const {
    return indexReads_;
}

std::uint64_t PowerSenseMCUModel::dataReads() const {
    return dataReads_;
}

std::uint64_t PowerSenseMCUModel::indexWrites() const {
    return indexWrites_;
}

std::uint64_t PowerSenseMCUModel::dataWrites() const {
    return dataWrites_;
}

KeyboardControllerModel::KeyboardControllerModel(FirmwareImageInfo image)
    : image_(std::move(image)) {}

void KeyboardControllerModel::setImage(FirmwareImageInfo image) {
    image_ = std::move(image);
}

const FirmwareImageInfo &KeyboardControllerModel::image() const {
    return image_;
}

InterruptResult KeyboardControllerModel::command(std::uint8_t commandByte) {
    commandCount_++;
    InterruptResult result = ok("KBC command accepted");
    switch (commandByte) {
    case 0x20u:
        queueResponse(commandByte_);
        result.ax = commandByte_;
        result.note = "read command byte";
        break;
    case 0xAAu:
        selfTestCount_++;
        queueResponse(0x55u);
        result.ax = 0x0055u;
        result.note = "controller self test";
        break;
    case 0xABu:
        interfaceTestCount_++;
        queueResponse(0x00u);
        result.ax = 0x0000u;
        result.note = "keyboard interface test";
        break;
    case 0xA7u:
        auxDisabled_ = true;
        result.note = "aux disabled";
        break;
    case 0xA8u:
        auxDisabled_ = false;
        result.note = "aux enabled";
        break;
    case 0xA9u:
        queueResponse(0x00u);
        result.ax = 0x0000u;
        result.note = "aux interface test";
        break;
    case 0xADu:
        keyboardDisabled_ = true;
        result.note = "keyboard disabled";
        break;
    case 0xAEu:
        keyboardDisabled_ = false;
        result.note = "keyboard enabled";
        break;
    case 0xD1u:
        result.note = "next data byte writes output port";
        break;
    case 0xD4u:
        result.note = "next data byte writes aux device";
        break;
    default:
        result.note = "benign KBC command";
        break;
    }
    return result;
}

void KeyboardControllerModel::writeCommandByte(std::uint8_t commandByte) {
    commandByte_ = commandByte;
    keyboardDisabled_ = (commandByte & 0x10u) != 0;
    auxDisabled_ = (commandByte & 0x20u) != 0;
}

std::uint8_t KeyboardControllerModel::commandByte() const {
    return commandByte_;
}

bool KeyboardControllerModel::responsePending() const {
    return responsePending_;
}

std::uint8_t KeyboardControllerModel::readResponse() {
    if (!responsePending_) return 0x00u;
    responsePending_ = false;
    responseReadCount_++;
    return pendingResponse_;
}

std::uint64_t KeyboardControllerModel::commandCount() const {
    return commandCount_;
}

std::uint64_t KeyboardControllerModel::responseReadCount() const {
    return responseReadCount_;
}

std::uint64_t KeyboardControllerModel::selfTestCount() const {
    return selfTestCount_;
}

std::uint64_t KeyboardControllerModel::interfaceTestCount() const {
    return interfaceTestCount_;
}

bool KeyboardControllerModel::keyboardDisabled() const {
    return keyboardDisabled_;
}

bool KeyboardControllerModel::auxDisabled() const {
    return auxDisabled_;
}

void KeyboardControllerModel::queueResponse(std::uint8_t value) {
    pendingResponse_ = value;
    responsePending_ = true;
}

const EasySetupState &EasySetupModel::state() const {
    return state_;
}

void EasySetupModel::enter() {
    state_.active = true;
    state_.selected = EasySetupPage::Config;
    state_.detailOpen = false;
    state_.restartConfirmOpen = false;
}

void EasySetupModel::leave() {
    state_ = {};
}

void EasySetupModel::select(EasySetupPage page) {
    state_.selected = page;
}

void EasySetupModel::activateSelected() {
    if (state_.selected == EasySetupPage::Restart) {
        state_.restartConfirmOpen = true;
    } else {
        state_.detailOpen = true;
    }
}

void EasySetupModel::cancelDetail() {
    state_.detailOpen = false;
    state_.restartConfirmOpen = false;
}

const char *EasySetupModel::pageTitle(EasySetupPage page) {
    switch (page) {
    case EasySetupPage::Config: return "Config";
    case EasySetupPage::DateTime: return "Date/Time";
    case EasySetupPage::Password: return "Password";
    case EasySetupPage::Startup: return "Start up";
    case EasySetupPage::Test: return "Test";
    case EasySetupPage::Restart: return "Restart";
    }
    return "Config";
}

InterruptResult BIOSServiceModel::handle(const InterruptRequest &request, const MachineObservation &observation) const {
    switch (request.number) {
    case 0x13u: return handleInt13(request, observation);
    case 0x15u: return handleInt15(request);
    case 0x16u: return handleInt16(request);
    case 0x1Au: return handleInt1A(request, observation);
    default: {
        InterruptResult result;
        result.note = "interrupt not modeled";
        return result;
    }
    }
}

InterruptResult BIOSServiceModel::handleInt13(const InterruptRequest &request, const MachineObservation &observation) const {
    const std::uint8_t ah = highByte(request.ax);
    InterruptResult result = observation.bootMediaAttached ? ok("INT13 disk service") : fail("INT13 no boot media");
    result.ax = request.ax;
    result.bx = request.bx;
    result.cx = request.cx;
    result.dx = request.dx;
    result.es = request.es;

    if (!observation.bootMediaAttached) {
        result.ax = makeWord(0x01u, lowByte(request.ax));
        return result;
    }

    switch (ah) {
    case 0x00u:
        result.ax = makeWord(0x00u, lowByte(request.ax));
        result.note = "INT13 reset success";
        break;
    case 0x02u:
        if (lowByte(request.ax) == 0u) {
            result = fail("INT13 zero-sector read rejected");
            result.ax = makeWord(0x01u, lowByte(request.ax));
        } else {
            result.ax = makeWord(0x00u, lowByte(request.ax));
            result.note = "INT13 read request accepted";
        }
        break;
    case 0x08u:
        result.cx = 0x4F12u;
        result.dx = makeWord(1u, lowByte(request.dx));
        result.note = "INT13 drive parameters";
        break;
    case 0x15u:
        result.ax = makeWord(0x03u, lowByte(request.ax));
        result.note = "INT13 DASD type";
        break;
    default:
        result = fail("INT13 function not modeled");
        result.ax = makeWord(0x01u, lowByte(request.ax));
        break;
    }
    return result;
}

InterruptResult BIOSServiceModel::handleInt15(const InterruptRequest &request) const {
    const std::uint16_t ax = request.ax;
    const std::uint8_t ah = highByte(ax);
    InterruptResult result = ok("INT15 service");
    result.ax = request.ax;
    result.bx = request.bx;
    result.cx = request.cx;
    result.dx = request.dx;
    result.es = request.es;

    switch (ax) {
    case 0x2101u:
        result.note = "PC110 private AX=2101 accepted";
        break;
    case 0x2400u:
        result.note = "A20 disabled";
        break;
    case 0x2401u:
        result.note = "A20 enabled";
        break;
    case 0x2402u:
        result.ax = 0x0001u;
        result.note = "A20 queried enabled";
        break;
    case 0x2403u:
        result.ax = 0x0003u;
        result.note = "A20 keyboard+fast supported";
        break;
    case 0xE801u:
        result.ax = 0x3C00u;
        result.bx = 0x0050u;
        result.cx = result.ax;
        result.dx = result.bx;
        result.note = "extended memory geometry";
        break;
    default:
        if (ah == 0x88u) {
            result.ax = 0x3C00u;
            result.note = "extended memory below 16M";
        } else if (ah == 0xC0u) {
            result.es = 0x0000u;
            result.bx = 0x0500u;
            result.note = "system configuration table pointer";
        } else if (ax == 0x5000u || ax == 0x5380u) {
            result = fail("PC110 private wait/event service has no event");
            result.ax = makeWord(0x86u, lowByte(request.ax));
        } else {
            result.note = "benign INT15 success";
        }
        break;
    }
    return result;
}

InterruptResult BIOSServiceModel::handleInt16(const InterruptRequest &request) const {
    const std::uint8_t ah = highByte(request.ax);
    InterruptResult result = ok("INT16 keyboard service");
    result.ax = request.ax;

    switch (ah) {
    case 0x00u:
        result.ax = 0x0000u;
        result.zero = true;
        result.note = "keyboard read empty";
        break;
    case 0x01u:
        result.ax = 0x0000u;
        result.zero = true;
        result.note = "keyboard peek empty";
        break;
    case 0x02u:
    case 0x12u:
        result.ax = 0x0000u;
        result.note = "shift flags clear";
        break;
    case 0x03u:
        result.note = "typematic/private keyboard service accepted";
        break;
    case 0x05u:
        result.note = "store keystroke accepted";
        break;
    default:
        result.zero = true;
        result.note = "keyboard no-key response";
        break;
    }
    return result;
}

InterruptResult BIOSServiceModel::handleInt1A(const InterruptRequest &request, const MachineObservation &observation) const {
    const std::uint8_t ah = highByte(request.ax);
    InterruptResult result = ok("INT1A RTC service");

    switch (ah) {
    case 0x00u: {
        const std::uint32_t ticks = static_cast<std::uint32_t>(observation.hostSecondsSincePowerOn * 18.2065);
        result.cx = static_cast<std::uint16_t>((ticks >> 16u) & 0xFFFFu);
        result.dx = static_cast<std::uint16_t>(ticks & 0xFFFFu);
        result.ax = request.ax & 0xFF00u;
        result.note = "BIOS tick count";
        break;
    }
    case 0x02u:
        result.cx = makeWord(toBCD(observation.hour), toBCD(observation.minute));
        result.dx = 0x0000u;
        result.note = "RTC time";
        break;
    case 0x04u:
        result.cx = 0x2026u;
        result.dx = 0x0524u;
        result.note = "RTC date placeholder";
        break;
    default:
        result.note = "benign INT1A success";
        break;
    }
    return result;
}

} // namespace pc110::firmware
