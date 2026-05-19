import AVFoundation
import Foundation

final class PCSpeakerAudio {
    private let engine = AVAudioEngine()
    private let player = AVAudioPlayerNode()
    private let format = AVAudioFormat(standardFormatWithSampleRate: 44_100, channels: 1)!

    private var lastEventCount: UInt64 = 0
    private var lastContinuousSchedule = Date.distantPast
    private var phase: Double = 0

    init() {
        engine.attach(player)
        engine.connect(player, to: engine.mainMixerNode, format: format)
        startIfNeeded()
    }

    func stop() {
        player.stop()
        engine.stop()
    }

    func update(enabled: Bool, frequency: Double, eventCount: UInt64, eventFrequency: Double) {
        startIfNeeded()

        var playedEvent = false
        if eventCount < lastEventCount {
            lastEventCount = eventCount
        } else if eventCount > lastEventCount {
            lastEventCount = eventCount
            playTone(frequency: eventFrequency, duration: 0.14, amplitude: 0.12)
            playedEvent = true
        }

        guard enabled, !playedEvent else { return }
        let now = Date()
        if now.timeIntervalSince(lastContinuousSchedule) >= 0.04 {
            playTone(frequency: frequency, duration: 0.055, amplitude: 0.10)
            lastContinuousSchedule = now
        }
    }

    private func startIfNeeded() {
        if !engine.isRunning {
            try? engine.start()
        }
        if !player.isPlaying {
            player.play()
        }
    }

    private func playTone(frequency: Double, duration: Double, amplitude: Float) {
        guard frequency >= 20.0, frequency <= 20_000.0 else { return }

        let sampleRate = format.sampleRate
        let frameCount = AVAudioFrameCount(max(1, Int(sampleRate * duration)))
        guard let buffer = AVAudioPCMBuffer(pcmFormat: format, frameCapacity: frameCount),
              let samples = buffer.floatChannelData?[0] else {
            return
        }

        buffer.frameLength = frameCount
        let frames = Int(frameCount)
        let step = frequency / sampleRate
        for frame in 0..<frames {
            let fadeIn = min(1.0, Double(frame) / 96.0)
            let fadeOut = min(1.0, Double(frames - frame - 1) / 96.0)
            let envelope = Float(min(fadeIn, fadeOut))
            samples[frame] = (phase < 0.5 ? amplitude : -amplitude) * envelope
            phase += step
            phase -= floor(phase)
        }

        player.scheduleBuffer(buffer, completionHandler: nil)
    }
}
