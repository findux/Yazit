#include "FileDialogs.h"
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")

static const char* kFilter =
    "Tum Dosyalar\0*.*\0"
    "C/C++ Kaynak\0*.cpp;*.c;*.cc;*.cxx\0"
    "C/C++ Baslik\0*.h;*.hpp;*.hxx\0"
    "Metin\0*.txt\0"
    "GLSL Shader\0*.glsl;*.vert;*.frag;*.geom\0"
    "Lua\0*.lua\0"
    "SQL\0*.sql\0";

std::vector<std::string> OpenFileDialog(HWND owner) {
    // Çoklu seçim için geniş buffer: "klasör\0dosya1\0dosya2\0\0"
    std::vector<char> buf(32768, '\0');
    OPENFILENAMEA ofn  = {};
    ofn.lStructSize    = sizeof(ofn);
    ofn.hwndOwner      = owner;
    ofn.lpstrFile      = buf.data();
    ofn.nMaxFile       = (DWORD)buf.size();
    ofn.lpstrFilter    = kFilter;
    ofn.nFilterIndex   = 1;
    ofn.lpstrTitle     = "Dosya Ac";
    ofn.Flags          = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
                         OFN_NOCHANGEDIR   | OFN_EXPLORER      |
                         OFN_ALLOWMULTISELECT;

    std::vector<std::string> result;
    if (!GetOpenFileNameA(&ofn)) return result;

    const char* p = buf.data();
    std::string first = p;          // tek dosya → tam yol, çoklu → klasör
    p += first.size() + 1;

    if (*p == '\0') {
        // Tek dosya seçildi: buf doğrudan tam yol içerir
        result.push_back(first);
    } else {
        // Çoklu dosya: ilk token klasör, kalanlar dosya adları
        while (*p) {
            std::string fname = p;
            result.push_back(first + "\\" + fname);
            p += fname.size() + 1;
        }
    }
    return result;
}

std::string SaveFileDialog(const std::string& current, HWND owner) {
    char buf[MAX_PATH] = {};
    strncpy_s(buf, current.c_str(), sizeof(buf) - 1);
    OPENFILENAMEA ofn = {};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = owner;
    ofn.lpstrFile    = buf;
    ofn.nMaxFile     = sizeof(buf);
    ofn.lpstrFilter  = kFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle   = "Farkli Kaydet";
    ofn.lpstrDefExt  = "txt";
    ofn.Flags        = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    return GetSaveFileNameA(&ofn) ? buf : std::string{};
}
