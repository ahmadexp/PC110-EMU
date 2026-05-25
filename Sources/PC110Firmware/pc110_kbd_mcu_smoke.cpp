#include "PC110Firmware/PC110FirmwareModel.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

using pc110::firmware::FirmwareImageInfo;
using pc110::firmware::KeyboardControllerModel;
using pc110::firmware::loadKeyboardControllerMCUFirmwareImage;

namespace {

void expect(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

} // namespace

int main(int argc, char **argv) {
    const std::string path = argc > 1 ? argv[1] : "Roms/M38813E4HP@QFP64.bin";
    FirmwareImageInfo image = loadKeyboardControllerMCUFirmwareImage(path);

    expect(image.loaded, "keyboard MCU firmware image loads");
    expect(image.size == 16255u, "keyboard MCU firmware size matches known dump");
    expect(image.versionMajor == 1u, "keyboard MCU major version is 1");
    expect(image.versionMinor == 1u, "keyboard MCU minor version is 1");
    expect(image.id.find("MELPS 740 Series Keyboard Firmware") == 0u, "keyboard MCU banner recognized");

    KeyboardControllerModel kbc(image);

    auto selfTest = kbc.command(0xAAu);
    expect(selfTest.handled && !selfTest.carry, "KBC self-test command handled");
    expect(kbc.responsePending(), "KBC self-test queues response");
    expect(kbc.readResponse() == 0x55u, "KBC self-test response is 55h");

    auto interfaceTest = kbc.command(0xABu);
    expect(interfaceTest.handled && !interfaceTest.carry, "KBC keyboard interface test handled");
    expect(kbc.responsePending(), "KBC interface test queues response");
    expect(kbc.readResponse() == 0x00u, "KBC interface test response is 00h");

    kbc.command(0xADu);
    expect(kbc.keyboardDisabled(), "KBC disable keyboard command updates state");
    kbc.command(0xAEu);
    expect(!kbc.keyboardDisabled(), "KBC enable keyboard command updates state");

    kbc.command(0xA7u);
    expect(kbc.auxDisabled(), "KBC disable aux command updates state");
    kbc.command(0xA8u);
    expect(!kbc.auxDisabled(), "KBC enable aux command updates state");

    kbc.writeCommandByte(0x30u);
    expect(kbc.keyboardDisabled(), "KBC command byte bit 4 disables keyboard");
    expect(kbc.auxDisabled(), "KBC command byte bit 5 disables aux");
    kbc.command(0x20u);
    expect(kbc.responsePending(), "KBC read command byte queues response");
    expect(kbc.readResponse() == 0x30u, "KBC read command byte response matches written byte");

    expect(kbc.selfTestCount() == 1u, "KBC self-test counter increments");
    expect(kbc.interfaceTestCount() == 1u, "KBC keyboard interface-test counter increments");
    expect(kbc.responseReadCount() == 3u, "KBC response-read counter increments");

    std::printf("keyboard MCU clean-room model ok: %s size=%u version=%u.%u checksum=%08X\n",
                image.id.c_str(),
                static_cast<unsigned>(image.size),
                static_cast<unsigned>(image.versionMajor),
                static_cast<unsigned>(image.versionMinor),
                static_cast<unsigned>(image.checksum));
    return 0;
}
