// ASH Pattern System — Windows Development Migration — Migration Guide Example
//
// This file demonstrates the use of the migration guide in the ASH Pattern System.

#include "include/migration_guide.hpp"
#include <iostream>

int main() {
    // Initialize migration guide
    if (!MigrationGuide::Initialize()) {
        std::cerr << "Failed to initialize migration guide." << std::endl;
        return 1;
    }

    // Get best practices
    auto bestPractices = MigrationGuide::GetBestPractices();
    for (const auto& practice : bestPractices) {
        std::cout << practice << std::endl;
    }

    // Get common pitfalls
    auto commonPitfalls = MigrationGuide::GetCommonPitfalls();
    for (const auto& pitfall : commonPitfalls) {
        std::cout << pitfall << std::endl;
    }

    // Get recommended approaches
    auto recommendedApproaches = MigrationGuide::GetRecommendedApproaches();
    for (const auto& approach : recommendedApproaches) {
        std::cout << approach << std::endl;
    }

    // Initialize migration checklist
    if (!MigrationChecklist::Initialize()) {
        std::cerr << "Failed to initialize migration checklist." << std::endl;
        return 1;
    }

    // Check if a file needs migration
    bool needsMigration = MigrationChecklist::NeedsMigration("path/to/file.cpp");
    std::cout << "File needs migration: " << (needsMigration ? "yes" : "no") << std::endl;

    // Get migration status
    std::string status = MigrationChecklist::GetMigrationStatus("path/to/file.cpp");
    std::cout << "Migration status: " << status << std::endl;

    // Initialize migration report
    if (!MigrationReport::Initialize()) {
        std::cerr << "Failed to initialize migration report." << std::endl;
        return 1;
    }

    // Add a file to the migration report
    MigrationReport::AddFile("path/to/file.cpp");

    // Get migration report
    std::string report = MigrationReport::GetReport();
    std::cout << "Migration report: " << report << std::endl;

    // Generate migration plan
    std::string migrationPlan = GenerateMigrationPlan("path/to/project");
    std::cout << "Migration plan: " << migrationPlan << std::endl;

    // Validate migration
    bool isValid = ValidateMigration("path/to/project");
    std::cout << "Migration is valid: " << (isValid ? "yes" : "no") << std::endl;

    // Apply migration
    bool applied = ApplyMigration("path/to/project");
    std::cout << "Migration applied: " << (applied ? "yes" : "no") << std::endl;

    // Shutdown migration guide
    MigrationGuide::Shutdown();
    MigrationChecklist::Shutdown();
    MigrationReport::Shutdown();

    return 0;
}
