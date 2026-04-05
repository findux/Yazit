#pragma once
#include <string>
#include "TextEditor.h"

// Desteklenen diller
static const char* kLangNames[] = {
    "C++", "C", "GLSL", "Lua", "SQL", "AngelScript", "Duz Metin"
};

static TextEditor::LanguageDefinition LangByIdx(int i) {
    switch (i) {
        case 0: return TextEditor::LanguageDefinition::CPlusPlus();
        case 1: return TextEditor::LanguageDefinition::C();
        case 2: return TextEditor::LanguageDefinition::GLSL();
        case 3: return TextEditor::LanguageDefinition::Lua();
        case 4: return TextEditor::LanguageDefinition::SQL();
        case 5: return TextEditor::LanguageDefinition::AngelScript();
        default: return {};
    }
}

// Uzantıdan dil indeksini tahmin et
static int LangIdxFromPath(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return 6;
    std::string ext = path.substr(dot + 1);
    for (auto& c : ext) c = (char)tolower((unsigned char)c);
    if (ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "hpp" || ext == "hxx") return 0;
    if (ext == "c"   || ext == "h")   return 1;
    if (ext == "glsl"|| ext == "vert" || ext == "frag" || ext == "geom") return 2;
    if (ext == "lua") return 3;
    if (ext == "sql") return 4;
    if (ext == "as")  return 5;
    return 6;
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

    EditorTab();
    void Load(const std::string& filePath);
    void Save();

    static std::string Basename(const std::string& p);
};
