import SwiftUI

struct ContentView: View {
    @EnvironmentObject private var host: EmulatorHost
    @State private var selectedDiagnostic: DiagnosticTab = .cpu

    private let screenWidth: CGFloat = 720
    private let screenHeight: CGFloat = 540

    var body: some View {
        ZStack(alignment: .topLeading) {
            HSplitView {
                leftPane
                    .frame(minWidth: 760, idealWidth: 820)

                rightPane
                    .frame(minWidth: 520, idealWidth: 600)
            }

            keyboardCapture
        }
        .padding(16)
        .frame(minWidth: 1280, minHeight: 820)
    }

    private var leftPane: some View {
        VStack(alignment: .leading, spacing: 14) {
            header
            screenPanel
            frontLCDPanel
            statusPanel
            controlsPanel
        }
        .padding(.trailing, 8)
    }

    private var rightPane: some View {
        VStack(alignment: .leading, spacing: 12) {
            diagnosticsHeader
            diagnosticsPicker
            diagnosticsBody
        }
        .padding(.leading, 8)
    }

    private var header: some View {
        HStack(alignment: .firstTextBaseline) {
            VStack(alignment: .leading, spacing: 2) {
                Text("IBM PC110 EMU")
                    .font(.system(.title2, design: .rounded).weight(.semibold))
                Text("BIOS bring-up harness")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            Spacer()
            Text("Gradual boot")
                .font(.system(.caption, design: .monospaced).weight(.semibold))
                .padding(.horizontal, 10)
                .padding(.vertical, 5)
                .background(.quaternary, in: Capsule())
        }
    }

    private var screenPanel: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Label("Display", systemImage: "display")
                    .font(.headline)
                Spacer()
                Text("640 × 480")
                    .font(.caption.monospaced())
                    .foregroundStyle(.secondary)
            }

            ZStack {
                RoundedRectangle(cornerRadius: 14, style: .continuous)
                    .fill(Color.black)
                if host.image != nil {
                    PC110DisplayView(
                        image: host.image,
                        pixelWidth: 640,
                        pixelHeight: 480,
                        onMouseMoved: { host.mouseMoved(x: $0, y: $1) },
                        onMouseDown: { host.mouseDown(x: $0, y: $1, button: $2) },
                        onMouseUp: { host.mouseUp(x: $0, y: $1, button: $2) },
                        onTextInput: { host.textInput($0) },
                        onKeyDown: { host.keyDown($0) },
                        onKeyUp: { host.keyUp($0) }
                    )
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                } else {
                    Text("No framebuffer")
                        .foregroundStyle(.secondary)
                }
            }
            .frame(width: screenWidth, height: screenHeight)
            .overlay(
                RoundedRectangle(cornerRadius: 14, style: .continuous)
                    .stroke(.separator, lineWidth: 1)
            )
        }
    }

    private var frontLCDPanel: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Label("Front LCD", systemImage: "rectangle.inset.filled")
                    .font(.headline)
                Spacer()
                Text("M3822x power/status MCU")
                    .font(.caption.monospaced())
                    .foregroundStyle(.secondary)
            }

            PC110FrontLCDView(state: host.frontLCD)
        }
        .frame(width: screenWidth)
    }

    private var statusPanel: some View {
        HStack(spacing: 8) {
            Image(systemName: "waveform.path.ecg")
                .foregroundStyle(.secondary)
            Text(host.status)
                .font(.caption)
                .lineLimit(2)
                .textSelection(.enabled)
            Spacer()
        }
        .padding(10)
        .background(.quaternary, in: RoundedRectangle(cornerRadius: 10, style: .continuous))
    }

    private var controlsPanel: some View {
        VStack(alignment: .leading, spacing: 12) {
            ControlSection(title: "Session") {
                ControlButton(
                    host.continuousRunEnabled ? "Pause Run" : "Continue Run",
                    systemImage: host.continuousRunEnabled ? "pause.fill" : "play.fill",
                    variant: .primary,
                    help: "Run continuously with PC DOS boot screens paced for visibility, then switch to 8 MHz runtime."
                ) {
                    host.toggleContinuousRun()
                }
                ControlButton(
                    "Easy Setup",
                    systemImage: "gearshape.fill",
                    variant: .primary,
                    help: "Open the ROM-backed Easy Setup screen."
                ) {
                    host.enterEasySetup()
                }
                ControlButton("Reset", systemImage: "arrow.counterclockwise", help: "Reset the machine and reload BIOS/boot assets.") { host.reset() }
            }

            ControlSection(title: "BIOS Input") {
                ControlButton(
                    "F1 Setup",
                    systemImage: "keyboard",
                    variant: .accent,
                    help: "Enter ROM Easy Setup through the BIOS F1 path."
                ) {
                    host.induceF1AndCopyStatusBundleToClipboard()
                }
                ControlButton("Clear Trace", systemImage: "trash", variant: .destructive, help: "Clear the accumulated trace log.") { host.clearTrace() }
            }

            ControlSection(title: "Diagnostics") {
                ControlButton("Copy Bundle", systemImage: "doc.on.clipboard", help: "Copy CPU, trace tail, and text-screen diagnostics.") { host.copyStatusBundleToClipboard() }
                ControlButton("Copy Text", systemImage: "text.rectangle", help: "Copy the current text-screen diagnostics.") { host.copyTextScreenToClipboard() }
            }
        }
        .padding(12)
        .background(.thinMaterial, in: RoundedRectangle(cornerRadius: 14, style: .continuous))
    }

    private var diagnosticsHeader: some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                Text("Diagnostics")
                    .font(.headline)
                Text("Copy-ready CPU, trace, memory, and text-screen state")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            Spacer()
            Button("Copy Bundle") { host.copyStatusBundleToClipboard() }
        }
    }

    private var diagnosticsPicker: some View {
        Picker("Diagnostics", selection: $selectedDiagnostic) {
            ForEach(DiagnosticTab.allCases) { tab in
                Label(tab.title, systemImage: tab.systemImage).tag(tab)
            }
        }
        .pickerStyle(.segmented)
    }

    @ViewBuilder
    private var diagnosticsBody: some View {
        switch selectedDiagnostic {
        case .cpu:
            DiagnosticPanel(title: "CPU", systemImage: "cpu", copyTitle: "Copy CPU") {
                host.copyCPUStateToClipboard()
            } content: {
                MonospaceScrollText(host.cpuState)
            }
        case .trace:
            DiagnosticPanel(title: "Trace Tail", systemImage: "list.bullet.rectangle", copyTitle: "Copy Trace") {
                host.copyTraceTailToClipboard()
            } content: {
                ScrollViewReader { proxy in
                    ScrollView {
                        Text(host.traceText)
                            .font(.system(.caption, design: .monospaced))
                            .frame(maxWidth: .infinity, alignment: .leading)
                            .textSelection(.enabled)
                            .id("traceEnd")
                    }
                    .onChange(of: host.traceText) { _ in
                        proxy.scrollTo("traceEnd", anchor: .bottom)
                    }
                }
            }
        case .memory:
            DiagnosticPanel(title: "Memory", systemImage: "memorychip", copyTitle: "Copy Memory") {
                host.copyMemoryDumpToClipboard()
            } content: {
                VStack(alignment: .leading, spacing: 8) {
                    HStack {
                        TextField("Address", text: $host.memoryAddress)
                            .font(.system(.body, design: .monospaced))
                            .textFieldStyle(.roundedBorder)
                            .frame(width: 140)
                        Button("Read") { host.refreshMemoryDump() }
                        Spacer()
                    }
                    MonospaceScrollText(host.memoryDump)
                }
            }
        case .text:
            DiagnosticPanel(title: "Text Screen", systemImage: "text.rectangle", copyTitle: "Copy Text") {
                host.copyTextScreenToClipboard()
            } content: {
                VStack(alignment: .leading, spacing: 8) {
                    HStack {
                        Button("Refresh") { host.refreshTextScreen() }
                        Spacer()
                    }
                    MonospaceScrollText(host.textScreen)
                }
            }
        }
    }

    private var keyboardCapture: some View {
        KeyboardCaptureView(
            onTextInput: { host.textInput($0) },
            onKeyDown: { host.keyDown($0) },
            onKeyUp: { host.keyUp($0) }
        )
        .frame(width: 1, height: 1)
        .accessibilityHidden(true)
    }
}

private enum DiagnosticTab: String, CaseIterable, Identifiable {
    case cpu
    case trace
    case memory
    case text

    var id: String { rawValue }

    var title: String {
        switch self {
        case .cpu: return "CPU"
        case .trace: return "Trace"
        case .memory: return "Memory"
        case .text: return "Text"
        }
    }

    var systemImage: String {
        switch self {
        case .cpu: return "cpu"
        case .trace: return "list.bullet.rectangle"
        case .memory: return "memorychip"
        case .text: return "text.rectangle"
        }
    }
}

private struct ControlSection<Content: View>: View {
    let title: String
    @ViewBuilder let content: Content

    private let columns = [
        GridItem(.adaptive(minimum: 118), spacing: 8, alignment: .leading)
    ]

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text(title)
                .font(.caption.weight(.semibold))
                .foregroundStyle(.secondary)
            LazyVGrid(columns: columns, alignment: .leading, spacing: 8) {
                content
            }
        }
    }
}

private struct ControlButton: View {
    let title: String
    let systemImage: String
    let variant: ControlButtonVariant
    let help: String?
    let action: () -> Void

    init(
        _ title: String,
        systemImage: String,
        variant: ControlButtonVariant = .standard,
        help: String? = nil,
        action: @escaping () -> Void
    ) {
        self.title = title
        self.systemImage = systemImage
        self.variant = variant
        self.help = help
        self.action = action
    }

    var body: some View {
        Button(action: action) {
            Label(title, systemImage: systemImage)
                .font(.system(.caption, design: .rounded).weight(.semibold))
                .lineLimit(1)
                .minimumScaleFactor(0.82)
                .frame(maxWidth: .infinity, alignment: .leading)
                .frame(height: 30)
        }
        .buttonStyle(ControlButtonStyle(variant: variant))
        .help(help ?? title)
    }
}

private enum ControlButtonVariant {
    case standard
    case primary
    case accent
    case destructive

    var foreground: Color {
        switch self {
        case .standard:
            return .primary
        case .primary, .accent, .destructive:
            return .white
        }
    }

    var background: Color {
        switch self {
        case .standard:
            return Color(nsColor: .controlBackgroundColor)
        case .primary:
            return .accentColor
        case .accent:
            return Color(nsColor: .systemIndigo)
        case .destructive:
            return Color(nsColor: .systemRed)
        }
    }

    var border: Color {
        switch self {
        case .standard:
            return Color(nsColor: .separatorColor)
        case .primary, .accent, .destructive:
            return .clear
        }
    }
}

private struct ControlButtonStyle: ButtonStyle {
    let variant: ControlButtonVariant

    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .padding(.horizontal, 9)
            .foregroundStyle(variant.foreground)
            .background(
                RoundedRectangle(cornerRadius: 8, style: .continuous)
                    .fill(variant.background.opacity(configuration.isPressed ? 0.78 : 1.0))
            )
            .overlay(
                RoundedRectangle(cornerRadius: 8, style: .continuous)
                    .stroke(variant.border, lineWidth: 1)
            )
            .contentShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
    }
}

private struct DiagnosticPanel<Content: View>: View {
    let title: String
    let systemImage: String
    let copyTitle: String
    let copyAction: () -> Void
    @ViewBuilder let content: Content

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Label(title, systemImage: systemImage)
                    .font(.headline)
                Spacer()
                Button(copyTitle, action: copyAction)
            }
            content
                .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .padding(12)
        .background(Color(nsColor: .textBackgroundColor), in: RoundedRectangle(cornerRadius: 14, style: .continuous))
        .overlay(
            RoundedRectangle(cornerRadius: 14, style: .continuous)
                .stroke(.separator, lineWidth: 1)
        )
    }
}

private struct MonospaceScrollText: View {
    let text: String

    init(_ text: String) {
        self.text = text
    }

    var body: some View {
        ScrollView {
            Text(text)
                .font(.system(.caption, design: .monospaced))
                .frame(maxWidth: .infinity, alignment: .leading)
                .textSelection(.enabled)
                .padding(8)
        }
        .background(Color(nsColor: .controlBackgroundColor), in: RoundedRectangle(cornerRadius: 10, style: .continuous))
    }
}
