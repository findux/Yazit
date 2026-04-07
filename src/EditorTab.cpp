#include "EditorTab.h"
#include <fstream>
#include <iterator>

// UTF-8 patikayı std::ifstream/ofstream için geniş karaktere çevirir (MSVC gereksinimi)
static std::wstring PathToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring w(n - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &w[0], n);
    return w;
}

int EditorTab::s_NextId = 0;

// ─── Yardımcı: ANSI mi? ───────────────────────────────────────────────────────
bool EditorTab::IsLikelyAnsi(const std::string& s) {
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) { i++; continue; }
        int extra;
        if      (c >= 0xF0 && c <= 0xF4) extra = 3;
        else if (c >= 0xE0)               extra = 2;
        else if (c >= 0xC2)               extra = 1;
        else return true;   // 0x80-0xC1 → geçersiz UTF-8 başlangıcı → ANSI
        for (int j = 1; j <= extra; j++) {
            if (i + j >= s.size() || ((unsigned char)s[i+j] & 0xC0) != 0x80)
                return true; // geçersiz devam baytı → ANSI
        }
        i += extra + 1;
    }
    return false; // geçerli UTF-8 (veya saf ASCII → UTF-8 say)
}

// ─── Yardımcı: ANSI → UTF-8 ─────────────────────────────────────────────────
std::string EditorTab::AnsiToUtf8(const std::string& ansi) {
    if (ansi.empty()) return ansi;
    int wlen = MultiByteToWideChar(CP_ACP, 0, ansi.data(), (int)ansi.size(), nullptr, 0);
    if (wlen <= 0) return ansi;
    std::wstring wide(wlen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, ansi.data(), (int)ansi.size(), &wide[0], wlen);
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wlen, nullptr, 0, nullptr, nullptr);
    if (ulen <= 0) return ansi;
    std::string utf8(ulen, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), wlen, &utf8[0], ulen, nullptr, nullptr);
    return utf8;
}

// ─── Yardımcı: UTF-8 → ANSI ─────────────────────────────────────────────────
// Başarısız olursa boş string yerine orijinal UTF-8'i döndürür, hata flag'i set eder
std::string EditorTab::Utf8ToAnsi(const std::string& utf8, bool* lossyOut) {
    if (lossyOut) *lossyOut = false;
    if (utf8.empty()) return utf8;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
    if (wlen <= 0) return utf8;                   // dönüşüm başarısız, orijinal kalsın
    std::wstring wide(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), &wide[0], wlen);

    // WC_NO_BEST_FIT_CHARS + lossy flag → hedef kod sayfasında karşılığı olmayan
    // karakterler için varsayılan '?' yazılır ve lossy = true döner
    BOOL usedDefault = FALSE;
    int alen = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS,
                                   wide.data(), wlen, nullptr, 0,
                                   nullptr, &usedDefault);
    if (alen <= 0) return utf8;
    std::string ansi(alen, '\0');
    WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS,
                        wide.data(), wlen, &ansi[0], alen,
                        nullptr, &usedDefault);
    if (lossyOut) *lossyOut = (usedDefault == TRUE);
    return ansi;
}

// ─── Disk değişiklik zamanı ───────────────────────────────────────────────────
FILETIME EditorTab::ReadFileMTime(const std::string& utf8path) {
    FILETIME ft = {};
    WIN32_FILE_ATTRIBUTE_DATA fa;
    if (GetFileAttributesExW(PathToWide(utf8path).c_str(), GetFileExInfoStandard, &fa))
        ft = fa.ftLastWriteTime;
    return ft;
}

// ─── Yapıcı ──────────────────────────────────────────────────────────────────
EditorTab::EditorTab() : id(s_NextId++) {
    editor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
    editor.SetShowWhitespaces(false);
}

// ─── Yükleme ─────────────────────────────────────────────────────────────────
void EditorTab::Load(const std::string& filePath) {
    path = filePath;
    name = Basename(filePath);

    std::ifstream f(PathToWide(filePath), std::ios::binary);
    if (f) {
        std::string raw(std::istreambuf_iterator<char>(f), {});

        // BOM kontrolü
        if (raw.size() >= 3 &&
            (unsigned char)raw[0] == 0xEF &&
            (unsigned char)raw[1] == 0xBB &&
            (unsigned char)raw[2] == 0xBF) {
            encoding = Encoding::UTF8_BOM;
            raw = raw.substr(3);        // editöre BOM'suz ver
        } else if (IsLikelyAnsi(raw)) {
            encoding = Encoding::ANSI;
            raw = AnsiToUtf8(raw);      // editör içinde UTF-8 çalışır
        } else {
            encoding = Encoding::UTF8;
        }

        editor.SetText(raw);
    }

    langIdx     = LangIdxFromPath(filePath);
    editor.SetLanguageDefinition(LangByIdx(langIdx));
    modified    = false;
    diskModTime = ReadFileMTime(filePath);   // dış değişiklik tespiti için referans
}

// ─── Kaydetme ────────────────────────────────────────────────────────────────
bool EditorTab::Save(std::string* outError) {
    if (path.empty()) {
        if (outError) *outError = "Dosya yolu yok.";
        return false;
    }

    std::string text = editor.GetText();

    // TextEditor sona fazladan \n ekleyebilir; varsa kırp
    if (!text.empty() && text.back() == '\n')
        text.pop_back();

    std::ofstream f(PathToWide(path), std::ios::binary);
    if (!f.is_open()) {
        if (outError) *outError = "Dosya acilamadi: " + path;
        return false;
    }

    if (encoding == Encoding::UTF8_BOM) {
        f << '\xEF' << '\xBB' << '\xBF';
        f.write(text.data(), (std::streamsize)text.size());
    } else if (encoding == Encoding::ANSI) {
        bool lossy = false;
        std::string ansi = Utf8ToAnsi(text, &lossy);
        if (ansi.empty() && !text.empty()) {
            if (outError) *outError = "UTF-8 → ANSI donusumu basarisiz.";
            return false;
        }
        f.write(ansi.data(), (std::streamsize)ansi.size());
        if (lossy && outError)
            *outError = "Uyari: Bazi karakterler ANSI'de karsilik bulunamadi, '?' ile degistirildi.";
    } else {
        f.write(text.data(), (std::streamsize)text.size());
    }

    if (!f.good()) {
        if (outError) *outError = "Yazma hatasi: " + path;
        return false;
    }

    modified    = false;
    diskModTime = ReadFileMTime(path);   // başarılı kayıt sonrası referansı güncelle
    return true;
}

// ─── Encoding dönüşümü ───────────────────────────────────────────────────────
void EditorTab::SetEncoding(Encoding newEnc) {
    if (newEnc == encoding) return;

    // ANSI → UTF-8/BOM: editördeki metin zaten UTF-8, sadece flag değişir
    // UTF-8/BOM → ANSI: editördeki metin UTF-8'den ANSI'ye çevrilir
    // UTF-8 ↔ UTF-8 BOM: sadece flag değişir

    if (encoding == Encoding::ANSI &&
        (newEnc == Encoding::UTF8 || newEnc == Encoding::UTF8_BOM)) {
        // Editör içinde zaten UTF-8 var (Load sırasında çevirildi)
        // Sadece kayıt davranışı değişiyor
    } else if ((encoding == Encoding::UTF8 || encoding == Encoding::UTF8_BOM) &&
               newEnc == Encoding::ANSI) {
        // UTF-8 → ANSI: görsel olarak değişmez, kaydetmede ANSI yazılır
    }

    encoding = newEnc;
    modified = true;
}

// ─── Encoding adı ────────────────────────────────────────────────────────────
const char* EditorTab::EncodingName() const {
    switch (encoding) {
        case Encoding::ANSI:     return "ANSI";
        case Encoding::UTF8:     return "UTF-8";
        case Encoding::UTF8_BOM: return "UTF-8 BOM";
        default:                 return "?";
    }
}

// ─── Yardımcı ────────────────────────────────────────────────────────────────
std::string EditorTab::Basename(const std::string& p) {
    auto i = p.find_last_of("/\\");
    return i == std::string::npos ? p : p.substr(i + 1);
}
