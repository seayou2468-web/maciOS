import Foundation

class LiveProcess: ObservableObject {
    let binaryPath: String
    let arguments: [String]
    let id: Int

    @Published var isRunning = false
    @Published var output = ""

    init(binaryPath: String, arguments: [String] = []) {
        self.binaryPath = binaryPath
        self.arguments = arguments
        self.id = Int.random(in: 1000...9999)
    }

    func start() {
        guard !isRunning else { return }
        isRunning = true

        // Use NotificationCenter or custom XPC to receive output from AppEx
        NotificationCenter.default.addObserver(forName: NSNotification.Name("LiveProcessOutput"), object: nil, queue: .main) { [weak self] notification in
            if let pid = notification.userInfo?["pid"] as? Int, pid == self?.id,
               let data = notification.userInfo?["data"] as? String {
                self?.output += data

                // Also send to global terminal runner
                // StdioRunner.shared.newOutputLine = data (if integrated)
            }
        }

        ProcessManager.shared.spawnProcess(binaryPath: binaryPath, arguments: arguments)
    }
}
