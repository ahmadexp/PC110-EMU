import SwiftUI

struct PC110FrontLCDState: Equatable {
    var time: Date?
    var runMode: String
    var detail: String
    var powerMCULoaded: Bool
    var powerMCURevision: UInt8
    var keyboardMCULoaded: Bool
    var speakerActive: Bool
    var diskAttached: Bool
    var easySetupActive: Bool
    var startupLogoActive: Bool

    static let inactive = PC110FrontLCDState(
        time: nil,
        runMode: "OFF",
        detail: "NO POWER",
        powerMCULoaded: false,
        powerMCURevision: 0,
        keyboardMCULoaded: false,
        speakerActive: false,
        diskAttached: false,
        easySetupActive: false,
        startupLogoActive: false
    )
}

struct PC110FrontLCDView: View {
    let state: PC110FrontLCDState

    private var timeText: String {
        if state.startupLogoActive { return "IBM" }
        guard let time = state.time else { return "--:--" }
        return Self.timeFormatter.string(from: time)
    }

    private var powerMCUTitle: String {
        state.powerMCULoaded ? "PMCU R\(state.powerMCURevision)" : "PMCU"
    }

    private static let timeFormatter: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "HH:mm"
        return formatter
    }()

    var body: some View {
        HStack(spacing: 14) {
            lcdGlass

            VStack(alignment: .leading, spacing: 6) {
                HStack(spacing: 6) {
                    FrontLCDIndicator(title: state.runMode, systemImage: "power", isActive: state.time != nil)
                    FrontLCDIndicator(title: powerMCUTitle, systemImage: "bolt.horizontal", isActive: state.powerMCULoaded)
                    FrontLCDIndicator(title: "KBC", systemImage: "keyboard", isActive: state.keyboardMCULoaded)
                    FrontLCDIndicator(title: "DSK", systemImage: "externaldrive", isActive: state.diskAttached)
                    FrontLCDIndicator(title: "SND", systemImage: "speaker.wave.2", isActive: state.speakerActive)
                    FrontLCDIndicator(title: "SET", systemImage: "slider.horizontal.3", isActive: state.easySetupActive)
                }

                Text(state.detail)
                    .font(.system(.caption2, design: .monospaced).weight(.semibold))
                    .foregroundStyle(.secondary)
                    .lineLimit(1)
                    .truncationMode(.middle)
            }

            Spacer(minLength: 0)
        }
        .padding(10)
        .background(
            RoundedRectangle(cornerRadius: 8, style: .continuous)
                .fill(Color(nsColor: .windowBackgroundColor))
        )
        .overlay(
            RoundedRectangle(cornerRadius: 8, style: .continuous)
                .stroke(.separator, lineWidth: 1)
        )
    }

    private var lcdGlass: some View {
        HStack(spacing: 4) {
            ForEach(Array(timeText.enumerated()), id: \.offset) { _, character in
                if character == ":" {
                    SevenSegmentColon()
                } else {
                    SevenSegmentCharacter(character: character)
                }
            }
        }
        .padding(.horizontal, 14)
        .padding(.vertical, 9)
        .background(
            RoundedRectangle(cornerRadius: 5, style: .continuous)
                .fill(
                    LinearGradient(
                        colors: [
                            Color(red: 0.48, green: 0.52, blue: 0.42),
                            Color(red: 0.69, green: 0.72, blue: 0.58)
                        ],
                        startPoint: .top,
                        endPoint: .bottom
                    )
                )
        )
        .overlay(
            RoundedRectangle(cornerRadius: 5, style: .continuous)
                .stroke(Color.black.opacity(0.42), lineWidth: 2)
        )
        .shadow(color: .black.opacity(0.22), radius: 1, x: 0, y: 1)
    }
}

private struct FrontLCDIndicator: View {
    let title: String
    let systemImage: String
    let isActive: Bool

    var body: some View {
        Label(title, systemImage: systemImage)
            .font(.system(size: 10, weight: .bold, design: .monospaced))
            .labelStyle(.titleAndIcon)
            .foregroundStyle(isActive ? Color.primary : Color.secondary.opacity(0.45))
            .padding(.horizontal, 6)
            .frame(height: 20)
            .background(
                RoundedRectangle(cornerRadius: 5, style: .continuous)
                    .fill(isActive ? Color(nsColor: .controlBackgroundColor) : Color.clear)
            )
            .overlay(
                RoundedRectangle(cornerRadius: 5, style: .continuous)
                    .stroke(isActive ? Color(nsColor: .separatorColor) : Color.secondary.opacity(0.18), lineWidth: 1)
            )
            .help(title)
    }
}

private struct SevenSegmentCharacter: View {
    let character: Character

    private var activeSegments: Set<Int> {
        switch character {
        case "0": return [0, 1, 2, 3, 4, 5]
        case "1": return [1, 2]
        case "2": return [0, 1, 6, 4, 3]
        case "3": return [0, 1, 6, 2, 3]
        case "4": return [5, 6, 1, 2]
        case "5": return [0, 5, 6, 2, 3]
        case "6": return [0, 5, 6, 4, 2, 3]
        case "7": return [0, 1, 2]
        case "8": return [0, 1, 2, 3, 4, 5, 6]
        case "9": return [0, 1, 2, 3, 5, 6]
        case "I": return [0, 3]
        case "B": return [2, 3, 4, 5, 6]
        case "M": return [0, 1, 2, 4, 5]
        default: return []
        }
    }

    var body: some View {
        ZStack {
            segment(.top, index: 0)
            segment(.upperRight, index: 1)
            segment(.lowerRight, index: 2)
            segment(.bottom, index: 3)
            segment(.lowerLeft, index: 4)
            segment(.upperLeft, index: 5)
            segment(.middle, index: 6)
        }
        .frame(width: 22, height: 38)
    }

    private func segment(_ position: SegmentPosition, index: Int) -> some View {
        RoundedRectangle(cornerRadius: 2, style: .continuous)
            .fill(activeSegments.contains(index) ? SegmentStyle.active : SegmentStyle.inactive)
            .frame(width: position.size.width, height: position.size.height)
            .offset(x: position.offset.x, y: position.offset.y)
    }
}

private struct SevenSegmentColon: View {
    var body: some View {
        VStack(spacing: 8) {
            Circle().fill(SegmentStyle.active)
            Circle().fill(SegmentStyle.active)
        }
        .frame(width: 5, height: 38)
    }
}

private enum SegmentStyle {
    static let active = Color.black.opacity(0.72)
    static let inactive = Color.black.opacity(0.08)
}

private enum SegmentPosition {
    case top
    case upperRight
    case lowerRight
    case bottom
    case lowerLeft
    case upperLeft
    case middle

    var size: CGSize {
        switch self {
        case .top, .middle, .bottom:
            return CGSize(width: 17, height: 4)
        case .upperRight, .lowerRight, .lowerLeft, .upperLeft:
            return CGSize(width: 4, height: 15)
        }
    }

    var offset: CGPoint {
        switch self {
        case .top:
            return CGPoint(x: 0, y: -17)
        case .upperRight:
            return CGPoint(x: 9, y: -8)
        case .lowerRight:
            return CGPoint(x: 9, y: 9)
        case .bottom:
            return CGPoint(x: 0, y: 18)
        case .lowerLeft:
            return CGPoint(x: -9, y: 9)
        case .upperLeft:
            return CGPoint(x: -9, y: -8)
        case .middle:
            return CGPoint(x: 0, y: 0)
        }
    }
}
