#pragma once
#include <string>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

class App; // ileri bildirim — App.h döngüsel include'u önler

class LuaEngine {
public:
    LuaEngine()  = default;
    ~LuaEngine();
    LuaEngine(const LuaEngine&)            = delete;
    LuaEngine& operator=(const LuaEngine&) = delete;

    void Init(App* app);

    // Kodu çalıştır, print() çıktılarını + hataları döndür
    std::string Execute(const std::string& code);

    void        AppendOutput(const std::string& s) { m_output += s; }
    void        ClearOutput()                       { m_output.clear(); }
    const std::string& GetOutput() const            { return m_output; }

    lua_State* GetState() const { return m_L; }

private:
    lua_State*  m_L      = nullptr;
    App*        m_app    = nullptr;
    std::string m_output;

    void RegisterBindings();
};
