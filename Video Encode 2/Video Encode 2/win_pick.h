#pragma once
#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>
#include <string>

inline std::wstring to_wide(const std::string& s) {
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), len);
    return w;
}
inline std::string to_utf8(const std::wstring& w) {
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), len, nullptr, nullptr);
    return s;
}

inline bool pickOpenFile(std::string& outPath, const wchar_t* title=L"Select a file") {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    IFileOpenDialog* pDlg = nullptr;
    if (SUCCEEDED(hr)) hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDlg));
    if (FAILED(hr)) { CoUninitialize(); return false; }
    pDlg->SetTitle(title);
    DWORD opts=0; pDlg->GetOptions(&opts); pDlg->SetOptions(opts | FOS_FORCEFILESYSTEM);
    hr = pDlg->Show(nullptr);
    if (FAILED(hr)) { pDlg->Release(); CoUninitialize(); return false; }
    IShellItem* pItem=nullptr; hr = pDlg->GetResult(&pItem);
    if (FAILED(hr)) { pDlg->Release(); CoUninitialize(); return false; }
    PWSTR pszFilePath=nullptr; hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
    if (SUCCEEDED(hr)) { outPath = to_utf8(pszFilePath); CoTaskMemFree(pszFilePath); }
    pItem->Release(); pDlg->Release(); CoUninitialize();
    return !outPath.empty();
}

inline bool pickSaveFile(std::string& outPath, const wchar_t* title=L"Save as", const wchar_t* defaultName=L"output") {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    IFileSaveDialog* pDlg = nullptr;
    if (SUCCEEDED(hr)) hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDlg));
    if (FAILED(hr)) { CoUninitialize(); return false; }
    pDlg->SetTitle(title);
    pDlg->SetFileName(defaultName);
    DWORD opts=0; pDlg->GetOptions(&opts); pDlg->SetOptions(opts | FOS_OVERWRITEPROMPT | FOS_FORCEFILESYSTEM);
    hr = pDlg->Show(nullptr);
    if (FAILED(hr)) { pDlg->Release(); CoUninitialize(); return false; }
    IShellItem* pItem=nullptr; hr = pDlg->GetResult(&pItem);
    if (FAILED(hr)) { pDlg->Release(); CoUninitialize(); return false; }
    PWSTR pszFilePath=nullptr; hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
    if (SUCCEEDED(hr)) { outPath = to_utf8(pszFilePath); CoTaskMemFree(pszFilePath); }
    pItem->Release(); pDlg->Release(); CoUninitialize();
    return !outPath.empty();
}
#endif
