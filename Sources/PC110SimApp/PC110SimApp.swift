import SwiftUI

@main
struct PC110EMUApp: App {
    @StateObject private var host = EmulatorHost()

    var body: some Scene {
        WindowGroup("PC110 EMU") {
            ContentView()
                .environmentObject(host)
                .onAppear {
                    host.start()
                }
        }
        .windowStyle(.titleBar)
    }
}
