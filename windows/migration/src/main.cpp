#include <iostream>
#include "include/modern_features.hpp"
#include "include/cross_platform_abstraction.hpp"
#include "include/migration_guide.hpp"

int main() {
    // Modern C++ features example
    std::cout << "=== ASH Pattern System — Windows Development Migration ===" << std::endl;
    
    // Smart pointer wrapper
    auto ptr = ash::migration::SharedPtrWrapper<std::shared_ptr<int>>();
    *ptr = 42;
    std::cout << "Smart pointer value: " << *ptr << std::endl;
    
    // Optional wrapper
    auto opt = ash::migration::OptionalWrapper<std::optional<int>>();
    *opt = 100;
    if (opt.has_value()) {
        std::cout << "Optional value: " << *opt << std::endl;
    }
    
    // Variant wrapper
    auto var = ash::migration::VariantWrapper<std::variant<int, std::string>>();
    var.value() = 200;
    if (std::holds_alternative<int>(var)) {
        std::cout << "Variant value: " << std::get<int>(var) << std::endl;
    }
    
    // Function wrapper
    auto func = ash::migration::FunctionWrapper([](int x) { return x * 2; });
    std::cout << "Function result: " << func(50) << std::endl;
    
    // Lambda wrapper
    auto lambda = ash::migration::LambdaWrapper([]() { std::cout << "Hello, World!" << std::endl; });
    lambda();
    
    // Cross-platform abstractions example
    std::cout << "\n=== Cross-Platform Abstractions ===" << std::endl;
    
    // File system abstraction
    FileSystemAbstraction fs;
    if (fs.Initialize()) {
        std::cout << "File system initialized successfully" << std::endl;
        fs.CreateDirectory("C:/Temp");
        fs.WriteFile("C:/Temp/test.txt", "Test content");
        std::cout << "File operations completed successfully" << std::endl;
        fs.Shutdown();
    }
    
    // Registry abstraction
    RegistryAbstraction registry;
    if (registry.Initialize()) {
        std::cout << "Registry initialized successfully" << std::endl;
        registry.CreateKey("HKEY_CURRENT_USER\\Software\\MyApp");
        registry.WriteValue("HKEY_CURRENT_USER\\Software\\MyApp", "Version", "1.0.0");
        std::cout << "Registry operations completed successfully" << std::endl;
        registry.Shutdown();
    }
    
    // Process abstraction
    ProcessAbstraction process;
    if (process.Initialize()) {
        std::cout << "Process initialized successfully" << std::endl;
        auto proc = process.Start("notepad.exe");
        proc->Terminate();
        std::cout << "Process operations completed successfully" << std::endl;
        process.Shutdown();
    }
    
    // Network abstraction
    NetworkAbstraction network;
    if (network.Initialize()) {
        std::cout << "Network initialized successfully" << std::endl;
        auto socket = network.Connect("localhost", 8080);
        socket->Close();
        std::cout << "Network operations completed successfully" << std::endl;
        network.Shutdown();
    }
    
    // Thread abstraction
    ThreadAbstraction thread;
    if (thread.Initialize()) {
        std::cout << "Thread initialized successfully" << std::endl;
        auto t = thread.Create([]() {
            std::cout << "Thread code executed" << std::endl;
        });
        t->Join();
        std::cout << "Thread operations completed successfully" << std::endl;
        thread.Shutdown();
    }
    
    // Event abstraction
    EventAbstraction event;
    if (event.Initialize()) {
        std::cout << "Event initialized successfully" << std::endl;
        auto evt = event.Create();
        evt->Set();
        bool result = evt->WaitForEvent();
        evt->Reset();
        std::cout << "Event operations completed successfully" << std::endl;
        event.Shutdown();
    }
    
    // Migration guide example
    std::cout << "\n=== Migration Guide ===" << std::endl;
    
    MigrationGuide guide;
    if (guide.Initialize()) {
        auto bestPractices = guide.GetBestPractices();
        auto commonPitfalls = guide.GetCommonPitfalls();
        auto recommendedApproaches = guide.GetRecommendedApproaches();
        
        std::cout << "Best practices: " << bestPractices.size() << std::endl;
        std::cout << "Common pitfalls: " << commonPitfalls.size() << std::endl;
        std::cout << "Recommended approaches: " << recommendedApproaches.size() << std::endl;
        
        guide.Shutdown();
    }
    
    MigrationChecklist checklist;
    if (checklist.Initialize()) {
        bool needsMigration = checklist.NeedsMigration("path/to/file.cpp");
        std::string status = checklist.GetMigrationStatus("path/to/file.cpp");
        
        std::cout << "Needs migration: " << (needsMigration ? "yes" : "no") << std::endl;
        std::cout << "Migration status: " << status << std::endl;
        
        checklist.Shutdown();
    }
    
    MigrationReport report;
    if (report.Initialize()) {
        report.AddFile("path/to/file.cpp");
        std::string reportStr = report.GetReport();
        
        std::cout << "Migration report: " << reportStr << std::endl;
        
        report.Shutdown();
    }
    
    std::cout << "\n=== ASH Pattern System — Windows Development Migration ===" << std::endl;
    std::cout << "All operations completed successfully!" << std::endl;
    
    return 0;
}
