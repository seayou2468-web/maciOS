import Foundation

class ProcessManager: ObservableObject {
    static let shared = ProcessManager()

    @Published var runningProcesses: [Int: String] = [:]

    func setupEnvironment() {
        let home = URL.documentsDirectory.path
        setenv("HOME", home, 1)
        setenv("LC_HOME_PATH", home, 1)
        setenv("PATH", "/usr/bin:/bin:/usr/sbin:/sbin", 1)
        setenv("SHELL", "/bin/sh", 1)
        setenv("DYLD_LIBRARY_PATH", (home as NSString).appendingPathComponent("macroot/usr/lib"), 1)
    }

    func spawnTask(binaryPath: String, arguments: [String] = []) {
        DispatchQueue.global(qos: .userInitiated).async {
            print("Spawning virtual task: \(binaryPath)")
            let pid = Int.random(in: 1000...9999)

            DispatchQueue.main.async {
                self.runningProcesses[pid] = (binaryPath as NSString).lastPathComponent
            }

            MachOLoader.loadAndExecute(binaryPath: binaryPath, arguments: arguments)

            DispatchQueue.main.async {
                self.runningProcesses.removeValue(forKey: pid)
            }
        }
    }
}
