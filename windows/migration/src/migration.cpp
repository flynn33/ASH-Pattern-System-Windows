// ASH Pattern System — Migration Layer Implementation
//
// This file implements the migration layer for Windows platform support.
// It handles legacy API compatibility and modern C++ features.

#include "include/migration.hpp"
#include <windows.h>
#include <commdlg.h>
#include <oleauto.h>

namespace ash {
namespace migration {

// ============================================================================
// MigrationContext Implementation
// ============================================================================

MigrationContext& MigrationContext::GetInstance() {
    static MigrationContext instance;
    return instance;
}

bool MigrationContext::Initialize(const std::string& platform) {
    m_platform = platform;
    m_legacyApiCompat = std::make_unique<LegacyApiCompat>();
    m_modernFeatures = std::make_unique<ModernFeatures>();
    m_crossPlatformAbstraction = std::make_unique<CrossPlatformAbstraction>();

    if (!m_legacyApiCompat->Initialize()) {
        return false;
    }
    if (!m_modernFeatures->Initialize()) {
        return false;
    }
    if (!m_crossPlatformAbstraction->Initialize()) {
        return false;
    }

    m_initialized = true;
    return true;
}

void MigrationContext::Shutdown() {
    if (m_legacyApiCompat) {
        m_legacyApiCompat->Shutdown();
    }
    if (m_modernFeatures) {
        m_modernFeatures->Shutdown();
    }
    if (m_crossPlatformAbstraction) {
        m_crossPlatformAbstraction->Shutdown();
    }

    m_initialized = false;
}

LegacyApiCompat& MigrationContext::GetLegacyApiCompat() {
    return *m_legacyApiCompat;
}

ModernFeatures& MigrationContext::GetModernFeatures() {
    return *m_modernFeatures;
}

CrossPlatformAbstraction& MigrationContext::GetCrossPlatformAbstraction() {
    return *m_crossPlatformAbstraction;
}

bool MigrationContext::IsInitialized() const {
    return m_initialized;
}

// ============================================================================
// LegacyApiCompat Implementation
// ============================================================================

bool LegacyApiCompat::Initialize() {
    // Initialize COM if needed
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    m_initialized = true;
    return true;
}

void LegacyApiCompat::Shutdown() {
    // Uninitialize COM if initialized
    CoUninitialize();

    m_initialized = false;
}

std::shared_ptr<WindowWrapper> LegacyApiCompat::WrapWindow(HWND hwnd) {
    return std::make_shared<WindowWrapper>(hwnd);
}

// ============================================================================
// ModernFeatures Implementation
// ============================================================================

bool ModernFeatures::Initialize() {
    // Initialize modern C++ features (already available in C++17/20)
    m_initialized = true;
    return true;
}

void ModernFeatures::Shutdown() {
    // No cleanup needed for modern C++ features
    m_initialized = false;
}

// ============================================================================
// CrossPlatformAbstraction Implementation
// ============================================================================

bool CrossPlatformAbstraction::Initialize() {
    m_initialized = true;
    return true;
}

void CrossPlatformAbstraction::Shutdown() {
    m_initialized = false;
}

std::string CrossPlatformAbstraction::GetPlatform() const {
    return m_platform;
}

bool CrossPlatformAbstraction::IsWindows() const {
    return m_platform == "windows-x64" || m_platform == "windows-x86";
}

// ============================================================================
// WindowWrapper Implementation
// ============================================================================

WindowWrapper::WindowWrapper(HWND hwnd) : m_handle(hwnd) {
}

// ============================================================================
// Migration Layer Initialization Functions
// ============================================================================

bool InitializeMigrationLayer() {
    return MigrationContext::GetInstance().Initialize("windows-x64");
}

void ShutdownMigrationLayer() {
    MigrationContext::GetInstance().Shutdown();
}

} // namespace migration
} // namespace ash
