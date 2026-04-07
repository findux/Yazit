#include "FileDialogs.h"
#include <commdlg.h>
#include <shellapi.h>
#pragma comment(lib, "comdlg32.lib")

// ── UTF-8 <-> UTF-16 dönüşüm yardımcıları ────────────────────────────────────
static std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring out(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &out[0], len);
    return out;
}

static std::string WideToUtf8(const wchar_t* wide, int wlen = -1) {
    if (!wide || wide[0] == L'\0') return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wide, wlen, nullptr, 0, nullptr, nullptr);
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, wlen, &out[0], len, nullptr, nullptr);
    // -1 uzunlukla çağırınca sona null eklenir, onu kırp
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

// Geniş karakter filter dizisi — çift-null sonlu (OPENFILENAMEW gerektirir)
static const wchar_t kFilterW[] =
    L"Tüm Dosyalar\0*.*\0"
    L"C/C++ Kaynak\0*.cpp;*.c;*.cc;*.cxx\0"
    L"C/C++ Başlık\0*.h;*.hpp;*.hxx\0"
    L"Metin\0*.txt\0"
    L"GLSL Shader\0*.glsl;*.vert;*.frag;*.geom\0"
    L"Lua\0*.lua\0"
    L"SQL\0*.sql\0";

std::vector<std::string> OpenFileDialog(HWND owner) {
    // Çoklu seçim için geniş buffer: "klasör\0dosya1\0dosya2\0\0"
    std::vector<wchar_t> buf(32768, L'\0');

    OPENFILENAMEW ofn   = {};
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = owner;
    ofn.lpstrFile       = buf.data();
    ofn.nMaxFile        = (DWORD)buf.size();
    ofn.lpstrFilter     = kFilterW;
    ofn.nFilterIndex    = 1;
    ofn.lpstrTitle      = L"Dosya Aç";
    ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
                          OFN_NOCHANGEDIR   | OFN_EXPLORER      |
                          OFN_ALLOWMULTISELECT;

    std::vector<std::string> result;
    if (!GetOpenFileNameW(&ofn)) return result;

    const wchar_t* p = buf.data();
    std::wstring first = p;          // tek dosya → tam yol, çoklu → klasör
    p += first.size() + 1;

    if (*p == L'\0') {
        // Tek dosya seçildi: buf doğrudan tam yolu içerir
        result.push_back(WideToUtf8(first.c_str()));
    } else {
        // Çoklu dosya: ilk token klasör, kalanlar dosya adları
        while (*p) {
            std::wstring fname = p;
            std::wstring fullPath = first + L"\\" + fname;
            result.push_back(WideToUtf8(fullPath.c_str()));
            p += fname.size() + 1;
        }
    }
    return result;
}

std::string SaveFileDialog(const std::string& current, HWND owner) {
    std::wstring wCurrent = Utf8ToWide(current);
    wchar_t buf[MAX_PATH] = {};
    if (!wCurrent.empty())
        wcsncpy_s(buf, wCurrent.c_str(), MAX_PATH - 1);

    OPENFILENAMEW ofn = {};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = owner;
    ofn.lpstrFile    = buf;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrFilter  = kFilterW;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle   = L"Farklı Kaydet";
    ofn.lpstrDefExt  = L"txt";
    ofn.Flags        = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    return GetSaveFileNameW(&ofn) ? WideToUtf8(buf) : std::string{};
}
