import Foundation

class LiveProcess: ObservableObject {
    let binaryPath: String
    let arguments: [String]
    let id: Int

    @Published var isRunning = false
    @Published var output = ""

    private var outputPipe: Pipe?

    init(binaryPath: String, arguments: [String] = []) {
        self.binaryPath = binaryPath
        self.arguments = arguments
        self.id = Int.random(in: 1000...9999)
    }

    func start() {
        guard !isRunning else { return }
        isRunning = true

        // In a real LiveProcess implementation (like Nyxian/LiveContainer),
        // this might involve posix_spawn or custom Mach task creation.
        // For our non-jailbreak environment, we use managed threads + redirection.

        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            guard let self = self else { return }

            // Setup environment and hooks specific to this process context
            // Note: In a single-process iOS app, global hooks affect all threads.

            print("[LiveProcess \(self.id)] Starting: \(self.binaryPath)")
            MachOLoader.loadAndExecute(binaryPath: self.binaryPath, arguments: self.arguments)

            DispatchQueue.main.async {
                self.isRunning = false
                print("[LiveProcess \(self.id)] Finished")
            }
        }
    }
}
