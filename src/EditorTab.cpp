#include "EditorTab.h"
#include <fstream>
#include <iterator>

int EditorTab::s_NextId = 0;

EditorTab::EditorTab() : id(s_NextId++) {
    editor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
    editor.SetShowWhitespaces(false);
}

void EditorTab::Load(const std::string& filePath) {
    path = filePath;
    name = Basename(filePath);

    std::ifstream f(filePath);
    if (f) {
        std::string content(std::istreambuf_iterator<char>(f), {});
        editor.SetText(content);
    }

    langIdx = LangIdxFromPath(filePath);
    editor.SetLanguageDefinition(LangByIdx(langIdx));
    modified = false;
}

void EditorTab::Save() {
    if (path.empty()) return;
    std::ofstream f(path);
    f << editor.GetText();
    modified = false;
}

std::string EditorTab::Basename(const std::string& p) {
    auto i = p.find_last_of("/\\");
    return i == std::string::npos ? p : p.substr(i + 1);
}
