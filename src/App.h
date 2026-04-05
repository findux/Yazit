#pragma once
#include <vector>
#include <string>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <SDL2/SDL.h>
#include <imgui.h>
#include "EditorTab.h"
#include "FindState.h"
#include "LuaEngine.h"

class App {
public:
    // ── Ortak durum (LuaEngine ve diğerleri bu alanları kullanır) ──────────
    std::vector<EditorTab> tabs;
    int         leftActive  = 0;
    int         rightActive = 0;
    bool        wantFocusL  = false;
    bool        wantFocusR  = false;
    bool        splitMode   = false;
    FindState   findState;
    std::string statusMsg   = "Hazir.";
    HWND        hwnd        = nullptr;
    LuaEngine   luaEngine;

    // ── Yaşam döngüsü ───────────────────────────────────────────────────────
    void Init(SDL_Window* window);
    void Draw(bool& running);

    // ── Tema ────────────────────────────────────────────────────────────────
    enum class Theme { Dark, Light };
    void SetTheme(Theme t);

    // ── Editör paleti (langIdx == 6 → düz metin, tüm renkler Default'a eşit) ─
    void ApplyEditorPalette(TextEditor& editor, int langIdx);

    // ── Sekme işlemleri ──────────────────���──────────────────────────────────
    void NewTab();
    void OpenFile(const std::string& path);
    int&       ActiveIdx() { return m_activePanel == 0 ? leftActive : rightActive; }
    EditorTab& ActiveTab() { return tabs[ActiveIdx()]; }

private:
    // Lua betik penceresi durumu
    bool        m_luaWinOpen        = false;
    TextEditor  m_luaEditor;
    bool        m_luaOutputAtBottom = true;

    // Hangi panel odakta (0=sol, 1=sağ)
    int         m_activePanel = 0;

    // Tema ve pencere başlığı
    Theme       m_theme     = Theme::Dark;
    SDL_Window* m_sdlWindow = nullptr;

    // Çizim yardımcıları
    void DrawMenuBar(bool& running);
    void DrawPanel(const char* uid, int& active, ImVec2 size, bool& wantFocus);
    void DrawFindWindow();
    void DrawStatusBar();
    void DrawLuaWindow();
    void HandleShortcuts(bool& running);

    // Dosya işlemleri (dialog açar)
    void OpenFileWithDialog();
    void SaveActive();
    void SaveActiveAs();
};
