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

// JSON için özel dil tanımı (TextEditor sözdizim vurgulama)
static TextEditor::LanguageDefinition JsonLangDef() {
    static bool inited = false;
    static TextEditor::LanguageDefinition def;
    if (!inited) {
        // Anahtar kelimeler
        for (auto& kw : { "true", "false", "null" })
            def.mKeywords.insert(kw);

        using PI = TextEditor::PaletteIndex;
        // String:  "..." (kaçış sekansları dahil)
        def.mTokenRegexStrings.push_back({ "\\\"(\\\\.|[^\\\"])*\\\"", PI::String });
        // Sayı:    tam sayı ve ondalıklı
        def.mTokenRegexStrings.push_back({ "-?[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?", PI::Number });
        // Noktalama
        def.mTokenRegexStrings.push_back({ "[\\{\\}\\[\\]:,]", PI::Punctuation });
        // Tanımlayıcı (true/false/null anahtar kelime olarak renklendirilir)
        def.mTokenRegexStrings.push_back({ "[a-zA-Z_][a-zA-Z0-9_]*", PI::Identifier });

        def.mName             = "JSON";
        def.mCaseSensitive    = true;
        def.mAutoIndentation  = true;
        inited = true;
    }
    return def;
}

// G-Code dil tanımı — her token grubu ayrı PaletteIndex ile renklendiriliyor
static TextEditor::LanguageDefinition GCodeLangDef() {
    static bool inited = false;
    static TextEditor::LanguageDefinition def;
    if (!inited) {
        using PI = TextEditor::PaletteIndex;

        // ── Regex'ler öncelik sırasıyla (ilk eşleşen kazanır) ─────────────────

        // N satır numaraları: N10, N0100 vb.  →  Preprocessor (gri)
        def.mTokenRegexStrings.push_back({ "[Nn][0-9]+", PI::Preprocessor });

        // Hareket G-kodları (kesme/hareket): G0/G00‥G4/G04, G28,G29,G30,G38,
        //   G73,G76, G80‥G89  →  Keyword (turuncu-kırmızı)
        def.mTokenRegexStrings.push_back({
            "[Gg](?:0*[0-4]|2[89]|3[08]|7[36]|8[0-9])(?![0-9])",
            PI::Keyword });

        // Modal/koordinat G-kodları: G10,G17‥G21,G40‥G49,G53‥G59,G61,G64,
        //   G90‥G99 — catch-all: herhangi bir G + rakam  →  KnownIdentifier (mor)
        def.mTokenRegexStrings.push_back({ "[Gg][0-9]+", PI::KnownIdentifier });

        // M-kodları: M0‥M99 vb.  →  CharLiteral (cyan)
        def.mTokenRegexStrings.push_back({ "[Mm][0-9]+", PI::CharLiteral });

        // Eksen harfleri + değer: X, Y, Z, A, B, C  →  Identifier (mavi)
        def.mTokenRegexStrings.push_back({
            "[XYZABCxyzabc][+-]?(?:[0-9]+\\.?[0-9]*|\\.[0-9]+)",
            PI::Identifier });

        // Besleme/hız/takım/ofset: F, S, T, H, D + değer  →  PreprocIdentifier (altın sarı)
        def.mTokenRegexStrings.push_back({
            "[FSTHDfsthdDd][+-]?(?:[0-9]+\\.?[0-9]*|\\.[0-9]+)",
            PI::PreprocIdentifier });

        // Yay merkezi/döngü parametreleri: I, J, K, R, P, Q, L, E + değer  →  String (pembe)
        def.mTokenRegexStrings.push_back({
            "[IJKRPQLEijkrpqle][+-]?(?:[0-9]+\\.?[0-9]*|\\.[0-9]+)",
            PI::String });

        // Serbest sayılar  →  Number (yeşil)
        def.mTokenRegexStrings.push_back({
            "[+-]?(?:[0-9]+\\.?[0-9]*|\\.[0-9]+)", PI::Number });

        def.mSingleLineComment = ";";
        def.mCommentStart      = "(";
        def.mCommentEnd        = ")";
        def.mName              = "G-Code";
        def.mCaseSensitive     = false;
        def.mAutoIndentation   = false;
        inited = true;
    }
    return def;
}

static TextEditor::LanguageDefinition LangByIdx(int i) {
    switch (i) {
        case 0: return TextEditor::LanguageDefinition::CPlusPlus();
        case 1: return TextEditor::LanguageDefinition::C();
        case 2: return TextEditor::LanguageDefinition::GLSL();
        case 3: return TextEditor::LanguageDefinition::Lua();
        case 4: return TextEditor::LanguageDefinition::SQL();
        case 5: return TextEditor::LanguageDefinition::AngelScript();
        case 6: return TextEditor::LanguageDefinition::Rust();
        case 7: return JsonLangDef();
        case 8: return GCodeLangDef();
        default: return {};
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

