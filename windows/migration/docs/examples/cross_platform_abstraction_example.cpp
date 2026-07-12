// ASH Pattern System — Windows Development Migration — Cross-Platform Abstraction Example
//
// This file demonstrates the use of cross-platform abstractions in the ASH Pattern System.

#include "include/cross_platform_abstraction.hpp"
#include <iostream>

int main() {
    // Initialize cross-platform abstraction
    if (!CrossPlatformAbstraction::Initialize("windows")) {
        std::cerr << "Failed to initialize cross-platform abstraction." << std::endl;
        return 1;
    }

    // File system abstraction
    FileSystemAbstraction fs;
    fs.CreateDirectory("C:/Temp");
    fs.WriteFile("C:/Temp/test.txt", "Hello, World!");
    std::string content = fs.ReadFile("C:/Temp/test.txt");
    fs.Delete("C:/Temp/test.txt");

    // Registry abstraction
    RegistryAbstraction registry;
    registry.CreateKey("HKEY_CURRENT_USER\\Software\\MyApp");
    registry.DeleteKey("HKEY_CURRENT_USER\\Software\\MyApp");

    // Process abstraction
    ProcessAbstraction process;
    auto proc = process.Start("notepad.exe");

    // Network abstraction
    NetworkAbstraction network;
    auto socket = network.Connect("localhost", 8080);

    // Thread abstraction
    ThreadAbstraction thread;
    auto t = thread.Create([]() { /* thread code */ });

    // Event abstraction
    EventAbstraction event;
    auto evt = event.Create();

    // Shutdown cross-platform abstraction
    CrossPlatformAbstraction::Shutdown();

    return 0;
}
