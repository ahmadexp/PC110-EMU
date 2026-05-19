import SwiftUI
import AppKit

struct KeyboardCaptureView: NSViewRepresentable {
    let onTextInput: (String) -> Void
    let onKeyDown: (UInt16) -> Void
    let onKeyUp: (UInt16) -> Void

    func makeNSView(context: Context) -> KeyView {
        let view = KeyView()
        view.onTextInput = onTextInput
        view.onKeyDown = onKeyDown
        view.onKeyUp = onKeyUp
        DispatchQueue.main.async {
            view.window?.makeFirstResponder(view)
        }
        return view
    }

    func updateNSView(_ nsView: KeyView, context: Context) {
        nsView.onTextInput = onTextInput
        nsView.onKeyDown = onKeyDown
        nsView.onKeyUp = onKeyUp
        DispatchQueue.main.async {
            nsView.window?.makeFirstResponder(nsView)
        }
    }

    final class KeyView: NSView {
        var onTextInput: ((String) -> Void)?
        var onKeyDown: ((UInt16) -> Void)?
        var onKeyUp: ((UInt16) -> Void)?

        override var acceptsFirstResponder: Bool { true }

        override func viewDidMoveToWindow() {
            super.viewDidMoveToWindow()
            window?.makeFirstResponder(self)
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
