#include "PC110Firmware/PC110FirmwareModel.hpp"

#include <cstdio>
#include <cstdlib>

using pc110::firmware::BIOSPostPhase;
using pc110::firmware::DiskGeometry;
using pc110::firmware::InterruptRequest;
using pc110::firmware::LCDMode;
using pc110::firmware::PC110BiosModel;

namespace {

void expect(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

} // namespace

int main() {
    PC110BiosModel bios;
    bios.powerOn(true);
    bios.setClock(12, 53, 15);
    bios.setDate(2026, 5, 24);
    bios.setSecondsSincePowerOn(0.25);

    expect(bios.state().biosLoaded, "BIOS starts loaded");
    expect(bios.state().phase == BIOSPostPhase::ResetVector, "BIOS starts at reset vector phase");
    expect(bios.frontLCD().mode == LCDMode::StartupLogo, "front LCD shows startup logo after power-on");

    DiskGeometry disk;
    disk.name = "Disk1.img";
    disk.cylinders = 80;
    disk.heads = 2;
    disk.sectorsPerTrack = 18;
    disk.biosDrive = 0x00;
    bios.attachBootMedia(disk);

    bios.advancePOST();
    bios.advancePOST();
    bios.advancePOST();
    expect(bios.state().phase == BIOSPostPhase::BootStrap, "POST advances to bootstrap");
    bios.advancePOST();
    expect(bios.state().phase == BIOSPostPhase::Runtime, "bootstrap advances to runtime");

    auto geometry = bios.interrupt({0x13, 0x0800, 0, 0, 0x0000, 0, 0});
    expect(geometry.handled && !geometry.carry, "INT13 geometry handled");
    expect((geometry.dx & 0x00FFu) == 0x00u, "INT13 geometry reports drive 00h");
    expect((geometry.dx >> 8u) == 1u, "INT13 geometry reports last head 1");

    auto zeroRead = bios.interrupt({0x13, 0x0200, 0, 0, 0x0000, 0, 0});
    expect(zeroRead.handled && zeroRead.carry, "INT13 zero-sector read fails");

    bios.queueKey('A', 0x1E);
    auto peek = bios.interrupt({0x16, 0x0100, 0, 0, 0, 0, 0});
    expect(peek.handled && !peek.zero && peek.ax == 0x1E41u, "INT16 peek sees queued key");
    expect(bios.pendingKeyCount() == 1u, "INT16 peek does not consume key");
    auto read = bios.interrupt({0x16, 0x0000, 0, 0, 0, 0, 0});
    expect(read.handled && !read.zero && read.ax == 0x1E41u, "INT16 read consumes queued key");
    expect(bios.pendingKeyCount() == 0u, "INT16 read drains key");

    auto store = bios.interrupt({0x16, 0x0500, 0, 0x3042, 0, 0, 0});
    expect(store.handled && bios.pendingKeyCount() == 1u, "INT16 store queues key from CX");

    bios.interrupt({0x15, 0x2400, 0, 0, 0, 0, 0});
    expect(!bios.state().a20Enabled, "INT15 disable A20 updates BIOS state");
    auto queryA20 = bios.interrupt({0x15, 0x2402, 0, 0, 0, 0, 0});
    expect(queryA20.ax == 0x0000u, "INT15 query A20 reports disabled");
    bios.interrupt({0x15, 0x2401, 0, 0, 0, 0, 0});
    queryA20 = bios.interrupt({0x15, 0x2402, 0, 0, 0, 0, 0});
    expect(queryA20.ax == 0x0001u, "INT15 query A20 reports enabled");

    auto time = bios.interrupt({0x1A, 0x0200, 0, 0, 0, 0, 0});
    expect(time.cx == 0x1253u && time.dx == 0x1500u, "INT1A time returns BCD clock");
    auto date = bios.interrupt({0x1A, 0x0400, 0, 0, 0, 0, 0});
    expect(date.cx == 0x2026u && date.dx == 0x0524u, "INT1A date returns BCD date");

    bios.requestEasySetup();
    bios.powerOn(true);
    bios.requestEasySetup();
    bios.advancePOST();
    bios.advancePOST();
    bios.advancePOST();
    expect(bios.state().phase == BIOSPostPhase::EasySetup, "setup request diverts POST to Easy Setup");
    expect(bios.easySetupState().active, "Easy Setup model is active");

    bios.setSecondsSincePowerOn(3.0);
    auto lcd = bios.frontLCD();
    expect(lcd.mode == LCDMode::Status, "front LCD switches to status after startup window");
    expect(lcd.detailText == "SETUP", "front LCD reports setup while Easy Setup is active");

    std::puts("PC110 BIOS clean-room model ok");
    return 0;
}
