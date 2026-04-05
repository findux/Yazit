#pragma once
#include <string>
#include <vector>
#include "TextEditor.h"

struct Match { size_t start, len; };

// Koordinat <-> string indeks dönüşümleri
TextEditor::Coordinates IdxToCoord(const std::string& text, size_t idx);
size_t                  CoordToIdx(const std::string& text, TextEditor::Coordinates c);

class EditorTab; // ileri bildirim

class FindState {
public:
    char find[512]    = {};
    char replace[512] = {};
    bool caseSens    = false;
    bool wholeWord   = false;
    bool useRegex    = false;
    bool wrapAround  = true;
    bool searchDown  = true;
    bool showWindow  = false;
    bool showReplace = false;
    char msg[256]    = {};

    // Metin içinde tüm eşleşmeleri bul
    std::vector<Match> FindAll(const std::string& text) const;

    // İmlecten itibaren ileri/geri arama, seçim yapar
    bool FindNext(EditorTab& tab, bool forward);

    // Seçili metni değiştir, sonraki eşleşmeye geç
    void ReplaceNext(EditorTab& tab);

    // Tüm eşleşmeleri değiştir, kaç adet değiştirildiğini döndür
    int ReplaceAll(EditorTab& tab);
};
