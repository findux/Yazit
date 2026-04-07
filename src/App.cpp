#include "App.h"
#include "FileDialogs.h"
#include <SDL2/SDL_syswm.h>
#include <GL/glew.h>
#include <imgui_impl_opengl3.h>
#include "imgui_impl_sdl2.h"
#include <cstdio>
#include <set>

// ─── Tema ─────────────────────────────────────────────────────────────────────
void App::SetTheme(Theme t) {
    m_theme = t;
    ImGuiStyle& st = ImGui::GetStyle();
    if (t == Theme::Dark) {
        ImGui::StyleColorsDark();
        st.Colors[ImGuiCol_WindowBg]  = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
        st.Colors[ImGuiCol_ChildBg]   = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
        st.Colors[ImGuiCol_Tab]       = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
        st.Colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.40f, 0.70f, 1.0f);
    } else {
        ImGui::StyleColorsLight();
        st.Colors[ImGuiCol_WindowBg]  = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);
        st.Colors[ImGuiCol_ChildBg]   = ImVec4(0.98f, 0.98f, 0.98f, 1.0f);
        st.Colors[ImGuiCol_Tab]       = ImVec4(0.80f, 0.80f, 0.83f, 1.0f);
        st.Colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.52f, 0.88f, 1.0f);
        st.Colors[ImGuiCol_TabHovered]= ImVec4(0.60f, 0.75f, 0.95f, 1.0f);
    }
    st.WindowRounding = 3.0f;
    st.FrameRounding  = 2.0f;
    st.TabRounding    = 2.0f;

    // Açık sekmelerin paleti temaya ve dil moduna göre güncelle
    for (auto& tab : tabs)
        ApplyEditorPalette(tab.editor, tab.langIdx);
}

// ─── Editör paleti ────────────────────────────────────────────────────────────
void App::ApplyEditorPalette(TextEditor& editor, int langIdx) {
    bool dark = (m_theme == Theme::Dark);
    auto p    = dark ? TextEditor::GetDarkPalette() : TextEditor::GetLightPalette();

    if (langIdx == 6) {  // Düz Metin: tüm syntax renklerini Default'a eşitle
        ImU32 fg = p[(size_t)TextEditor::PaletteIndex::Default];
        for (size_t i = (size_t)TextEditor::PaletteIndex::Default;
                    i <  (size_t)TextEditor::PaletteIndex::Background; ++i)
            p[i] = fg;
    }

    editor.SetPalette(p);
}

// ─── Başlık güncelleme ────────────────────────────────────────────────────────
static void UpdateWindowTitle(SDL_Window* win, const EditorTab& tab) {
    char title[512];
    const std::string& display = tab.path.empty() ? tab.name : tab.path;
    if (tab.modified)
        snprintf(title, sizeof(title), "YAZIT -%s *", display.c_str());
    else
        snprintf(title, sizeof(title), "YAZIT -%s",   display.c_str());
    SDL_SetWindowTitle(win, title);
}

// ─── Başlatma ────────────────────────────────────────────────────────────────
void App::Init(SDL_Window* window) {
    m_sdlWindow = window;
    SDL_SysWMinfo wm; SDL_VERSION(&wm.version);
    SDL_GetWindowWMInfo(window, &wm);
    hwnd = wm.info.win.window;

    // İlk sekme
    NewTab();
    tabs[0].editor.SetText(
        "#include <iostream>\n\nint main()\n{\n"
        "    std::cout << \"Merhaba!\" << std::endl;\n"
        "    return 0;\n}\n");
    tabs[0].modified = false;
    wantFocusL = false;

    // Lua betik editörü
    m_luaEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
    m_luaEditor.SetShowWhitespaces(false);
    m_luaEditor.SetText(
        "--[[\n"
        "  EDITOR API - Kullanilabilir Fonksiyonlar\n"
        "\n"
        "  editor.get_text([dosya])           -> string\n"
        "      Aktif veya belirtilen dosyanin metnini dondurur.\n"
        "\n"
        "  editor.set_text(metin [, dosya])\n"
        "      Aktif veya belirtilen dosyanin metnini degistirir.\n"
        "\n"
        "  editor.get_name()                  -> string\n"
        "      Aktif sekmenin dosya adini dondurur.\n"
        "\n"
        "  editor.get_path()                  -> string\n"
        "      Aktif sekmenin tam yolunu dondurur.\n"
        "\n"
        "  editor.get_all_names()             -> tablo\n"
        "      Acik tum sekmelerin dosya adlarini {ad1, ad2, ...} olarak dondurur.\n"
        "\n"
        "  editor.get_all_paths()             -> tablo\n"
        "      Acik tum sekmelerin tam yollarini {yol1, yol2, ...} olarak dondurur.\n"
        "\n"
        "  editor.insert(metin)\n"
        "      Imlecin bulundugu konuma metin ekler.\n"
        "\n"
        "  editor.get_cursor()                -> satir, sutun  (1-tabanli)\n"
        "      Imlecin satir ve sutun numarasini dondurur.\n"
        "\n"
        "  editor.set_cursor(satir, sutun)    (1-tabanli)\n"
        "      Imleci belirtilen satir/sutuna tasir.\n"
        "\n"
        "  editor.get_selection()             -> string\n"
        "      Secili metni dondurur.\n"
        "\n"
        "  editor.select_all()\n"
        "      Tum metni secker.\n"
        "\n"
        "  editor.replace_all(bul, yeni)      -> sayi\n"
        "      Aktif dosyada tum eslesmeler degistirilir, degistirilen adet dondurulur.\n"
        "\n"
        "  editor.get_line_count()            -> sayi\n"
        "      Aktif dosyanin satir sayisini dondurur.\n"
        "\n"
        "  editor.new_tab()\n"
        "      Yeni bos bir sekme acar.\n"
        "\n"
        "  editor.open_file(yol)\n"
        "      Belirtilen dosyayi yeni sekmede acar.\n"
        "\n"
        "  editor.save([dosya])\n"
        "      Aktif veya belirtilen dosyayi diske kaydeder.\n"
        "\n"
        "  editor.is_modified([dosya])        -> bool\n"
        "      Dosya kaydedilmemis degisiklik iceriyor mu?\n"
        "\n"
        "  print(...)\n"
        "      Ciktiyi bu pencerenin cikti alanina yazar.\n"
        "--]]\n"
        "\n"
        "-- --- Ornek betik ---\n"
        "print(\"Aktif dosya : \" .. editor.get_name())\n"
        "print(\"Tam yol     : \" .. editor.get_path())\n"
        "print(\"Satir sayisi: \" .. editor.get_line_count())\n"
        "\n"
        "local names = editor.get_all_names()\n"
        "print(\"\\nAcik sekmeler:\")\n"
        "for i, name in ipairs(names) do\n"
        "    local mod = editor.is_modified(name) and \" *\" or \"\"\n"
        "    print(string.format(\"  [%d] %s%s\", i, name, mod))\n"
        "end\n");

    luaEngine.Init(this);

    // Varsayılan tema uygula (sekme paletlerini de ayarlar)
    SetTheme(m_theme);
}

// ─── Ana çizim döngüsü ───────────────────────────────────────────────────────
void App::Draw(bool& running) {
    // Pencere başlığını her frame güncelle
    UpdateWindowTitle(m_sdlWindow, ActiveTab());

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##Root", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize    |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    DrawMenuBar(running);

    const float statusH  = 22.0f;
    const float resultsH = m_showResults ? m_resultsHeight : 0.0f;
    ImVec2 area = ImGui::GetContentRegionAvail();
    area.y -= statusH + resultsH;

    // ItemSpacing = 0 → paneller arası boşluk sıfır, taşma olmaz
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    if (splitMode) {
        float half = area.x * 0.5f - 1.0f;
        DrawPanel("L", leftActive,  {half, area.y}, wantFocusL);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.28f, 0.28f, 0.28f, 1.0f));
        ImGui::BeginChild("##div", {2.0f, area.y}); ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::SameLine();
        DrawPanel("R", rightActive, {half, area.y}, wantFocusR);
    } else {
        DrawPanel("L", leftActive, area, wantFocusL);
    }

    if (m_showResults) DrawResultsPanel();
    DrawStatusBar();
    DrawExitConfirmDialog(running);  // ##Root Begin/End içinde olmalı

    ImGui::PopStyleVar(); // ItemSpacing
    ImGui::End();

    DrawFindWindow();
    DrawLuaWindow();
    HandleShortcuts(running);
}

// ─── Menü çubuğu ─────────────────────────────────────────────────────────────
void App::DrawMenuBar(bool& running) {
    if (!ImGui::BeginMenuBar()) return;

    // ── Dosya ──
    if (ImGui::BeginMenu("Dosya")) {
        if (ImGui::MenuItem("Yeni",            "Ctrl+N"))       NewTab();
        if (ImGui::MenuItem("Ac...",           "Ctrl+O"))       OpenFileWithDialog();
        if (ImGui::MenuItem("Kaydet",          "Ctrl+S"))       SaveActive();
        if (ImGui::MenuItem("Farkli Kaydet..","Ctrl+Shift+S"))  SaveActiveAs();
        ImGui::Separator();
        for (int i = 0; i < (int)tabs.size(); i++) {
            char lbl[256];
            snprintf(lbl, sizeof(lbl), "%d: %s%s", i + 1,
                tabs[i].modified ? "*" : "", tabs[i].name.c_str());
            if (ImGui::MenuItem(lbl, nullptr, leftActive == i))
                { leftActive = i; wantFocusL = true; }
        }
        ImGui::Separator();
        // ── Kodlama alt menüsü ──────────────────────────────────────────────
        if (ImGui::BeginMenu("Kodlama")) {
            ImGui::TextDisabled("Mevcut: %s", ActiveTab().EncodingName());
            ImGui::Separator();
            if (ImGui::MenuItem("ANSI",        nullptr, ActiveTab().encoding == Encoding::ANSI))
                ActiveTab().SetEncoding(Encoding::ANSI);
            if (ImGui::MenuItem("UTF-8",       nullptr, ActiveTab().encoding == Encoding::UTF8))
                ActiveTab().SetEncoding(Encoding::UTF8);
            if (ImGui::MenuItem("UTF-8 BOM",   nullptr, ActiveTab().encoding == Encoding::UTF8_BOM))
                ActiveTab().SetEncoding(Encoding::UTF8_BOM);
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Çıkış", "Alt+F4")) RequestExit(running);
        ImGui::EndMenu();
    }

    // ── Düzenle ──
    if (ImGui::BeginMenu("Düzenle")) {
        if (ImGui::MenuItem("Geri Al",    "Ctrl+Z", false, ActiveTab().editor.CanUndo())) ActiveTab().editor.Undo();
        if (ImGui::MenuItem("Yinele",     "Ctrl+Y", false, ActiveTab().editor.CanRedo())) ActiveTab().editor.Redo();
        ImGui::Separator();
        if (ImGui::MenuItem("Kes",        "Ctrl+X")) ActiveTab().editor.Cut();
        if (ImGui::MenuItem("Kopyala",    "Ctrl+C")) ActiveTab().editor.Copy();
        if (ImGui::MenuItem("Yapistir",   "Ctrl+V")) ActiveTab().editor.Paste();
        if (ImGui::MenuItem("Tümünü Seç", "Ctrl+A")) ActiveTab().editor.SelectAll();
        ImGui::EndMenu();
    }

    // ── Ara ──
    if (ImGui::BeginMenu("Ara")) {
        if (ImGui::MenuItem("Bul...",             "Ctrl+F")) { findState.showWindow = true; findState.showReplace = false; }
        if (ImGui::MenuItem("Bul ve Değiştir...", "Ctrl+H")) { findState.showWindow = true; findState.showReplace = true;  }
        ImGui::Separator();
        if (ImGui::MenuItem("Sonraki", "F3")) findState.FindNext(ActiveTab(), true);
        if (ImGui::MenuItem("Önceki",  "F2")) findState.FindNext(ActiveTab(), false);
        ImGui::EndMenu();
    }

    // ── Görünüm ──
    if (ImGui::BeginMenu("Görünüm")) {
        if (ImGui::MenuItem("Yan Yana", "Ctrl+\\", splitMode)) splitMode = !splitMode;
        ImGui::Separator();
        if (ImGui::MenuItem("Koyu Tema",  nullptr, m_theme == Theme::Dark))  SetTheme(Theme::Dark);
        if (ImGui::MenuItem("Acık Tema",  nullptr, m_theme == Theme::Light)) SetTheme(Theme::Light);
        ImGui::Separator();
        ImGui::Text("Dil:"); ImGui::Separator();
        for (int i = 0; i < IM_ARRAYSIZE(kLangNames); i++) {
            if (ImGui::MenuItem(kLangNames[i], nullptr, ActiveTab().langIdx == i)) {
                ActiveTab().langIdx = i;
                ActiveTab().editor.SetLanguageDefinition(LangByIdx(i));
                ApplyEditorPalette(ActiveTab().editor, i);
            }
        }
        ImGui::Separator();
        bool ws = ActiveTab().editor.IsShowingWhitespaces();
        if (ImGui::MenuItem("Boşluk Göster", nullptr, ws))
            ActiveTab().editor.SetShowWhitespaces(!ws);
        ImGui::Separator();
        if (ImGui::MenuItem("Pencere Düzenini Sıfırla")) {
            // Boş ini yükleyerek tüm pencere konumlarını temizle, diske yaz
            ImGui::LoadIniSettingsFromMemory("", 0);
            const char* ini = ImGui::GetIO().IniFilename;
            if (ini && *ini) ImGui::SaveIniSettingsToDisk(ini);
            statusMsg = "Pencere duzeni sifirlandi.";
        }
        if (ImGui::MenuItem("imgui.ini Düzenle...")) {
            const char* ini = ImGui::GetIO().IniFilename;
            if (ini && *ini) {
                // Güncel pencere durumunu önce diske yaz
                ImGui::SaveIniSettingsToDisk(ini);
                OpenFile(ini);
            }
        }
        ImGui::EndMenu();
    }

    // ── Lua ──
    if (ImGui::BeginMenu("Lua")) {
        if (ImGui::MenuItem("Lua Betik Editoru", "F6", m_luaWinOpen))
            m_luaWinOpen = !m_luaWinOpen;
        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

// ─── Editör paneli ───────────────────────────────────────────────────────────
void App::DrawPanel(const char* uid, int& active, ImVec2 size, bool& wantFocus) {
    ImGui::PushID(uid);
    ImGui::BeginChild(uid, size, false, ImGuiWindowFlags_NoScrollbar);

    // Bu panel odakta mı? (editör dahil tüm alt pencereler)
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_RootWindow))
        m_activePanel = (uid[0] == 'L') ? 0 : 1;

    if (ImGui::BeginTabBar("TB",
        ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll |
        ImGuiTabBarFlags_TabListPopupButton))
    {
        for (int i = 0; i < (int)tabs.size(); i++) {
            char lbl[256];
            snprintf(lbl, sizeof(lbl), "%s%s###T%d",
                tabs[i].modified ? "*" : "", tabs[i].name.c_str(), tabs[i].id);

            ImGuiTabItemFlags fl = (wantFocus && active == i)
                                   ? ImGuiTabItemFlags_SetSelected
                                   : ImGuiTabItemFlags_None;
            bool open = true;
            if (ImGui::BeginTabItem(lbl, &open, fl)) {
                if (!wantFocus || active == i) {
                    active    = i;
                    wantFocus = false;
                }
                ImGui::EndTabItem();
            }
            if (!open && (int)tabs.size() > 1) {
                tabs.erase(tabs.begin() + i);
                if (active >= (int)tabs.size()) active = (int)tabs.size() - 1;
                i--;
            }
        }
        ImGui::EndTabBar();
    }

    if (active >= 0 && active < (int)tabs.size()) {
        EditorTab& tab = tabs[active];
        tab.editor.Render("##Ed", ImGui::GetContentRegionAvail());
        if (tab.editor.IsTextChanged()) tab.modified = true;
    }

    ImGui::EndChild();
    ImGui::PopID();
}

// ─── Bul/Değiştir penceresi ──────────────────────────────────────────────────
void App::DrawFindWindow() {
    if (!findState.showWindow) return;
    ImGui::SetNextWindowSize(ImVec2(500, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(300, 120), ImGuiCond_FirstUseEver);

    const char* title = findState.showReplace ? "Bul ve Degistir###FR" : "Bul###FR";
    if (!ImGui::Begin(title, &findState.showWindow, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End(); return;
    }

    if (ImGui::SmallButton(findState.showReplace ? "[ Sadece Bul ]" : "[ Bul ve Degistir ]"))
        findState.showReplace = !findState.showReplace;
    ImGui::Separator();

    const float labelW = 80.0f;
    ImGui::Text("Bul:");     ImGui::SameLine(labelW); ImGui::SetNextItemWidth(300);
    if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
    bool enter = ImGui::InputText("##Find", findState.find, sizeof(findState.find),
                                  ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (ImGui::SmallButton("x##cf")) {
        memset(findState.find, 0, sizeof(findState.find));
        memset(findState.msg,  0, sizeof(findState.msg));
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Aramayi temizle");

    if (findState.showReplace) {
        ImGui::Text("Degistir:"); ImGui::SameLine(labelW); ImGui::SetNextItemWidth(300);
        ImGui::InputText("##Repl", findState.replace, sizeof(findState.replace));
        ImGui::SameLine();
        if (ImGui::SmallButton("x##cr")) {
            memset(findState.replace, 0, sizeof(findState.replace));
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Degistirmeyi temizle");
    }

    ImGui::Separator();
    ImGui::Columns(2, "opts", false);
    ImGui::Checkbox("Buyuk/Kucuk Harf",   &findState.caseSens);
    ImGui::Checkbox("Tam Kelime",          &findState.wholeWord);
    ImGui::NextColumn();
    ImGui::Checkbox("Regex",               &findState.useRegex);
    ImGui::Checkbox("Dongusel",            &findState.wrapAround);
    ImGui::Columns(1);

    ImGui::Text("Yon:"); ImGui::SameLine();
    if (ImGui::RadioButton("Yukari", !findState.searchDown)) findState.searchDown = false;
    ImGui::SameLine();
    if (ImGui::RadioButton("Asagi",   findState.searchDown)) findState.searchDown = true;
    ImGui::Separator();

    bool valid = leftActive >= 0 && leftActive < (int)tabs.size();
    if ((ImGui::Button("Bul Sonraki") || enter) && valid)
        findState.FindNext(tabs[leftActive], findState.searchDown);
    ImGui::SameLine();
    if (ImGui::Button("Bul Önceki") && valid)
        findState.FindNext(tabs[leftActive], !findState.searchDown);
    if (findState.showReplace) {
        ImGui::SameLine();
        if (ImGui::Button("Degiştir") && valid)          findState.ReplaceNext(tabs[leftActive]);
        ImGui::SameLine();
        if (ImGui::Button("Tümünü Değiştir") && valid) {
            int n = findState.ReplaceAll(tabs[leftActive]);
            snprintf(findState.msg, sizeof(findState.msg), "%d yer degistirildi.", n);
        }
    }
    if (ImGui::Button("Aktif Dosyada Bul")) SearchInTabs(true);
    ImGui::SameLine();
    if (ImGui::Button("Tüm Dosyalarda Bul")) SearchInTabs(false);
    if (findState.msg[0]) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "%s", findState.msg);
    }
    ImGui::End();
}

// ─── Durum çubuğu ────────────────────────────────────────────────────────────
void App::DrawStatusBar() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.14f, 0.20f, 1.0f));
    ImGui::BeginChild("##status", {0, 22.0f}, false, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPosY(3);
    auto cp = ActiveTab().editor.GetCursorPosition();
    ImGui::Text("  Satir: %d  Sutun: %d  |  %s  |  %s  |  %s  |  %s  |  %s",
        cp.mLine + 1, cp.mColumn + 1,
        ActiveTab().editor.IsOverwrite() ? "OVR" : "INS",
        kLangNames[ActiveTab().langIdx],
        ActiveTab().EncodingName(),
        ActiveTab().modified ? "*Degistirildi" : "Kaydedildi",
        statusMsg.c_str());
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ─── Lua betik penceresi ─────────────────────────────────────────────────────
void App::DrawLuaWindow() {
    if (!m_luaWinOpen) return;
    ImGui::SetNextWindowSize(ImVec2(820, 620), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Lua Betik Editoru", &m_luaWinOpen)) { ImGui::End(); return; }

    // Araç çubuğu
    if (ImGui::Button("Çaliştir (F5)"))
        luaEngine.Execute(m_luaEditor.GetText());
    ImGui::SameLine();
    if (ImGui::Button("Çıktıyı Temizle")) luaEngine.ClearOutput();
    ImGui::SameLine();

    // Hızlı başvuru
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::BeginTooltip();
        ImGui::Text(
            "editor.get_text([dosya])          Metin al\n"
            "editor.set_text(metin[, dosya])   Metin ata\n"
            "editor.get_name()                 Aktif dosya adi\n"
            "editor.get_path()                 Aktif dosya yolu\n"
            "editor.get_all_names()            {ad1, ad2, ...}\n"
            "editor.get_all_paths()            {yol1, yol2, ...}\n"
            "editor.insert(metin)              Imlec konumuna ekle\n"
            "editor.get_cursor()     -> satir, sutun  (1-tabanli)\n"
            "editor.set_cursor(satir, sutun)   Imleci tasI\n"
            "editor.get_selection()            Secili metni al\n"
            "editor.select_all()               Tumunu sec\n"
            "editor.replace_all(bul, yeni)  -> sayi\n"
            "editor.get_line_count()           Satir sayisi\n"
            "editor.new_tab()                  Yeni sekme\n"
            "editor.open_file(yol)             Dosya ac\n"
            "editor.save([dosya])              Kaydet\n"
            "editor.is_modified([dosya])       Degistirildi mi?\n"
            "print(...)                        Bu pencereye yaz");
        ImGui::EndTooltip();
    }

    ImGui::Separator();

    float totalH = ImGui::GetContentRegionAvail().y;
    float edH    = totalH * 0.60f;
    float outH   = totalH - edH - 30.0f;

    // Betik editörü
    m_luaEditor.Render("##LuaEd", {-1.0f, edH});

    ImGui::Separator();
    ImGui::Text("Cikti:");

    // Çıktı alanı
    ImGui::BeginChild("##LuaOut", {-1.0f, outH}, true,
        ImGuiWindowFlags_HorizontalScrollbar);
    const std::string& out = luaEngine.GetOutput();
    ImGui::TextUnformatted(out.c_str(), out.c_str() + out.size());
    // Otomatik aşağı kaydır
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f)
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    ImGui::End();
}

// ─── Klavye kısayolları ───────────────────────────────────────────────────────
void App::HandleShortcuts(bool& running) {
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsKeyPressed(ImGuiKey_F5) && m_luaWinOpen)
        luaEngine.Execute(m_luaEditor.GetText());
    if (ImGui::IsKeyPressed(ImGuiKey_F6))
        m_luaWinOpen = !m_luaWinOpen;
    if (ImGui::IsKeyPressed(ImGuiKey_F3))
        findState.FindNext(ActiveTab(), true);
    if (ImGui::IsKeyPressed(ImGuiKey_F2))
        findState.FindNext(ActiveTab(), false);

    if (!io.KeyCtrl) return;

    if (ImGui::IsKeyPressed(ImGuiKey_N))           NewTab();
    if (ImGui::IsKeyPressed(ImGuiKey_O))           OpenFileWithDialog();
    if (ImGui::IsKeyPressed(ImGuiKey_S)) {
        if (io.KeyShift) SaveActiveAs(); else SaveActive();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F)) { findState.showWindow = true; findState.showReplace = false; }
    if (ImGui::IsKeyPressed(ImGuiKey_H)) { findState.showWindow = true; findState.showReplace = true;  }
    if (ImGui::IsKeyPressed(ImGuiKey_Backslash)) splitMode = !splitMode;
    if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        if (m_activePanel == 0) {
            leftActive = (leftActive + 1) % (int)tabs.size();
            wantFocusL = true;
        } else {
            rightActive = (rightActive + 1) % (int)tabs.size();
            wantFocusR = true;
        }
    }
}

// ─── Çıkış onayı ─────────────────────────────────────────────────────────────
void App::RequestExit(bool& running) {
    // Sadece modified olan sekmeleri kuyruğa al
    m_exitTabQueue.clear();
    for (int i = 0; i < (int)tabs.size(); i++)
        if (tabs[i].modified)
            m_exitTabQueue.push_back(i);

    if (m_exitTabQueue.empty()) {
        running = false;
    } else {
        // İlk modified sekmeye geç
        leftActive = m_exitTabQueue.front();
        wantFocusL = true;
        m_exitConfirmOpen = true;
    }
}

void App::DrawExitConfirmDialog(bool& running) {
    if (!m_exitConfirmOpen) return;

    // Kuyruktan artık modified olmayan veya silinmiş sekmeleri temizle
    while (!m_exitTabQueue.empty()) {
        int idx = m_exitTabQueue.front();
        if (idx >= (int)tabs.size() || !tabs[idx].modified)
            m_exitTabQueue.erase(m_exitTabQueue.begin());
        else
            break;
    }

    // Kuyruk bittiyse çık
    if (m_exitTabQueue.empty()) {
        running = false;
        m_exitConfirmOpen = false;
        return;
    }

    // Aktif sekmeyi kuyruğun başındakine getir
    leftActive = m_exitTabQueue.front();
    wantFocusL = true;

    // Popup ##Root içinde açılır — her frame OpenPopup çağrısı gerekli
    ImGui::OpenPopup("###ExitConfirm");

    ImGui::SetNextWindowSize(ImVec2(380, 0), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    bool popupOpen = true;  // ImGui X'e basınca bunu false yapar
    if (ImGui::BeginPopupModal("Kaydedilmemis Degisiklik###ExitConfirm",
        &popupOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {
        EditorTab& cur = tabs[m_exitTabQueue.front()];
        ImGui::TextWrapped("\"%s\" dosyasinda kaydedilmemis degisiklikler var.",
                           cur.name.c_str());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float btnW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 3) * 0.25f;

        auto advanceQueue = [&]() {
            m_exitTabQueue.erase(m_exitTabQueue.begin());
            if (m_exitTabQueue.empty()) {
                running = false;
                m_exitConfirmOpen = false;
                ImGui::CloseCurrentPopup();
            }
        };

        if (ImGui::Button("Kaydet", ImVec2(btnW, 0))) {
            SaveActive();
            if (!cur.modified) advanceQueue();
        }
        ImGui::SameLine();
        if (ImGui::Button("Farklı Kaydet", ImVec2(btnW*1.5, 0))) {
            SaveActiveAs();
            if (!cur.modified) advanceQueue();
        }
        ImGui::SameLine();
        if (ImGui::Button("Kaydetme", ImVec2(btnW, 0))) {
            advanceQueue();
        }
        //ImGui::SameLine();
        if (ImGui::Button("Tümünü Kaydetmeden Çık", ImVec2(btnW*2.25, 0))) {
            running = false;
            m_exitConfirmOpen = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // X'e basıldıysa popupOpen=false olur, BeginPopupModal bloğuna girilmez
    // → çıkıştan vazgeç
    if (!popupOpen) {
        m_exitConfirmOpen = false;
        m_exitTabQueue.clear();
    }
}

// ─── Dosyalarda arama ────────────────────────────────────────────────────────
void App::SearchInTabs(bool activeOnly) {
    m_searchResults.clear();
    int start = activeOnly ? leftActive : 0;
    int end   = activeOnly ? leftActive + 1 : (int)tabs.size();

    for (int t = start; t < end; t++) {
        std::string text  = tabs[t].editor.GetText();
        auto        lines = tabs[t].editor.GetTextLines();
        auto        all   = findState.FindAll(text);

        // Her satırda yalnızca ilk eşleşmeyi kaydet (satır başvurusu için yeterli)
        std::set<int> seen;
        for (auto& m : all) {
            auto coord = IdxToCoord(text, m.start);
            if (seen.count(coord.mLine)) continue;
            seen.insert(coord.mLine);

            std::string ln = (coord.mLine < (int)lines.size())
                             ? lines[coord.mLine] : "";
            // Baştaki boşlukları kırp
            size_t f = ln.find_first_not_of(" \t");
            if (f != std::string::npos) ln = ln.substr(f);

            m_searchResults.push_back({t, tabs[t].name,
                                       coord.mLine, coord.mColumn, ln});
        }
    }
    m_showResults = true;
    snprintf(findState.msg, sizeof(findState.msg),
             "%d sonuc bulundu.", (int)m_searchResults.size());
}

// ─── Sonuçlar paneli ─────────────────────────────────────────────────────────
void App::DrawResultsPanel() {
    // Yeniden boyutlandırma kolu (ItemSpacing=0 → kol ile child arası boşluk yok)
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.25f, 0.25f, 0.30f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.45f, 1.0f));
    ImGui::Button("##resizeR", ImVec2(-1, 4));
    if (ImGui::IsItemActive()) {
        m_resultsHeight -= ImGui::GetIO().MouseDelta.y;
        if (m_resultsHeight < 60.0f)  m_resultsHeight = 60.0f;
        if (m_resultsHeight > 450.0f) m_resultsHeight = 450.0f;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.14f, 1.0f));
    ImGui::BeginChild("##results", ImVec2(0, m_resultsHeight - 4),
                      false, ImGuiWindowFlags_HorizontalScrollbar);

    // Başlık satırı
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
    ImGui::TextColored(ImVec4(0.45f, 0.80f, 1.0f, 1.0f),
                       "  Arama Sonuclari  —  \"%s\"  —  %d eslesme",
                       findState.find, (int)m_searchResults.size());
    ImGui::SameLine();
    if (ImGui::SmallButton("Temizle")) {
        m_searchResults.clear();
        memset(findState.msg, 0, sizeof(findState.msg));
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sonuclari temizle");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 18);
    if (ImGui::SmallButton("X")) {
        m_showResults = false;
        m_searchResults.clear();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Paneli kapat");
    ImGui::Separator();

    if (m_searchResults.empty()) {
        ImGui::TextDisabled("  Sonuc bulunamadi.");
    } else {
        // Sütun başlıkları
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::Text("  %-30s  %6s   %s", "Dosya", "Satir", "Icerik");
        ImGui::PopStyleColor();
        ImGui::Separator();

        for (int i = 0; i < (int)m_searchResults.size(); i++) {
            auto& r = m_searchResults[i];
            char  lbl[1024];
            snprintf(lbl, sizeof(lbl), "  %-30s  %6d   %s##r%d",
                     r.tabName.c_str(), r.line + 1, r.lineText.c_str(), i);

            if (ImGui::Selectable(lbl, false,
                                  ImGuiSelectableFlags_AllowDoubleClick)) {
                // Sekmeye geç
                leftActive = r.tabIdx;
                wantFocusL = true;

                // İmleci satıra ve sütuna taşı, eşleşmeyi seç
                auto& tab  = tabs[r.tabIdx];
                std::string text = tab.editor.GetText();
                auto all = findState.FindAll(text);
                for (auto& m : all) {
                    auto c = IdxToCoord(text, m.start);
                    if (c.mLine == r.line && c.mColumn == r.col) {
                        auto e = IdxToCoord(text, m.start + m.len);
                        tab.editor.SetSelection(c, e);
                        tab.editor.SetCursorPosition(c);
                        break;
                    }
                }
            }
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ─── Sekme yönetimi ──────────────────────────────────────────────────────────
void App::NewTab() {
    tabs.emplace_back();
    leftActive = (int)tabs.size() - 1;
    wantFocusL = true;
    statusMsg  = "Yeni dosya olusturuldu.";
}

void App::OpenFile(const std::string& path) {
    for (int i = 0; i < (int)tabs.size(); i++) {
        if (tabs[i].path == path) {
            leftActive = i;
            wantFocusL = true;
            statusMsg  = "Dosya zaten acik: " + path;
            return;
        }
    }
    tabs.emplace_back();
    tabs.back().Load(path);
    ApplyEditorPalette(tabs.back().editor, tabs.back().langIdx);
    leftActive = (int)tabs.size() - 1;
    wantFocusL = true;
    statusMsg  = "Açıldı: " + path;
}

// ─── Dosya dialogları ─────────────────────────────────────────────────────────
void App::OpenFileWithDialog() {
    auto paths = OpenFileDialog(hwnd);
    for (auto& p : paths)
        OpenFile(p);
}

void App::SaveActive() {
    if (!ActiveTab().path.empty()) {
        std::string err;
        if (ActiveTab().Save(&err)) {
            char buf[512];
            snprintf(buf, sizeof(buf), "Kaydedildi [%s]: %s",
                     ActiveTab().EncodingName(), ActiveTab().path.c_str());
            statusMsg = buf;
            if (!err.empty()) statusMsg += "  |  " + err;  // lossy uyarısı
        } else {
            statusMsg = "HATA: " + (err.empty() ? "Kayit basarisiz." : err);
        }
    } else {
        SaveActiveAs();
    }
}

void App::SaveActiveAs() {
    std::string p = SaveFileDialog(ActiveTab().path, hwnd);
    if (!p.empty()) {
        ActiveTab().path = p;
        ActiveTab().name = EditorTab::Basename(p);
        ActiveTab().langIdx = LangIdxFromPath(p);
        ActiveTab().editor.SetLanguageDefinition(LangByIdx(ActiveTab().langIdx));
        ApplyEditorPalette(ActiveTab().editor, ActiveTab().langIdx);
        std::string err;
        if (ActiveTab().Save(&err)) {
            char buf[512];
            snprintf(buf, sizeof(buf), "Kaydedildi [%s]: %s",
                     ActiveTab().EncodingName(), p.c_str());
            statusMsg = buf;
            if (!err.empty()) statusMsg += "  |  " + err;
        } else {
            statusMsg = "HATA: " + (err.empty() ? "Kayit basarisiz." : err);
        }
    }
}






