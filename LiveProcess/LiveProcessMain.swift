import Foundation

@objc(LiveProcessMain)
class LiveProcessMain: NSObject, NSExtensionRequestHandling {
    func beginRequest(with context: NSExtensionContext) {
        // Extract arguments from context inputItems
        // Expected: [ "binaryPath": "...", "arguments": ["...", "..."] ]

        let inputItems = context.inputItems as? [NSExtensionItem] ?? []
        guard let item = inputItems.first, let userInfo = item.userInfo as? [String: Any] else {
            context.completeRequest(returningItems: [], completionHandler: nil)
            return
        }

        let binaryPath = userInfo["binaryPath"] as? String ?? ""
        let arguments = userInfo["arguments"] as? [String] ?? []

        // Execute virtualization logic inside the appex process
        setup_libsystem_hooks()
        setup_foundation_hooks()
        init_bypassDyldLibValidation()

        MachOLoader.loadAndExecute(binaryPath: binaryPath, arguments: arguments)

        context.completeRequest(returningItems: [], completionHandler: nil)
    }
}
