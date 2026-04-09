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
    "C++", "C", "GLSL", "Lua", "SQL", "AngelScript", "Rust", "Düz Metin"
};

static TextEditor::LanguageDefinition LangByIdx(int i) {
    switch (i) {
        case 0: return TextEditor::LanguageDefinition::CPlusPlus();
        case 1: return TextEditor::LanguageDefinition::C();
        case 2: return TextEditor::LanguageDefinition::GLSL();
        case 3: return TextEditor::LanguageDefinition::Lua();
        case 4: return TextEditor::LanguageDefinition::SQL();
        case 5: return TextEditor::LanguageDefinition::AngelScript();
        case 6: return TextEditor::LanguageDefinition::Rust();
        default: return {};
    }
}

// Uzantıdan dil indeksini tahmin et
static int LangIdxFromPath(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return 7;
    std::string ext = path.substr(dot + 1);
    for (auto& c : ext) c = (char)tolower((unsigned char)c);
    if (ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "hpp" || ext == "hxx") return 0;
    if (ext == "c"   || ext == "h")   return 1;
    if (ext == "glsl"|| ext == "vert" || ext == "frag" || ext == "geom") return 2;
    if (ext == "lua") return 3;
    if (ext == "sql") return 4;
    if (ext == "as")  return 5;
    if (ext == "rs")  return 6;
    return 7;
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

