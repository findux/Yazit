#include "LuaEngine.h"
#include "App.h"        // App tam tanımı burada, LuaEngine.h'ta sadece ileri bildirim
#include "FindState.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

// ─── Kayıt yardımcıları ──────────────────────────────────────────────────────
static App* GetApp(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "_app");
    auto* p = static_cast<App*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return p;
}
static LuaEngine* GetEngine(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "_engine");
    auto* p = static_cast<LuaEngine*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return p;
}

// ─── Özel print — çıktıyı LuaEngine arabelleğine yazar ───────────────────���──
static int lf_print_capture(lua_State* L) {
    LuaEngine* eng = GetEngine(L);
    int n = lua_gettop(L);
    std::string line;
    for (int i = 1; i <= n; i++) {
        if (i > 1) line += "\t";
        lua_getglobal(L, "tostring");
        lua_pushvalue(L, i);
        lua_call(L, 1, 1);
        line += lua_tostring(L, -1);
        lua_pop(L, 1);
    }
    eng->AppendOutput(line + "\n");
    return 0;
}

// ─── editor.get_text([dosyaAdi]) ─────────────────────────────────────────────
static int lf_get_text(lua_State* L) {
    App* app = GetApp(L);
    if (lua_gettop(L) >= 1 && lua_isstring(L, 1)) {
        const char* name = lua_tostring(L, 1);
        for (auto& t : app->tabs)
            if (t.name == name || t.path == name) {
                lua_pushstring(L, t.editor.GetText().c_str());
                return 1;
            }
        lua_pushnil(L);
        return 1;
    }
    lua_pushstring(L, app->ActiveTab().editor.GetText().c_str());
    return 1;
}

// ─── editor.set_text(metin [, dosyaAdi]) ─────────────────────────────────────
static int lf_set_text(lua_State* L) {
    App* app = GetApp(L);
    const char* text = luaL_checkstring(L, 1);
    if (lua_gettop(L) >= 2 && lua_isstring(L, 2)) {
        const char* name = lua_tostring(L, 2);
        for (auto& t : app->tabs)
            if (t.name == name || t.path == name) {
                t.editor.SetText(text); t.modified = true; return 0;
            }
    } else {
        app->ActiveTab().editor.SetText(text);
        app->ActiveTab().modified = true;
    }
    return 0;
}

// ─── editor.get_name() ───────────────────────────────────────────────────────
static int lf_get_name(lua_State* L) {
    lua_pushstring(L, GetApp(L)->ActiveTab().name.c_str());
    return 1;
}

// ─── editor.get_path() ───────────────────────────────────────────────────────
static int lf_get_path(lua_State* L) {
    lua_pushstring(L, GetApp(L)->ActiveTab().path.c_str());
    return 1;
}

// ─── editor.get_all_names() → tablo ─────────────────────────────────────────
static int lf_get_all_names(lua_State* L) {
    App* app = GetApp(L);
    lua_newtable(L);
    for (int i = 0; i < (int)app->tabs.size(); i++) {
        lua_pushstring(L, app->tabs[i].name.c_str());
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

// ─── editor.get_all_paths() → tablo ─────────────────────────────────────────
static int lf_get_all_paths(lua_State* L) {
    App* app = GetApp(L);
    lua_newtable(L);
    for (int i = 0; i < (int)app->tabs.size(); i++) {
        lua_pushstring(L, app->tabs[i].path.c_str());
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

// ─── editor.insert(metin) ────────────────────────────────────────────────────
static int lf_insert(lua_State* L) {
    App* app = GetApp(L);
    const char* text = luaL_checkstring(L, 1);
    app->ActiveTab().editor.InsertTextAtCursor(text);
    app->ActiveTab().modified = true;
    return 0;
}

// ─── editor.get_cursor() → satir, sutun (1 tabanlı) ─────────────────────────
static int lf_get_cursor(lua_State* L) {
    auto c = GetApp(L)->ActiveTab().editor.GetCursorPosition();
    lua_pushinteger(L, c.mLine + 1);
    lua_pushinteger(L, c.mColumn + 1);
    return 2;
}

// ─── editor.set_cursor(satir, sutun) — 1 tabanlı ────────────────────────────
static int lf_set_cursor(lua_State* L) {
    App* app = GetApp(L);
    int line = (int)luaL_checkinteger(L, 1) - 1;
    int col  = (int)luaL_checkinteger(L, 2) - 1;
    app->ActiveTab().editor.SetCursorPosition({std::max(0, line), std::max(0, col)});
    return 0;
}

// ─── editor.get_selection() → metin ─────────────────────────────────────────
static int lf_get_selection(lua_State* L) {
    lua_pushstring(L, GetApp(L)->ActiveTab().editor.GetSelectedText().c_str());
    return 1;
}

// ─── editor.select_all() ───────────────────────────────────────────���─────────
static int lf_select_all(lua_State* L) {
    GetApp(L)->ActiveTab().editor.SelectAll();
    return 0;
}

// ─── editor.replace_all(bul, degistir) → sayi ───────────────────────────────
static int lf_replace_all(lua_State* L) {
    App* app = GetApp(L);
    const char* find = luaL_checkstring(L, 1);
    const char* repl = luaL_checkstring(L, 2);
    FindState fs;
    strncpy_s(fs.find,    find, sizeof(fs.find)    - 1);
    strncpy_s(fs.replace, repl, sizeof(fs.replace) - 1);
    lua_pushinteger(L, fs.ReplaceAll(app->ActiveTab()));
    return 1;
}

// ─── editor.get_line_count() → sayi ────────────────────────────────��────────
static int lf_get_line_count(lua_State* L) {
    auto txt = GetApp(L)->ActiveTab().editor.GetText();
    int cnt = 1;
    for (char c : txt) if (c == '\n') cnt++;
    lua_pushinteger(L, cnt);
    return 1;
}

// ─── editor.new_tab() ────────────────────────────────────────────────────────
static int lf_new_tab(lua_State* L) {
    GetApp(L)->NewTab();
    return 0;
}

// ─── editor.open_file(yol) ───────────────────────────────────────────────────
static int lf_open_file(lua_State* L) {
    GetApp(L)->OpenFile(luaL_checkstring(L, 1));
    return 0;
}

// ─── editor.save([dosyaAdi]) ────────────────────────────────���────────────────
static int lf_save(lua_State* L) {
    App* app = GetApp(L);
    if (lua_gettop(L) >= 1 && lua_isstring(L, 1)) {
        const char* name = lua_tostring(L, 1);
        for (auto& t : app->tabs)
            if (t.name == name || t.path == name) { t.Save(); return 0; }
    }
    app->ActiveTab().Save();
    return 0;
}

// ─── editor.is_modified([dosyaAdi]) → bool ──────────────────────────────────��
static int lf_is_modified(lua_State* L) {
    App* app = GetApp(L);
    if (lua_gettop(L) >= 1 && lua_isstring(L, 1)) {
        const char* name = lua_tostring(L, 1);
        for (auto& t : app->tabs)
            if (t.name == name || t.path == name) {
                lua_pushboolean(L, t.modified);
                return 1;
            }
    }
    lua_pushboolean(L, app->ActiveTab().modified);
    return 1;
}

// ─── LuaEngine ───────────────────────────────────────────────────────────────

LuaEngine::~LuaEngine() {
    if (m_L) lua_close(m_L);
}

void LuaEngine::Init(App* app) {
    m_app = app;
    m_L   = luaL_newstate();
    luaL_openlibs(m_L);

    // App ve engine işaretçilerini Lua kaydına koy
    lua_pushlightuserdata(m_L, app);
    lua_setfield(m_L, LUA_REGISTRYINDEX, "_app");
    lua_pushlightuserdata(m_L, this);
    lua_setfield(m_L, LUA_REGISTRYINDEX, "_engine");

    RegisterBindings();
}

void LuaEngine::RegisterBindings() {
    // print → çıktı arabelleğine yönlendir
    lua_pushcfunction(m_L, lf_print_capture);
    lua_setglobal(m_L, "print");

    // editor tablosu
    static const luaL_Reg kEditorFuncs[] = {
        { "get_text",       lf_get_text       },
        { "set_text",       lf_set_text       },
        { "get_name",       lf_get_name       },
        { "get_path",       lf_get_path       },
        { "get_all_names",  lf_get_all_names  },
        { "get_all_paths",  lf_get_all_paths  },
        { "insert",         lf_insert         },
        { "get_cursor",     lf_get_cursor     },
        { "set_cursor",     lf_set_cursor     },
        { "get_selection",  lf_get_selection  },
        { "select_all",     lf_select_all     },
        { "replace_all",    lf_replace_all    },
        { "get_line_count", lf_get_line_count },
        { "new_tab",        lf_new_tab        },
        { "open_file",      lf_open_file      },
        { "save",           lf_save           },
        { "is_modified",    lf_is_modified    },
        { nullptr, nullptr }
    };

    lua_newtable(m_L);
    luaL_setfuncs(m_L, kEditorFuncs, 0);
    lua_setglobal(m_L, "editor");
}

std::string LuaEngine::Execute(const std::string& code) {
    m_output.clear();
    if (luaL_dostring(m_L, code.c_str()) != LUA_OK) {
        m_output += "HATA: ";
        m_output += lua_tostring(m_L, -1);
        m_output += "\n";
        lua_pop(m_L, 1);
    }
    return m_output;
}
