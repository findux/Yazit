#include "FindState.h"
#include "EditorTab.h"
#include <regex>
#include <algorithm>
#include <cctype>
#include <cstdio>

TextEditor::Coordinates IdxToCoord(const std::string& t, size_t idx) {
    int ln = 0, col = 0;
    for (size_t i = 0; i < idx && i < t.size(); i++)
        if (t[i] == '\n') { ln++; col = 0; } else col++;
    return {ln, col};
}

size_t CoordToIdx(const std::string& t, TextEditor::Coordinates c) {
    size_t i = 0; int ln = 0, col = 0;
    while (i < t.size() && !(ln == c.mLine && col == c.mColumn)) {
        if (t[i] == '\n') { ln++; col = 0; } else col++;
        i++;
    }
    return i;
}

std::vector<Match> FindState::FindAll(const std::string& text) const {
    std::vector<Match> res;
    if (!find[0]) return res;

    if (useRegex) {
        try {
            auto fl = std::regex_constants::ECMAScript;
            if (!caseSens) fl |= std::regex_constants::icase;
            std::regex re(find, fl);
            for (auto it = std::sregex_iterator(text.begin(), text.end(), re);
                 it != std::sregex_iterator(); ++it)
                res.push_back({(size_t)it->position(), (size_t)it->length()});
        } catch (...) {}
        return res;
    }

    std::string hay = text, needle = find;
    if (!caseSens) {
        for (auto& c : hay)    c = (char)std::tolower((unsigned char)c);
        for (auto& c : needle) c = (char)std::tolower((unsigned char)c);
    }
    auto isW = [](char c) { return std::isalnum((unsigned char)c) || c == '_'; };
    size_t pos = 0, nl = needle.size();
    while ((pos = hay.find(needle, pos)) != std::string::npos) {
        bool ok = !wholeWord ||
            ((pos == 0 || !isW(text[pos - 1])) &&
             (pos + nl >= text.size() || !isW(text[pos + nl])));
        if (ok) res.push_back({pos, nl});
        pos++;
    }
    return res;
}

static bool SelectMatch(EditorTab& tab, const Match& m, const std::string& txt) {
    auto s = IdxToCoord(txt, m.start);
    auto e = IdxToCoord(txt, m.start + m.len);
    tab.editor.SetSelection(s, e);
    tab.editor.SetCursorPosition(s);
    return true;
}

bool FindState::FindNext(EditorTab& tab, bool fwd) {
    auto txt = tab.editor.GetText();
    auto all = FindAll(txt);
    if (all.empty()) { snprintf(msg, sizeof(msg), "Bulunamadi."); return false; }

    size_t cur = CoordToIdx(txt, tab.editor.GetCursorPosition());
    size_t end = cur + tab.editor.GetSelectedText().size();

    if (fwd) {
        for (auto& m : all)
            if (m.start >= end) {
                snprintf(msg, sizeof(msg), "%d eslesme.", (int)all.size());
                return SelectMatch(tab, m, txt);
            }
        if (wrapAround) { snprintf(msg, sizeof(msg), "Basa donuldu."); return SelectMatch(tab, all[0], txt); }
    } else {
        for (int i = (int)all.size() - 1; i >= 0; i--)
            if (all[i].start < cur) {
                snprintf(msg, sizeof(msg), "%d eslesme.", (int)all.size());
                return SelectMatch(tab, all[i], txt);
            }
        if (wrapAround) { snprintf(msg, sizeof(msg), "Sona donuldu."); return SelectMatch(tab, all.back(), txt); }
    }
    snprintf(msg, sizeof(msg), "Bulunamadi.");
    return false;
}

void FindState::ReplaceNext(EditorTab& tab) {
    auto sel = tab.editor.GetSelectedText();
    if (!FindAll(sel).empty()) {
        tab.editor.InsertText(replace);
        tab.modified = true;
    }
    FindNext(tab, true);
}

int FindState::ReplaceAll(EditorTab& tab) {
    auto txt = tab.editor.GetText();
    auto all = FindAll(txt);
    for (int i = (int)all.size() - 1; i >= 0; i--)
        txt.replace(all[i].start, all[i].len, replace);
    if (!all.empty()) {
        tab.editor.SetText(txt);
        tab.modified = true;
    }
    return (int)all.size();
}
