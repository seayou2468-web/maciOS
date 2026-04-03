import Foundation

class ProcessManager: ObservableObject {
    static let shared = ProcessManager()

    @Published var processes: [Int: LiveProcess] = [:]

    func setupEnvironment() {
        let home = URL.documentsDirectory.path
        setenv("HOME", home, 1)
        setenv("LC_HOME_PATH", home, 1)
        setenv("PATH", "/usr/bin:/bin:/usr/sbin:/sbin", 1)
        setenv("SHELL", "/bin/sh", 1)
        setenv("DYLD_LIBRARY_PATH", (home as NSString).appendingPathComponent("macroot/usr/lib"), 1)
    }

    func spawnProcess(binaryPath: String, arguments: [String] = []) {
        let process = LiveProcess(binaryPath: binaryPath, arguments: arguments)

        DispatchQueue.main.async {
            self.processes[process.id] = process
            process.start()
        }
    }

    func killProcess(id: Int) {
        // In this architecture, "killing" a process means removing it from the tracking.
        // Thread management for actual stopping would require more complex Mach port usage.
        DispatchQueue.main.async {
            self.processes.removeValue(forKey: id)
        }
    }
}
