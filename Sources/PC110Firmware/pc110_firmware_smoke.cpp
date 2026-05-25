#include "PC110Firmware/PC110FirmwareModel.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

using pc110::firmware::FirmwareImageInfo;
using pc110::firmware::FrontLCDModel;
using pc110::firmware::LCDMode;
using pc110::firmware::MachineObservation;
using pc110::firmware::PowerSenseMCUModel;
using pc110::firmware::loadPowerSenseMCUFirmwareImage;

namespace {

void expect(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

std::uint8_t readIndex(PowerSenseMCUModel &mcu, std::uint8_t index) {
    mcu.selectIndex(index);
    return mcu.readSelectedValue();
}

} // namespace

int main(int argc, char **argv) {
    const std::string path = argc > 1 ? argv[1] : "Roms/M38223E4HP@QFP80.BIN";
    FirmwareImageInfo image = loadPowerSenseMCUFirmwareImage(path);

    expect(image.loaded, "power MCU firmware image loads");
    expect(image.size == 16254u, "power MCU firmware size matches known dump");
    expect(image.revision == 8u, "power MCU firmware revision is Rev 8");
    expect(image.id.find("M3822X POWER SENSE MICON FIRMWARE") == 0u, "power MCU banner recognized");

    PowerSenseMCUModel mcu(image);
    expect(readIndex(mcu, 0xF0u) == 'M', "indexed F0 returns M");
    expect(readIndex(mcu, 0xF1u) == 'C', "indexed F1 returns C");
    expect(readIndex(mcu, 0xF2u) == 'U', "indexed F2 returns U");
    expect(readIndex(mcu, 0xF3u) == 8u, "indexed F3 returns revision");
    expect(readIndex(mcu, 0xF4u) == static_cast<std::uint8_t>(image.size & 0xFFu), "indexed F4 returns size low byte");
    expect(readIndex(mcu, 0xF9u) == 0x81u, "indexed F9 returns loaded/status marker");
    expect(readIndex(mcu, 0xFFu) == 0xA5u, "indexed FF returns sentinel");

    mcu.selectIndex(0x80u);
    expect(mcu.readSelectedIndex() == 0x80u, "index readback returns selected index");
    expect(mcu.readSelectedValue() == 'M', "indexed firmware id starts with M");
    expect(mcu.indexReads() == 1u, "index read counter increments");
    expect(mcu.dataReads() >= 8u, "data read counter increments");

    MachineObservation observation;
    observation.biosLoaded = true;
    observation.powerMCULoaded = true;
    observation.keyboardMCULoaded = true;
    observation.bootMediaAttached = true;
    observation.powerMCURevision = image.revision;
    observation.bootMediaName = "Disk1.img";
    observation.continuousRun = true;
    observation.hostSecondsSincePowerOn = 0.5;
    observation.hour = 18;
    observation.minute = 55;

    FrontLCDModel lcd;
    auto startup = lcd.render(observation);
    expect(startup.mode == LCDMode::StartupLogo, "LCD shows startup logo during initial window");
    expect(startup.primaryText == "IBM", "LCD startup text is IBM");

    observation.hostSecondsSincePowerOn = 3.0;
    auto status = lcd.render(observation);
    expect(status.mode == LCDMode::Status, "LCD switches to status after startup window");
    expect(status.primaryText == "18:55", "LCD status clock formats host time");

    std::printf("power MCU clean-room model ok: %s size=%u rev=%u checksum=%08X\n",
                image.id.c_str(),
                static_cast<unsigned>(image.size),
                static_cast<unsigned>(image.revision),
                static_cast<unsigned>(image.checksum));
    return 0;
}
