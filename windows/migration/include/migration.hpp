// ASH Pattern System — Migration Layer Header
//
// This header provides the migration layer for Windows platform support.
// It handles legacy API compatibility and modern C++ features.

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <variant>

namespace ash {
namespace migration {

// ============================================================================
// Forward Declarations
// ============================================================================

class MigrationContext;
class LegacyApiCompat;
class ModernFeatures;
class CrossPlatformAbstraction;

// ============================================================================
// Migration Context
// ============================================================================

/**
 * @brief Manages the migration layer context for Windows platform support.
 * 
 * Provides access to legacy API compatibility, modern features, and cross-platform
 * abstraction utilities.
 */
class MigrationContext {
public:
    /**
     * @brief Construct a new Migration Context.
     * 
     * Initializes the migration layer with default settings.
     */
    static MigrationContext& GetInstance();

    /**
     * @brief Initialize the migration layer.
     * 
     * @param platform The target Windows platform (e.g., "windows-x64").
     * @return true if initialization succeeded, false otherwise.
     */
    bool Initialize(const std::string& platform);

    /**
     * @brief Shutdown the migration layer.
     * 
     * Cleans up resources and releases handles.
     */
    void Shutdown();

    /**
     * @brief Get the legacy API compatibility instance.
     * 
     * @return LegacyApiCompat* Pointer to the legacy API compatibility instance.
     */
    LegacyApiCompat& GetLegacyApiCompat();

    /**
     * @brief Get the modern features instance.
     * 
     * @return ModernFeatures* Pointer to the modern features instance.
     */
    ModernFeatures& GetModernFeatures();

    /**
     * @brief Get the cross-platform abstraction instance.
     * 
     * @return CrossPlatformAbstraction* Pointer to the cross-platform abstraction instance.
     */
    CrossPlatformAbstraction& GetCrossPlatformAbstraction();

    /**
     * @brief Check if the migration layer is initialized.
     * 
     * @return true if initialized, false otherwise.
     */
    bool IsInitialized() const;

private:
    friend class LegacyApiCompat;
    friend class ModernFeatures;
    friend class CrossPlatformAbstraction;

    MigrationContext() = default;
    ~MigrationContext() = default;
    MigrationContext(const MigrationContext&) = delete;
    MigrationContext& operator=(const MigrationContext&) = delete;

    std::string m_platform;
    bool m_initialized = false;
    std::unique_ptr<LegacyApiCompat> m_legacyApiCompat;
    std::unique_ptr<ModernFeatures> m_modernFeatures;
    std::unique_ptr<CrossPlatformAbstraction> m_crossPlatformAbstraction;
};

// ============================================================================
// Legacy API Compatibility
// ============================================================================

/**
 * @brief Provides compatibility with legacy Windows APIs.
 * 
 * Handles Win32, MFC, ATL, and other legacy APIs while maintaining
 * modern C++ code quality.
 */
class LegacyApiCompat {
public:
    /**
     * @brief Initialize legacy API compatibility.
     * 
     * @return true if initialization succeeded, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Shutdown legacy API compatibility.
     * 
     * Cleans up resources and releases handles.
     */
    void Shutdown();

    /**
     * @brief Convert a Win32 HANDLE to a modern C++ handle wrapper.
     * 
     * @param handle The Win32 HANDLE to convert.
     * @return std::optional<std::shared_ptr<HandleWrapper>> Optional handle wrapper.
     */
    template<typename T>
    std::optional<std::shared_ptr<T>> ConvertToModernHandle(HANDLE handle) {
        if (!handle) {
            return std::nullopt;
        }
        return std::make_shared<T>(handle);
    }

    /**
     * @brief Wrap a Win32 window handle in a modern C++ class.
     * 
     * @param hwnd The Win32 HWND to wrap.
     * @return std::shared_ptr<WindowWrapper> Shared pointer to the wrapped window.
     */
    std::shared_ptr<WindowWrapper> WrapWindow(HWND hwnd);

    /**
     * @brief Convert an MFC CWnd pointer to a modern C++ wrapper.
     * 
     * @param pWnd The MFC CWnd pointer to convert.
     * @return std::optional<std::shared_ptr<CWndWrapper>> Optional CWnd wrapper.
     */
    template<typename T>
    std::optional<std::shared_ptr<T>> ConvertMfcCWnd(CWnd* pWnd) {
        if (!pWnd) {
            return std::nullopt;
        }
        return std::make_shared<T>(pWnd);
    }

private:
    bool m_initialized = false;
};

// ============================================================================
// Modern Features
// ============================================================================

/**
 * @brief Provides modern C++17/20 features for Windows development.
 * 
 * Includes smart pointers, optional, variant, and other modern C++ features.
 */
class ModernFeatures {
public:
    /**
     * @brief Initialize modern features.
     * 
     * @return true if initialization succeeded, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Shutdown modern features.
     * 
     * Cleans up resources and releases handles.
     */
    void Shutdown();

    /**
     * @brief Create a thread-safe shared pointer wrapper.
     * 
     * @param object The object to wrap.
     * @return std::shared_ptr<ObjectWrapper> Shared pointer to the wrapped object.
     */
    template<typename T>
    std::shared_ptr<T> CreateSharedPointer(T&& object) {
        return std::make_shared<T>(std::forward<T>(object));
    }

    /**
     * @brief Create a unique pointer wrapper.
     * 
     * @param object The object to wrap.
     * @return std::unique_ptr<ObjectWrapper> Unique pointer to the wrapped object.
     */
    template<typename T, typename... Args>
    std::unique_ptr<T> CreateUniquePointer(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

private:
    bool m_initialized = false;
};

// ============================================================================
// Cross-Platform Abstraction
// ============================================================================

/**
 * @brief Provides cross-platform abstraction for Windows development.
 * 
 * Handles platform-specific code while maintaining a consistent API.
 */
class CrossPlatformAbstraction {
public:
    /**
     * @brief Initialize cross-platform abstraction.
     * 
     * @return true if initialization succeeded, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Shutdown cross-platform abstraction.
     * 
     * Cleans up resources and releases handles.
     */
    void Shutdown();

    /**
     * @brief Get the current platform string.
     * 
     * @return std::string The platform string (e.g., "windows-x64").
     */
    std::string GetPlatform() const;

    /**
     * @brief Check if running on Windows.
     * 
     * @return true if running on Windows, false otherwise.
     */
    bool IsWindows() const;

private:
    std::string m_platform = "windows-x64";
    bool m_initialized = false;
};

// ============================================================================
// Window Wrapper (Legacy API Compatibility)
// ============================================================================

/**
 * @brief Modern C++ wrapper for Win32 window handles.
 */
class WindowWrapper {
public:
    /**
     * @brief Construct a new Window Wrapper.
     * 
     * @param hwnd The Win32 HWND to wrap.
     */
    explicit WindowWrapper(HWND hwnd);

    /**
     * @brief Get the underlying Win32 window handle.
     * 
     * @return HWND The Win32 HWND.
     */
    HWND GetHandle() const { return m_handle; }

    /**
     * @brief Check if the window is valid.
     * 
     * @return true if the window is valid, false otherwise.
     */
    bool IsValid() const { return m_handle != nullptr; }

private:
    HWND m_handle = nullptr;
};

// ============================================================================
// Migration Layer Initialization
// ============================================================================

/**
 * @brief Initialize all migration layer components.
 * 
 * @return true if initialization succeeded, false otherwise.
 */
bool InitializeMigrationLayer();

/**
 * @brief Shutdown all migration layer components.
 * 
 * Cleans up resources and releases handles.
 */
void ShutdownMigrationLayer();

} // namespace migration
} // namespace ash
