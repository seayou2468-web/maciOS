//
//  Execute.swift
//  maciOS
//
//  Created by Stossy11 on 22/08/2025.
//

import Foundation
import MachO

struct LCMain {
    let entryOffset: UInt64
    let stackSize: UInt64
}

class MachOLoader {

    static func loadAndExecute(binaryPath: String, arguments: [String] = []) {
        // Ensure hooks are initialized for the process space
        setup_libsystem_hooks()
        setup_foundation_hooks()
        
        guard let entryPoint = getARM64EntryPoint(from: binaryPath) else {
            print("Could not find ARM64 entry point")
            return
        }
        
        // Dynamically load dependencies using redirected dlopen
        if dlopen(binaryPath, RTLD_NOW | RTLD_GLOBAL) == nil {
            let error = String(cString: dlerror())
            print("dlopen failed: \(error)")
        }
        
        let fileData = try? Data(contentsOf: URL(fileURLWithPath: binaryPath))
        guard let data = fileData else { return }
        
        let size = data.count
        var addr: vm_address_t = 0
        let kr = vm_allocate(mach_task_self(), &addr, vm_size_t(size), VM_FLAGS_ANYWHERE)
        
        if kr == KERN_SUCCESS {
            let buffer = UnsafeMutableRawPointer(bitPattern: UInt(addr))!
            data.withUnsafeBytes { ptr in
                memcpy(buffer, ptr.baseAddress!, size)
            }
            
            // Apply executable permissions
            vm_protect(mach_task_self(), addr, vm_size_t(size), 0, VM_PROT_READ | VM_PROT_EXEC)
            
            let entry = buffer.advanced(by: Int(entryPoint.entryOffset))

            // Standard C-main signature: int main(int argc, char** argv, char** envp, char** apple)
            typealias MainFunc = @convention(c) (Int32, UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>?, UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>?, UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>?) -> Int32
            let main = unsafeBitCast(entry, to: MainFunc.self)
            
            var args = arguments
            args.insert(binaryPath, at: 0)
            
            let cArgs = UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>.allocate(capacity: args.count + 1)
            for (i, arg) in args.enumerated() {
                cArgs[i] = strdup(arg)
            }
            cArgs[args.count] = nil
            
            // Build simple environment pointers (passing through system ones for now)
            let cEnv = UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>.allocate(capacity: 1)
            cEnv[0] = nil

            let cApple = UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>.allocate(capacity: 1)
            cApple[0] = nil

            print("Executing binary at: \(entry)")
            let _ = main(Int32(args.count), cArgs, cEnv, cApple)
            
            // Cleanup
            for i in 0..<args.count {
                free(cArgs[i])
            }
            cArgs.deallocate()
            cEnv.deallocate()
            cApple.deallocate()
        }
    }

    static func getARM64EntryPoint(from path: String) -> LCMain? {
        let CPU_ARM64: Int32 = 0x0100000C
        let LC_MAIN: UInt32 = 0x80000028
        
        guard let file = fopen(path, "rb") else { return nil }
        defer { fclose(file) }
        
        var header = mach_header_64()
        fread(&header, MemoryLayout<mach_header_64>.size, 1, file)
        
        if header.magic != MH_MAGIC_64 { return nil }
        
        var currentOffset = MemoryLayout<mach_header_64>.size
        for _ in 0..<header.ncmds {
            fseek(file, currentOffset, SEEK_SET)
            var cmd = load_command()
            fread(&cmd, MemoryLayout<load_command>.size, 1, file)
            
            if cmd.cmd == LC_MAIN {
                fseek(file, currentOffset, SEEK_SET)
                var ep = entry_point_command()
                fread(&ep, MemoryLayout<entry_point_command>.size, 1, file)
                return LCMain(entryOffset: ep.entryoff, stackSize: ep.stacksize)
            }
            currentOffset += Int(cmd.cmdsize)
        }
        return nil
    }
}
