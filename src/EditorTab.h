#pragma once
#include <string>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "TextEditor.h"

enum class Encoding { ANSI, UTF8, UTF8_BOM };

// Desteklenen diller
static const char* kLangNames[] = {
    "C++", "C", "GLSL", "Lua", "SQL", "AngelScript", "Rust", "JSON", "G-Code", "Düz Metin"
};

static TextEditor::LanguageDefinitionId LangByIdx(int i) {
    using ID = TextEditor::LanguageDefinitionId;
    switch (i) {
        case 0: return ID::Cpp;
        case 1: return ID::C;
        case 2: return ID::Glsl;
        case 3: return ID::Lua;
        case 4: return ID::Sql;
        case 5: return ID::AngelScript;
        case 6: return ID::Rust;
        case 7: return ID::Json;
        case 8: return ID::GCode;
        default: return ID::None;
    }
}

// Uzantıdan dil indeksini tahmin et
static int LangIdxFromPath(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return 8;
    std::string ext = path.substr(dot + 1);
    for (auto& c : ext) c = (char)tolower((unsigned char)c);
    if (ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "hpp" || ext == "hxx") return 0;
    if (ext == "c"   || ext == "h")   return 1;
    if (ext == "glsl"|| ext == "vert" || ext == "frag" || ext == "geom") return 2;
    if (ext == "lua") return 3;
    if (ext == "sql") return 4;
    if (ext == "as")  return 5;
    if (ext == "rs")  return 6;
    if (ext == "json") return 7;
    if (ext == "gcode" || ext == "nc" || ext == "cnc" ||
        ext == "tap"   || ext == "ngc" || ext == "g") return 8;
    return 9;
}

class EditorTab {
public:
    static int s_NextId;

    int         id;
    std::string name     = "Yeni";
    std::string path;
    TextEditor  editor;
    bool        modified = false;
    int         langIdx  = 0;
    Encoding    encoding = Encoding::UTF8;
    FILETIME    diskModTime = {};   // disk'teki son bilinen değişiklik zamanı

    EditorTab();
    void Load(const std::string& filePath);
    bool Save(std::string* outError = nullptr); // false + hata mesajı döner
    void SetEncoding(Encoding newEnc);  // dönüştür + işaretle

    const char* EncodingName() const;

    // Disk üzerindeki dosyanın son yazma zamanını döndürür (UTF-8 yol)
    static FILETIME     ReadFileMTime(const std::string& utf8path);

    static std::string Basename(const std::string& p);

private:
    static bool        IsLikelyAnsi(const std::string& raw);
    static std::string AnsiToUtf8  (const std::string& ansi);
    static std::string Utf8ToAnsi  (const std::string& utf8, bool* lossyOut = nullptr);
};

