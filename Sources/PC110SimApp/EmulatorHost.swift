import Foundation
import AppKit
import PC110Core

final class EmulatorHost: ObservableObject {
    private var machine: OpaquePointer?
    private var timer: Timer?
    private var keyEventMonitor: Any?
    private let speakerAudio = PCSpeakerAudio()

    @Published var image: NSImage?
    @Published var traceText: String = ""
    @Published var memoryAddress: String = "000FFFF0"
    @Published var memoryDump: String = ""
    @Published var cpuState: String = ""
    @Published var textScreen: String = ""
    @Published var status: String = "Not started"
    @Published var continuousRunEnabled: Bool = false

    private let width = Int(pc110_framebuffer_width())
    private let height = Int(pc110_framebuffer_height())
    private let targetInstructionsPerSecond = 25_000_000.0
    private let maxContinuousRunSlice: Int32 = 2_500_000
    private let maxContinuousRunCatchupSeconds = 0.10
    private let keyEchoInstructionBudget: Int32 = 250_000
    private let enterCommandInstructionBudget: Int32 = 1_500_000
    private let mouseClickInstructionBudget: Int32 = 3_000_000
    private let textInputMaxInstructionBudget: Int32 = 1_500_000
    private var lastRunTickTime: TimeInterval?
    private var pendingContinuousRunInstructions = 0.0
    private var runReportStartTime: TimeInterval?
    private var runReportInstructions: Int64 = 0

    private func attachBootAssets(to machine: OpaquePointer, cwd: String) -> String? {
        let zipPath = "\(cwd)/Disks/img.ZIP"
        let attachedZip = pc110_attach_boot_zip(machine, zipPath) != 0 ? zipPath : nil
        let homePQI = FileManager.default.homeDirectoryForCurrentUser
            .appendingPathComponent("Desktop/Personaware.PQI")
            .path
        let candidates = [
            "\(cwd)/Disks/Personaware.PQI",
            "\(cwd)/Disks/Disk1.PQI",
            "\(cwd)/Disks/disk1.pqi",
            "\(cwd)/Disks/disk1.qpi",
            "\(cwd)/Disks/Disk1.qpi",
            "\(cwd)/Disks/Disk1.QPI",
            homePQI,
            "\(cwd)/Disks/Disk1.img",
            "\(cwd)/Disks/disk.img",
        ]
        for path in candidates {
            if pc110_attach_boot_image(machine, path) != 0 {
                return path
            }
        }
        return attachedZip
    }

    deinit {
        stop()
        speakerAudio.stop()
        if let machine {
            pc110_destroy(machine)
        }
    }

    func start() {
        guard machine == nil else { return }

        guard let created = pc110_create() else {
            status = "Failed to create PC110 machine"
            return
        }
        machine = created
        pc110_cpu_set_trace_mode(created, 0)

        let cwd = FileManager.default.currentDirectoryPath
        let biosPath = "\(cwd)/Roms/pc110_bios.bin"
        let loaded = pc110_load_bios(created, biosPath)
        let bootDiskPath = attachBootAssets(to: created, cwd: cwd)

        if loaded != 0 {
            let bootDiskName = bootDiskPath.map { URL(fileURLWithPath: $0).lastPathComponent } ?? "none"
            status = "BIOS loaded: \(pc110_bios_size(created)) bytes. Boot disk: \(bootDiskName). Press Continue Run to boot at 486SX 25 MHz speed."
        } else {
            status = "No BIOS loaded. Put Roms/pc110_bios.bin in the package directory."
        }

        resetRunPacing()
        refreshAll()
        installKeyEventMonitor()

        timer = Timer.scheduledTimer(withTimeInterval: 1.0 / 30.0, repeats: true) { [weak self] _ in
            self?.tick()
        }
    }

    func stop() {
        timer?.invalidate()
        timer = nil
        removeKeyEventMonitor()
        speakerAudio.stop()
    }

    func reset() {
        guard let machine else { return }
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_reset(machine)
        pc110_cpu_set_trace_mode(machine, 0)
        let cwd = FileManager.default.currentDirectoryPath
        _ = pc110_load_bios(machine, "\(cwd)/Roms/pc110_bios.bin")
        let bootDiskPath = attachBootAssets(to: machine, cwd: cwd)
        resetRunPacing()
        refreshAll()
        let bootDiskName = bootDiskPath.map { URL(fileURLWithPath: $0).lastPathComponent } ?? "none"
        status = continuousRunEnabled
            ? "Reset. Boot disk: \(bootDiskName). Continuous run is targeting 486SX 25 MHz speed."
            : "Reset. Boot disk: \(bootDiskName). Press Continue Run to boot at 486SX 25 MHz speed."
    }

    func clearTrace() {
        guard let machine else { return }
        pc110_trace_clear(machine)
        refreshTrace()
    }

    func testIO() {
        guard let machine else { return }
        pc110_io_write8(machine, 0x21, 0xF8)
        _ = pc110_io_read8(machine, 0x21)
        pc110_io_write8(machine, 0x70, 0x14)
        _ = pc110_io_read8(machine, 0x71)
        pc110_io_write8(machine, 0x226, 0x01)
        _ = pc110_io_read8(machine, 0x22E)
        _ = pc110_io_read8(machine, 0x1234)
        refreshAll()
    }

    func testMemory() {
        guard let machine else { return }
        pc110_mem_write8(machine, 0x00001000, 0x50)
        pc110_mem_write8(machine, 0x00001001, 0x43)
        pc110_mem_write8(machine, 0x00001002, 0x31)
        pc110_mem_write8(machine, 0x00001003, 0x31)
        memoryAddress = "00001000"
        refreshAll()
    }

    func stepCPU() {
        guard let machine else { return }
        pc110_cpu_step(machine, 1)
        refreshAll()
    }

    func runCPU100() {
        guard let machine else { return }
        pc110_cpu_step(machine, 100)
        refreshAll()
    }

    func runCPU1000() {
        guard let machine else { return }
        pc110_cpu_step(machine, 1000)
        refreshAll()
    }

    func runCPU100000() {
        guard let machine else { return }
        pc110_cpu_step(machine, 100000)
        refreshAll()
    }

    func postSprint() {
        guard let machine else { return }
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_step(machine, 500000)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
    }

    func postSprintMillion() {
        guard let machine else { return }
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_step(machine, 1000000)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
    }

    func bootFast() {
        guard let machine else { return }
        pc110_cpu_reset(machine)
        pc110_trace_clear(machine)
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_step(machine, 10000000)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
        status = "Fast boot run complete from reset: 10,000,000 instructions"
    }

    func bootTurbo() {
        guard let machine else { return }
        pc110_cpu_reset(machine)
        pc110_trace_clear(machine)
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_step(machine, 30000000)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
        status = "Turbo boot run complete from reset: 30,000,000 instructions"
    }

    func bootUltra() {
        guard let machine else { return }
        pc110_cpu_reset(machine)
        pc110_trace_clear(machine)
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_step(machine, 300000000)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
        status = "Ultra boot run complete from reset: 300,000,000 instructions"
    }

    func continueRun100M() {
        guard let machine else { return }
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_step(machine, 100000000)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
        status = "Continued current state: 100,000,000 instructions"
    }

    func continueRun300M() {
        guard let machine else { return }
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_step(machine, 300000000)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
        status = "Continued current state: 300,000,000 instructions"
    }

    func runTraced() {
        guard let machine else { return }
        pc110_cpu_set_trace_mode(machine, 1)
        pc110_cpu_step(machine, 10000)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
    }

    func runCPU10000() {
        guard let machine else { return }
        pc110_cpu_step(machine, 10000)
        refreshAll()
    }


    func enterEasySetup() {
        guard let machine else { return }
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_enter_easy_setup(machine)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshInteractiveState()
        status = "Opened ROM-backed graphical Easy Setup"
    }

    func easySetupAndCopyStatusBundleToClipboard() {
        guard let machine else { return }
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_reset(machine)
        pc110_trace_clear(machine)
        pc110_enter_easy_setup(machine)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
        copyStatusBundleToClipboard()
        status = "EASY SETUP+COPY: opened ROM-backed graphical menu and copied status bundle"
    }

    func induceF1AndCopyStatusBundleToClipboard() {
        guard let machine else { return }
        pc110_trace_clear(machine)
        pc110_induce_f1(machine)
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_step(machine, 1000000)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
        copyStatusBundleToClipboard()
        status = "INDUCE F1+COPY: armed F1 scancode, ran 1,000,000 instructions, copied status bundle"
    }

    func resetCPUOnly() {
        guard let machine else { return }
        pc110_cpu_reset(machine)
        refreshAll()
    }


    func copyTraceToClipboard() {
        refreshTrace()
        let pasteboard = NSPasteboard.general
        pasteboard.clearContents()
        pasteboard.setString(traceText, forType: .string)
        status = "Copied full trace: \(traceText.count) characters"
    }

    func copyTraceTailToClipboard() {
        refreshTrace()
        let tail = String(traceText.suffix(12000))
        let pasteboard = NSPasteboard.general
        pasteboard.clearContents()
        pasteboard.setString(tail, forType: .string)
        status = "Copied trace tail: \(tail.count) characters"
    }

    func exportTraceToDesktop() {
        refreshTrace()
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyyMMdd-HHmmss"
        let name = "PC110EMU-trace-\(formatter.string(from: Date())).txt"
        let desktop = FileManager.default.urls(for: .desktopDirectory, in: .userDomainMask).first
        guard let url = desktop?.appendingPathComponent(name) else {
            status = "Could not find Desktop for trace export."
            return
        }
        do {
            try traceText.write(to: url, atomically: true, encoding: .utf8)
            status = "Trace exported to Desktop: \(name)"
        } catch {
            status = "Trace export failed: \(error.localizedDescription)"
        }
    }

    func copyStatusBundleToClipboard() {
        refreshCPUState()
        refreshTrace()
        refreshTextScreen()
        let tail = String(traceText.suffix(12000))
        let bundle = "=== CPU ===\n\(cpuState)\n\n=== TRACE TAIL ===\n\(tail)\n\n=== TEXT SCREEN ===\n\(textScreen)"
        let pasteboard = NSPasteboard.general
        pasteboard.clearContents()
        pasteboard.setString(bundle, forType: .string)
        status = "Copied CPU + trace tail + text screen"
    }

    func startAndCopyStatusBundleToClipboard() {
        guard let machine else { return }
        pc110_cpu_reset(machine)
        pc110_trace_clear(machine)
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_step(machine, 5000000)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
        copyStatusBundleToClipboard()
        status = "START+COPY: reset, ran 5,000,000 instructions, copied status bundle"
    }

    func nextAndCopyStatusBundleToClipboard() {
        guard let machine else { return }
        pc110_trace_clear(machine)
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_step(machine, 5000000)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
        copyStatusBundleToClipboard()
        status = "NEXT+COPY: continued current state for 5,000,000 instructions, copied status bundle"
    }

    func next25AndCopyStatusBundleToClipboard() {
        guard let machine else { return }
        pc110_trace_clear(machine)
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_step(machine, 25000000)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
        copyStatusBundleToClipboard()
        status = "NEXT25+COPY: continued current state for 25,000,000 instructions, copied status bundle"
    }

    func postBootAndCopyStatusBundleToClipboard() {
        guard let machine else { return }
        pc110_cpu_reset(machine)
        pc110_trace_clear(machine)
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_step(machine, 5000000)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
        copyStatusBundleToClipboard()
        status = "POST booted from reset for 5,000,000 instructions and copied status bundle"
    }

    func continue5MAndCopyStatusBundleToClipboard() {
        guard let machine else { return }
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_step(machine, 5000000)
        pc110_cpu_set_trace_mode(machine, 0)
        refreshAll()
        copyStatusBundleToClipboard()
        status = "Continued 5,000,000 instructions and copied status bundle"
    }

    func bootAndCopyStatusBundleToClipboard() {
        bootFast()
        copyStatusBundleToClipboard()
        status = "Fast booted from reset and copied CPU + trace tail + text screen"
    }

    func turboBootAndCopyStatusBundleToClipboard() {
        bootTurbo()
        copyStatusBundleToClipboard()
        status = "Turbo booted from reset and copied CPU + trace tail + text screen"
    }

    func ultraBootAndCopyStatusBundleToClipboard() {
        bootUltra()
        copyStatusBundleToClipboard()
        status = "Ultra booted from reset and copied CPU + trace tail + text screen"
    }

    func continue100MAndCopyStatusBundleToClipboard() {
        continueRun100M()
        copyStatusBundleToClipboard()
        status = "Continued 100M instructions and copied CPU + trace tail + text screen"
    }

    func continue300MAndCopyStatusBundleToClipboard() {
        continueRun300M()
        copyStatusBundleToClipboard()
        status = "Continued 300M instructions and copied CPU + trace tail + text screen"
    }

    func copyCPUStateToClipboard() {
        refreshCPUState()
        let pasteboard = NSPasteboard.general
        pasteboard.clearContents()
        pasteboard.setString(cpuState, forType: .string)
    }

    func copyMemoryDumpToClipboard() {
        refreshMemoryDump()
        let pasteboard = NSPasteboard.general
        pasteboard.clearContents()
        pasteboard.setString(memoryDump, forType: .string)
    }

    func copyTextScreenToClipboard() {
        refreshTextScreen()
        let pasteboard = NSPasteboard.general
        pasteboard.clearContents()
        pasteboard.setString(textScreen, forType: .string)
        status = "Copied text screen dump: \(textScreen.count) characters"
    }

    func refreshAll() {
        refreshFramebuffer()
        refreshAudio()
        refreshTrace()
        refreshMemoryDump()
        refreshCPUState()
        refreshTextScreen()
    }

    private func refreshInteractiveState() {
        refreshFramebuffer()
        refreshAudio()
    }

    private func tick() {
        guard let machine else { return }
        let now = ProcessInfo.processInfo.systemUptime
        if continuousRunEnabled {
            let budget = continuousRunBudget(now: now)
            if budget > 0 {
                runUntraced(instructions: budget)
                reportContinuousRun(instructions: budget, now: ProcessInfo.processInfo.systemUptime)
            } else {
                pc110_run_frame(machine)
            }
        } else {
            lastRunTickTime = now
            pendingContinuousRunInstructions = 0
            pc110_run_frame(machine)
        }
        refreshFramebuffer()
        refreshAudio()
    }

    func toggleContinuousRun() {
        continuousRunEnabled.toggle()
        resetRunPacing()
        status = continuousRunEnabled
            ? "Continuous run started: targeting 486SX 25 MHz speed."
            : "Continuous run paused."
    }

    private func resetRunPacing() {
        let now = ProcessInfo.processInfo.systemUptime
        lastRunTickTime = now
        pendingContinuousRunInstructions = 0
        runReportStartTime = now
        runReportInstructions = 0
    }

    private func continuousRunBudget(now: TimeInterval) -> Int32 {
        let previous = lastRunTickTime ?? now
        lastRunTickTime = now

        let elapsed = min(max(now - previous, 0), maxContinuousRunCatchupSeconds)
        pendingContinuousRunInstructions += targetInstructionsPerSecond * elapsed

        let capped = min(pendingContinuousRunInstructions, Double(maxContinuousRunSlice))
        let budget = Int32(capped)
        pendingContinuousRunInstructions -= Double(budget)
        return budget
    }

    private func reportContinuousRun(instructions: Int32, now: TimeInterval) {
        let start = runReportStartTime ?? now
        runReportInstructions += Int64(instructions)

        let elapsed = now - start
        guard elapsed >= 1.0 else { return }

        let effectiveMHz = Double(runReportInstructions) / elapsed / 1_000_000.0
        status = String(format: "Continuous run: %.1f MHz effective, targeting 486SX 25 MHz speed.", effectiveMHz)
        runReportStartTime = now
        runReportInstructions = 0
    }

    private func runUntraced(instructions: Int32) {
        guard let machine, instructions > 0 else { return }
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_cpu_step(machine, instructions)
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_run_frame(machine)
    }

    private func installKeyEventMonitor() {
        guard keyEventMonitor == nil else { return }
        keyEventMonitor = NSEvent.addLocalMonitorForEvents(matching: [.keyDown, .keyUp]) { [weak self] event in
            guard let self else { return event }
            return self.handleLocalKeyEvent(event)
        }
    }

    private func removeKeyEventMonitor() {
        if let keyEventMonitor {
            NSEvent.removeMonitor(keyEventMonitor)
            self.keyEventMonitor = nil
        }
    }

    private func handleLocalKeyEvent(_ event: NSEvent) -> NSEvent? {
        guard machine != nil else { return event }
        guard !isMacTextInputActive(for: event) else { return event }

        switch event.type {
        case .keyDown:
            if let text = dosControlInput(from: event) ?? dosTextInput(from: event) {
                textInput(text)
                return nil
            }
            guard handlesRawMacKeyCode(event.keyCode, modifiers: event.modifierFlags) else { return event }
            keyDown(event.keyCode)
            return nil
        case .keyUp:
            guard handlesRawMacKeyCode(event.keyCode, modifiers: event.modifierFlags) else { return event }
            keyUp(event.keyCode)
            return nil
        default:
            return event
        }
    }

    private func isMacTextInputActive(for event: NSEvent) -> Bool {
        guard let responder = event.window?.firstResponder else { return false }
        if responder is NSTextView { return true }
        let responderName = String(describing: type(of: responder))
        return responderName.contains("FieldEditor") || responderName.contains("TextView")
    }

    private func dosTextInput(from event: NSEvent) -> String? {
        let blockedModifiers: NSEvent.ModifierFlags = [.command, .option, .control]
        guard event.modifierFlags.intersection(blockedModifiers).isEmpty else { return nil }
        guard event.keyCode != 49 else { return nil }
        guard let characters = event.characters, !characters.isEmpty else { return nil }

        for scalar in characters.unicodeScalars {
            guard scalar.value >= 0x21 && scalar.value <= 0x7E else { return nil }
        }
        return characters
    }

    private func dosControlInput(from event: NSEvent) -> String? {
        let blockedModifiers: NSEvent.ModifierFlags = [.command, .option]
        guard event.modifierFlags.intersection(blockedModifiers).isEmpty else { return nil }
        guard event.modifierFlags.contains(.control) else { return nil }
        guard let characters = event.characters, !characters.isEmpty else { return nil }

        for scalar in characters.unicodeScalars {
            guard scalar.value >= 0x01 && scalar.value <= 0x1A else { return nil }
        }
        return characters
    }

    private func handlesRawMacKeyCode(_ keyCode: UInt16, modifiers: NSEvent.ModifierFlags) -> Bool {
        let blockedModifiers: NSEvent.ModifierFlags = [.command, .option, .control]
        guard modifiers.intersection(blockedModifiers).isEmpty else { return false }

        switch keyCode {
        case 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16, 17,
             18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
             32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
             46, 47, 48, 49, 50, 51, 53, 76, 96, 97, 98, 99, 100, 101,
             109, 117, 118, 120, 122, 123, 124, 125, 126:
            return true
        default:
            return false
        }
    }

    private func refreshAudio() {
        guard let machine else { return }
        speakerAudio.update(
            enabled: pc110_speaker_enabled(machine) != 0,
            frequency: pc110_speaker_frequency(machine),
            eventCount: pc110_speaker_event_count(machine),
            eventFrequency: pc110_speaker_event_frequency(machine)
        )
    }

    private func refreshFramebuffer() {
        guard let machine else { return }
        guard let fb = pc110_get_framebuffer(machine) else { return }

        let pixelCount = width * height
        let data = Data(bytes: fb, count: pixelCount * MemoryLayout<UInt32>.size)

        guard let provider = CGDataProvider(data: data as CFData) else { return }
        let bitmapInfo = CGBitmapInfo(rawValue:
            CGImageAlphaInfo.noneSkipFirst.rawValue |
            CGBitmapInfo.byteOrder32Little.rawValue
        )
        guard let cg = CGImage(
            width: width,
            height: height,
            bitsPerComponent: 8,
            bitsPerPixel: 32,
            bytesPerRow: width * 4,
            space: CGColorSpaceCreateDeviceRGB(),
            bitmapInfo: bitmapInfo,
            provider: provider,
            decode: nil,
            shouldInterpolate: false,
            intent: .defaultIntent
        ) else { return }

        image = NSImage(cgImage: cg, size: NSSize(width: width, height: height))
    }

    func refreshTrace() {
        guard let machine else { return }
        var buffer = [CChar](repeating: 0, count: 1024 * 1024)
        let bufferCount = buffer.count
        buffer.withUnsafeMutableBufferPointer { ptr in
            _ = pc110_trace_copy(machine, ptr.baseAddress, bufferCount)
        }
        traceText = String(cString: buffer)
    }

    func refreshMemoryDump() {
        guard let machine else { return }
        let trimmed = memoryAddress.trimmingCharacters(in: .whitespacesAndNewlines)
        let start = UInt32(trimmed, radix: 16) ?? 0x000FFFF0

        var buffer = [CChar](repeating: 0, count: 8192)
        let bufferCount = buffer.count
        buffer.withUnsafeMutableBufferPointer { ptr in
            _ = pc110_debug_format_memory(machine, start, 256, ptr.baseAddress, bufferCount)
        }
        memoryDump = String(cString: buffer)
    }

    func refreshTextScreen() {
        guard let machine else { return }
        var buffer = [CChar](repeating: 0, count: 8192)
        let bufferCount = buffer.count
        buffer.withUnsafeMutableBufferPointer { ptr in
            _ = pc110_debug_format_text_screen(machine, ptr.baseAddress, bufferCount)
        }
        textScreen = String(cString: buffer)
    }

    func refreshCPUState() {
        guard let machine else { return }
        var buffer = [CChar](repeating: 0, count: 16 * 1024)
        let bufferCount = buffer.count
        buffer.withUnsafeMutableBufferPointer { ptr in
            _ = pc110_cpu_format_state(machine, ptr.baseAddress, bufferCount)
        }
        cpuState = String(cString: buffer)
    }

    func keyDown(_ macKeyCode: UInt16) {
        guard let machine else { return }
        let easySetupActive = pc110_easy_setup_active(machine) != 0
        pc110_cpu_set_trace_mode(machine, 0)
        pc110_key_down(machine, macKeyCode)
        pc110_cpu_set_trace_mode(machine, 0)
        status = "Queued DOS key code \(macKeyCode)"
        if !easySetupActive {
            runUntraced(instructions: keyBudget(for: macKeyCode))
        }
        refreshInteractiveState()
    }

    func keyUp(_ macKeyCode: UInt16) {
        guard let machine else { return }
        pc110_key_up(machine, macKeyCode)
    }

    func textInput(_ text: String) {
        guard let machine else { return }

        var accepted = 0
        let easySetupActive = pc110_easy_setup_active(machine) != 0
        pc110_cpu_set_trace_mode(machine, 0)
        for scalar in text.unicodeScalars {
            guard scalar.value <= UInt8.max else { continue }
            if pc110_key_ascii(machine, UInt8(scalar.value)) != 0 {
                accepted += 1
            }
        }
        pc110_cpu_set_trace_mode(machine, 0)

        guard accepted > 0 else { return }
        status = "Queued DOS text input"
        if !easySetupActive {
            let budget = min(textInputMaxInstructionBudget, keyEchoInstructionBudget * Int32(accepted))
            runUntraced(instructions: budget)
        }
        refreshInteractiveState()
    }

    private func keyBudget(for macKeyCode: UInt16) -> Int32 {
        switch macKeyCode {
        case 36, 76:
            return enterCommandInstructionBudget
        default:
            return keyEchoInstructionBudget
        }
    }

    func mouseMoved(x: Int32, y: Int32) {
        guard let machine else { return }
        pc110_mouse_move(machine, x, y)
        refreshFramebuffer()
    }

    func mouseDown(x: Int32, y: Int32, button: Int32) {
        guard let machine else { return }
        let easySetupActive = pc110_easy_setup_active(machine) != 0
        pc110_mouse_down(machine, x, y, button)
        if !easySetupActive {
            runUntraced(instructions: keyEchoInstructionBudget)
        }
        refreshInteractiveState()
    }

    func mouseUp(x: Int32, y: Int32, button: Int32) {
        guard let machine else { return }
        let easySetupActive = pc110_easy_setup_active(machine) != 0
        pc110_mouse_up(machine, x, y, button)
        if !easySetupActive {
            runUntraced(instructions: mouseClickInstructionBudget)
        }
        refreshInteractiveState()
    }
}
