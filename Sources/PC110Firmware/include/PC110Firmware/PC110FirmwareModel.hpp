#ifndef PC110_FIRMWARE_MODEL_HPP
#define PC110_FIRMWARE_MODEL_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace pc110::firmware {

struct FirmwareImageInfo {
    bool loaded = false;
    std::string id;
    std::uint32_t size = 0;
    std::uint32_t checksum = 0;
    std::uint8_t revision = 0;
    std::uint8_t versionMajor = 0;
    std::uint8_t versionMinor = 0;
    std::vector<std::uint8_t> tailBytes;
};

FirmwareImageInfo loadPowerSenseMCUFirmwareImage(const std::string &path);
FirmwareImageInfo loadKeyboardControllerMCUFirmwareImage(const std::string &path);

struct MachineObservation {
    bool biosLoaded = false;
    bool powerMCULoaded = false;
    bool keyboardMCULoaded = false;
    bool bootMediaAttached = false;
    bool speakerActive = false;
    bool easySetupActive = false;
    bool continuousRun = false;
    bool visualBootActive = false;
    std::uint8_t powerMCURevision = 0;
    std::string bootMediaName = "none";
    double hostSecondsSincePowerOn = 0.0;
    unsigned hour = 0;
    unsigned minute = 0;
};

enum class LCDMode {
    Off,
    StartupLogo,
    Status
};

struct LCDIndicator {
    std::string label;
    bool active = false;
};

struct LCDFrame {
    LCDMode mode = LCDMode::Off;
    std::string primaryText;
    std::string detailText;
    std::vector<LCDIndicator> indicators;
};

enum class EasySetupPage {
    Config,
    DateTime,
    Password,
    Startup,
    Test,
    Restart
};

struct EasySetupState {
    bool active = false;
    EasySetupPage selected = EasySetupPage::Config;
    bool detailOpen = false;
    bool restartConfirmOpen = false;
};

struct InterruptRequest {
    std::uint8_t number = 0;
    std::uint16_t ax = 0;
    std::uint16_t bx = 0;
    std::uint16_t cx = 0;
    std::uint16_t dx = 0;
    std::uint16_t es = 0;
    std::uint16_t flags = 0;
};

struct InterruptResult {
    bool handled = false;
    bool carry = false;
    bool zero = false;
    std::uint16_t ax = 0;
    std::uint16_t bx = 0;
    std::uint16_t cx = 0;
    std::uint16_t dx = 0;
    std::uint16_t es = 0;
    std::string note;
};

class FrontLCDModel {
public:
    explicit FrontLCDModel(double startupLogoSeconds = 2.5);

    LCDFrame render(const MachineObservation &observation) const;

private:
    double startupLogoSeconds_;
};

class PowerSenseMCUModel {
public:
    explicit PowerSenseMCUModel(FirmwareImageInfo image = {});

    void setImage(FirmwareImageInfo image);
    const FirmwareImageInfo &image() const;
    void selectIndex(std::uint8_t index);
    std::uint8_t readSelectedIndex();
    std::uint8_t selectedIndex() const;
    std::uint8_t readSelectedValue();
    void writeSelectedValue(std::uint8_t value);
    std::uint64_t indexReads() const;
    std::uint64_t dataReads() const;
    std::uint64_t indexWrites() const;
    std::uint64_t dataWrites() const;

private:
    FirmwareImageInfo image_;
    std::uint8_t selectedIndex_ = 0;
    std::uint64_t indexReads_ = 0;
    std::uint64_t dataReads_ = 0;
    std::uint64_t indexWrites_ = 0;
    std::uint64_t dataWrites_ = 0;
};

class KeyboardControllerModel {
public:
    explicit KeyboardControllerModel(FirmwareImageInfo image = {});

    void setImage(FirmwareImageInfo image);
    const FirmwareImageInfo &image() const;
    InterruptResult command(std::uint8_t commandByte);
    void writeCommandByte(std::uint8_t commandByte);
    std::uint8_t commandByte() const;
    bool responsePending() const;
    std::uint8_t readResponse();
    std::uint64_t commandCount() const;
    std::uint64_t responseReadCount() const;
    std::uint64_t selfTestCount() const;
    std::uint64_t interfaceTestCount() const;
    bool keyboardDisabled() const;
    bool auxDisabled() const;

private:
    void queueResponse(std::uint8_t value);

    FirmwareImageInfo image_;
    std::uint8_t commandByte_ = 0x45;
    std::uint8_t pendingResponse_ = 0;
    bool responsePending_ = false;
    std::uint64_t commandCount_ = 0;
    std::uint64_t responseReadCount_ = 0;
    std::uint64_t selfTestCount_ = 0;
    std::uint64_t interfaceTestCount_ = 0;
    bool keyboardDisabled_ = false;
    bool auxDisabled_ = false;
};

class EasySetupModel {
public:
    const EasySetupState &state() const;
    void enter();
    void leave();
    void select(EasySetupPage page);
    void activateSelected();
    void cancelDetail();
    static const char *pageTitle(EasySetupPage page);

private:
    EasySetupState state_;
};

class BIOSServiceModel {
public:
    InterruptResult handle(const InterruptRequest &request, const MachineObservation &observation) const;

private:
    InterruptResult handleInt13(const InterruptRequest &request, const MachineObservation &observation) const;
    InterruptResult handleInt15(const InterruptRequest &request) const;
    InterruptResult handleInt16(const InterruptRequest &request) const;
    InterruptResult handleInt1A(const InterruptRequest &request, const MachineObservation &observation) const;
};

} // namespace pc110::firmware

#endif
