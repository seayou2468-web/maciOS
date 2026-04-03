import Foundation

class ProcessManager {
    static let shared = ProcessManager()

    private var processes: [Int: String] = [:]

    func setupEnvironment() {
        let home = URL.documentsDirectory.path
        setenv("HOME", home, 1)
        setenv("LC_HOME_PATH", home, 1)
        setenv("PATH", "/usr/bin:/bin:/usr/sbin:/sbin", 1)
        setenv("SHELL", "/bin/sh", 1)
        setenv("DYLD_LIBRARY_PATH", (home as NSString).appendingPathComponent("macroot/usr/lib"), 1)
    }

    func registerProcess(pid: Int, name: String) {
        processes[pid] = name
    }
}
