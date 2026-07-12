// ASH Pattern System — Legacy API Compatibility Header
//
// This header provides compatibility with legacy Windows APIs.
// It handles Win32, MFC, ATL, and other legacy APIs while maintaining
// modern C++ code quality.

#pragma once

#include "migration.hpp"
#include <windows.h>
#include <commdlg.h>
#include <oleauto.h>

namespace ash {
namespace migration {

// ============================================================================
// Win32 API Compatibility Wrappers
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
    explicit WindowWrapper(HWND hwnd = nullptr);

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

    /**
     * @brief Show or hide the window.
     * 
     * @param show true to show, false to hide.
     */
    void Show(bool show) {
        if (m_handle) {
            ShowWindow(m_handle, show ? SW_SHOW : SW_HIDE);
        }
    }

    /**
     * @brief Destroy the window.
     */
    void Destroy() {
        if (m_handle) {
            DestroyWindow(m_handle);
            m_handle = nullptr;
        }
    }

private:
    HWND m_handle = nullptr;
};

/**
 * @brief Modern C++ wrapper for Win32 message handles.
 */
class MessageWrapper {
public:
    /**
     * @brief Construct a new Message Wrapper.
     * 
     * @param hMsg The Win32 HMSG to wrap.
     */
    explicit MessageWrapper(HMSG hMsg = nullptr);

    /**
     * @brief Get the underlying Win32 message handle.
     * 
     * @return HMSG The Win32 HMSG.
     */
    HMSG GetHandle() const { return m_handle; }

    /**
     * @brief Check if the message is valid.
     * 
     * @return true if the message is valid, false otherwise.
     */
    bool IsValid() const { return m_handle != nullptr; }

private:
    HMSG m_handle = nullptr;
};

/**
 * @brief Modern C++ wrapper for Win32 file handles.
 */
class FileHandleWrapper {
public:
    /**
     * @brief Construct a new File Handle Wrapper.
     * 
     * @param hFile The Win32 HANDLE to wrap.
     */
    explicit FileHandleWrapper(HANDLE hFile = nullptr);

    /**
     * @brief Get the underlying Win32 file handle.
     * 
     * @return HANDLE The Win32 HANDLE.
     */
    HANDLE GetHandle() const { return m_handle; }

    /**
     * @brief Check if the file handle is valid.
     * 
     * @return true if the file handle is valid, false otherwise.
     */
    bool IsValid() const { return m_handle != nullptr; }

private:
    HANDLE m_handle = nullptr;
};

// ============================================================================
// MFC API Compatibility Wrappers
// ============================================================================

/**
 * @brief Modern C++ wrapper for MFC CWnd objects.
 */
class CWndWrapper {
public:
    /**
     * @brief Construct a new CWnd Wrapper.
     * 
     * @param pWnd The MFC CWnd pointer to wrap.
     */
    explicit CWndWrapper(CWnd* pWnd = nullptr);

    /**
     * @brief Get the underlying MFC CWnd pointer.
     * 
     * @return CWnd* The MFC CWnd pointer.
     */
    CWnd* GetPointer() const { return m_pWnd; }

    /**
     * @brief Check if the CWnd is valid.
     * 
     * @return true if the CWnd is valid, false otherwise.
     */
    bool IsValid() const { return m_pWnd != nullptr; }

private:
    CWnd* m_pWnd = nullptr;
};

/**
 * @brief Modern C++ wrapper for MFC CDocView objects.
 */
class CDocViewWrapper {
public:
    /**
     * @brief Construct a new CDocView Wrapper.
     * 
     * @param pDocView The MFC CDocView pointer to wrap.
     */
    explicit CDocViewWrapper(CDocView* pDocView = nullptr);

    /**
     * @brief Get the underlying MFC CDocView pointer.
     * 
     * @return CDocView* The MFC CDocView pointer.
     */
    CDocView* GetPointer() const { return m_pDocView; }

    /**
     * @brief Check if the CDocView is valid.
     * 
     * @return true if the CDocView is valid, false otherwise.
     */
    bool IsValid() const { return m_pDocView != nullptr; }

private:
    CDocView* m_pDocView = nullptr;
};

// ============================================================================
// ATL API Compatibility Wrappers
// ============================================================================

/**
 * @brief Modern C++ wrapper for ATL CComPtr objects.
 */
template<typename T>
class CComPtrWrapper {
public:
    /**
     * @brief Construct a new CComPtr Wrapper.
     * 
     * @param ptr The ATL CComPtr pointer to wrap.
     */
    explicit CComPtrWrapper(T* ptr = nullptr);

    /**
     * @brief Get the underlying ATL CComPtr pointer.
     * 
     * @return T* The ATL CComPtr pointer.
     */
    T* GetPointer() const { return m_ptr; }

    /**
     * @brief Check if the CComPtr is valid.
     * 
     * @return true if the CComPtr is valid, false otherwise.
     */
    bool IsValid() const { return m_ptr != nullptr; }

private:
    T* m_ptr = nullptr;
};

// ============================================================================
// Legacy API Compatibility Functions
// ============================================================================

/**
 * @brief Convert a Win32 HANDLE to a modern C++ handle wrapper.
 * 
 * @param handle The Win32 HANDLE to convert.
 * @return std::optional<std::shared_ptr<HandleWrapper>> Optional handle wrapper.
 */
template<typename T>
std::optional<std::shared_ptr<T>> ConvertToModernHandle(HANDLE handle);

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
std::optional<std::shared_ptr<T>> ConvertMfcCWnd(CWnd* pWnd);

/**
 * @brief Convert an ATL CComPtr pointer to a modern C++ wrapper.
 * 
 * @param ptr The ATL CComPtr pointer to convert.
 * @return std::optional<std::shared_ptr<CComPtrWrapper>> Optional CComPtr wrapper.
 */
template<typename T>
std::optional<std::shared_ptr<CComPtrWrapper<T>>> ConvertAtlCComPtr(T* ptr);

// ============================================================================
// Legacy API Compatibility Macros
// ============================================================================

/**
 * @brief Macro to safely call Win32 APIs with error checking.
 * 
 * Example: SAFE_CALL(GetLastError())
 */
#define SAFE_CALL(api) \
    do { \
        if (!(api)) { \
            DWORD err = GetLastError(); \
            return false; \
        } \
    } while (false)

/**
 * @brief Macro to safely call Win32 APIs with error logging.
 * 
 * Example: SAFE_CALL_LOG(GetLastError(), "Failed to get last error")
 */
#define SAFE_CALL_LOG(api, msg) \
    do { \
        if (!(api)) { \
            DWORD err = GetLastError(); \
            OutputDebugStringA(msg); \
            return false; \
        } \
    } while (false)

// ============================================================================
// Legacy API Compatibility Classes
// ============================================================================

/**
 * @brief Provides a safe wrapper for Win32 GDI objects.
 */
class GdiObjectWrapper {
public:
    /**
     * @brief Construct a new GDI Object Wrapper.
     * 
     * @param hObject The Win32 HGDIOBJ to wrap.
     */
    explicit GdiObjectWrapper(HGDIOBJ hObject = nullptr);

    /**
     * @brief Get the underlying Win32 GDI object handle.
     * 
     * @return HGDIOBJ The Win32 HGDIOBJ.
     */
    HGDIOBJ GetHandle() const { return m_handle; }

    /**
     * @brief Check if the GDI object is valid.
     * 
     * @return true if the GDI object is valid, false otherwise.
     */
    bool IsValid() const { return m_handle != nullptr; }

    /**
     * @brief Select the GDI object into a device context.
     * 
     * @param hdc The device context to select into.
     * @return HGDIOBJ The previous GDI object, or nullptr if selection failed.
     */
    HGDIOBJ SelectInto(HDC hdc) {
        return SelectObject(hdc, m_handle);
    }

private:
    HGDIOBJ m_handle = nullptr;
};

/**
 * @brief Provides a safe wrapper for Win32 GDI+ objects.
 */
class GdiPlusObjectWrapper {
public:
    /**
     * @brief Construct a new GDI+ Object Wrapper.
     * 
     * @param hObject The Win32 HGDIOBJ to wrap.
     */
    explicit GdiPlusObjectWrapper(HGDIOBJ hObject = nullptr);

    /**
     * @brief Get the underlying Win32 GDI+ object handle.
     * 
     * @return HGDIOBJ The Win32 HGDIOBJ.
     */
    HGDIOBJ GetHandle() const { return m_handle; }

    /**
     * @brief Check if the GDI+ object is valid.
     * 
     * @return true if the GDI+ object is valid, false otherwise.
     */
    bool IsValid() const { return m_handle != nullptr; }

private:
    HGDIOBJ m_handle = nullptr;
};

} // namespace migration
} // namespace ash
