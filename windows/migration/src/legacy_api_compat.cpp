// ASH Pattern System — Legacy API Compatibility Implementation
//
// This file implements compatibility with legacy Windows APIs.
// It handles Win32, MFC, ATL, and other legacy APIs while maintaining
// modern C++ code quality.

#include "include/legacy_api_compat.hpp"
#include <windows.h>
#include <commdlg.h>
#include <oleauto.h>

namespace ash {
namespace migration {

// ============================================================================
// Win32 API Compatibility Wrappers Implementation
// ============================================================================

WindowWrapper::WindowWrapper(HWND hwnd) : m_handle(hwnd) {
}

MessageWrapper::MessageWrapper(HMSG hMsg) : m_handle(hMsg) {
}

FileHandleWrapper::FileHandleWrapper(HANDLE hFile) : m_handle(hFile) {
}

// ============================================================================
// MFC API Compatibility Wrappers Implementation
// ============================================================================

CWndWrapper::CWndWrapper(CWnd* pWnd) : m_pWnd(pWnd) {
}

CDocViewWrapper::CDocViewWrapper(CDocView* pDocView) : m_pDocView(pDocView) {
}

// ============================================================================
// ATL API Compatibility Wrappers Implementation
// ============================================================================

template<typename T>
CComPtrWrapper<T>::CComPtrWrapper(T* ptr) : m_ptr(ptr) {
}

// ============================================================================
// Legacy API Compatibility Functions Implementation
// ============================================================================

template<typename T>
std::optional<std::shared_ptr<T>> ConvertToModernHandle(HANDLE handle) {
    if (!handle) {
        return std::nullopt;
    }
    return std::make_shared<T>(handle);
}

std::shared_ptr<WindowWrapper> WrapWindow(HWND hwnd) {
    return std::make_shared<WindowWrapper>(hwnd);
}

template<typename T>
std::optional<std::shared_ptr<T>> ConvertMfcCWnd(CWnd* pWnd) {
    if (!pWnd) {
        return std::nullopt;
    }
    return std::make_shared<T>(pWnd);
}

template<typename T>
std::optional<std::shared_ptr<CComPtrWrapper<T>>> ConvertAtlCComPtr(T* ptr) {
    if (!ptr) {
        return std::nullopt;
    }
    return std::make_shared<CComPtrWrapper<T>>(ptr);
}

// ============================================================================
// Legacy API Compatibility Macros Implementation
// ============================================================================

#define SAFE_CALL(api) \
    do { \
        if (!(api)) { \
            DWORD err = GetLastError(); \
            return false; \
        } \
    } while (false)

#define SAFE_CALL_LOG(api, msg) \
    do { \
        if (!(api)) { \
            DWORD err = GetLastError(); \
            OutputDebugStringA(msg); \
            return false; \
        } \
    } while (false)

// ============================================================================
// Legacy API Compatibility Classes Implementation
// ============================================================================

GdiObjectWrapper::GdiObjectWrapper(HGDIOBJ hObject) : m_handle(hObject) {
}

GdiPlusObjectWrapper::GdiPlusObjectWrapper(HGDIOBJ hObject) : m_handle(hObject) {
}

} // namespace migration
} // namespace ash
