import AppKit
import SwiftUI

struct PC110DisplayView: NSViewRepresentable {
    let image: NSImage?
    let pixelWidth: Int
    let pixelHeight: Int
    let onMouseMoved: (Int32, Int32) -> Void
    let onMouseDown: (Int32, Int32, Int32) -> Void
    let onMouseUp: (Int32, Int32, Int32) -> Void
    let onTextInput: (String) -> Void
    let onKeyDown: (UInt16) -> Void
    let onKeyUp: (UInt16) -> Void

    func makeNSView(context: Context) -> SurfaceView {
        let view = SurfaceView()
        view.pixelWidth = pixelWidth
        view.pixelHeight = pixelHeight
        view.onMouseMoved = onMouseMoved
        view.onMouseDown = onMouseDown
        view.onMouseUp = onMouseUp
        view.onTextInput = onTextInput
        view.onKeyDown = onKeyDown
        view.onKeyUp = onKeyUp
        view.image = image
        DispatchQueue.main.async {
            view.window?.makeFirstResponder(view)
            view.window?.acceptsMouseMovedEvents = true
        }
        return view
    }

    func updateNSView(_ nsView: SurfaceView, context: Context) {
        nsView.pixelWidth = pixelWidth
        nsView.pixelHeight = pixelHeight
        nsView.onMouseMoved = onMouseMoved
        nsView.onMouseDown = onMouseDown
        nsView.onMouseUp = onMouseUp
        nsView.onTextInput = onTextInput
        nsView.onKeyDown = onKeyDown
        nsView.onKeyUp = onKeyUp
        nsView.image = image
        nsView.needsDisplay = true
    }

    final class SurfaceView: NSView {
        var image: NSImage?
        var pixelWidth = 640
        var pixelHeight = 480
        var onMouseMoved: ((Int32, Int32) -> Void)?
        var onMouseDown: ((Int32, Int32, Int32) -> Void)?
        var onMouseUp: ((Int32, Int32, Int32) -> Void)?
        var onTextInput: ((String) -> Void)?
        var onKeyDown: ((UInt16) -> Void)?
        var onKeyUp: ((UInt16) -> Void)?

        private var trackingAreaRef: NSTrackingArea?

        override var acceptsFirstResponder: Bool { true }
        override var isFlipped: Bool { true }

        override func acceptsFirstMouse(for event: NSEvent?) -> Bool {
            true
        }

        override func viewDidMoveToWindow() {
            super.viewDidMoveToWindow()
            window?.acceptsMouseMovedEvents = true
            window?.makeFirstResponder(self)
        }

        override func updateTrackingAreas() {
            if let trackingAreaRef {
                removeTrackingArea(trackingAreaRef)
            }
            let options: NSTrackingArea.Options = [.activeInKeyWindow, .mouseEnteredAndExited, .mouseMoved, .inVisibleRect]
            let area = NSTrackingArea(rect: .zero, options: options, owner: self, userInfo: nil)
            addTrackingArea(area)
            trackingAreaRef = area
            super.updateTrackingAreas()
        }

        override func draw(_ dirtyRect: NSRect) {
            NSColor.black.setFill()
            bounds.fill()

            guard let image else { return }
            image.draw(
                in: imageRect,
                from: NSRect(origin: .zero, size: image.size),
                operation: .sourceOver,
                fraction: 1.0,
                respectFlipped: true,
                hints: [.interpolation: NSImageInterpolation.none]
            )
        }

        override func mouseMoved(with event: NSEvent) {
            if let point = emulatorPoint(for: event) {
                onMouseMoved?(point.x, point.y)
            }
        }

        override func mouseDragged(with event: NSEvent) {
            mouseMoved(with: event)
        }

        override func mouseDown(with event: NSEvent) {
            window?.makeFirstResponder(self)
            if let point = emulatorPoint(for: event) {
                onMouseDown?(point.x, point.y, Int32(event.buttonNumber))
            }
        }

        override func mouseUp(with event: NSEvent) {
            if let point = emulatorPoint(for: event) {
                onMouseUp?(point.x, point.y, Int32(event.buttonNumber))
            }
        }

        override func rightMouseDown(with event: NSEvent) {
            window?.makeFirstResponder(self)
            if let point = emulatorPoint(for: event) {
                onMouseDown?(point.x, point.y, Int32(event.buttonNumber))
            }
        }

        override func rightMouseUp(with event: NSEvent) {
            if let point = emulatorPoint(for: event) {
                onMouseUp?(point.x, point.y, Int32(event.buttonNumber))
            }
        }

        override func keyDown(with event: NSEvent) {
            if let text = dosControlInput(from: event) {
                onTextInput?(text)
                return
            }
            if let text = dosTextInput(from: event) {
                onTextInput?(text)
                return
            }
            onKeyDown?(event.keyCode)
        }

        override func keyUp(with event: NSEvent) {
            onKeyUp?(event.keyCode)
        }

        private var imageRect: NSRect {
            let inset: CGFloat = 10
            let available = bounds.insetBy(dx: inset, dy: inset)
            let targetAspect = CGFloat(pixelWidth) / CGFloat(pixelHeight)
            let availableAspect = available.width / max(available.height, 1)

            if availableAspect > targetAspect {
                let height = available.height
                let width = height * targetAspect
                return NSRect(x: available.midX - width / 2, y: available.minY, width: width, height: height)
            } else {
                let width = available.width
                let height = width / targetAspect
                return NSRect(x: available.minX, y: available.midY - height / 2, width: width, height: height)
            }
        }

        private func emulatorPoint(for event: NSEvent) -> (x: Int32, y: Int32)? {
            let location = convert(event.locationInWindow, from: nil)
            let rect = imageRect
            guard rect.contains(location), rect.width > 0, rect.height > 0 else { return nil }

            let fx = (location.x - rect.minX) / rect.width
            let fy = (location.y - rect.minY) / rect.height
            let x = min(max(Int(fx * CGFloat(pixelWidth)), 0), pixelWidth - 1)
            let y = min(max(Int(fy * CGFloat(pixelHeight)), 0), pixelHeight - 1)
            return (Int32(x), Int32(y))
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
    }
}
