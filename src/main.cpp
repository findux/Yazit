#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <GL/glew.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include "imgui_impl_sdl2.h"
#include "App.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <string>

// ── Yardımcı: wide → UTF-8 ───────────────────────────────────────────────────
static std::string WideToUtf8(const wchar_t* w) {
    if (!w || !w[0]) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    std::string s(n - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w, -1, &s[0], n, nullptr, nullptr);
    return s;
}

// ── Win32 sürükle-bırak: WndProc alt sınıfı ─────────────────────────────────
// SDL'in WM_DROPFILES → SDL_DROPFILE dönüşümü UIPI tarafından engellendiğinde
// mesajı doğrudan Win32 seviyesinde yakalayıp App::OpenFile'a iletiyoruz.

static App*    g_DropApp     = nullptr;
static WNDPROC g_OrigWndProc = nullptr;

static LRESULT CALLBACK DropWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DROPFILES)
    {
        HDROP hDrop = reinterpret_cast<HDROP>(wParam);
        UINT  count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
        for (UINT i = 0; i < count; i++)
        {
            UINT len = DragQueryFileW(hDrop, i, nullptr, 0);
            std::wstring wp(len + 1, L'\0');
            DragQueryFileW(hDrop, i, &wp[0], len + 1);
            wp.resize(len);
            if (g_DropApp)
                g_DropApp->OpenFile(WideToUtf8(wp.c_str()));
        }
        DragFinish(hDrop);
        return 0;
    }
    return CallWindowProcW(g_OrigWndProc, hwnd, msg, wParam, lParam);
}

// ── Giriş noktası ─────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* win = SDL_CreateWindow("YAZIT",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1400, 900,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_GLContext glctx = SDL_GL_CreateContext(win);
    SDL_GL_MakeCurrent(win, glctx);
    SDL_GL_SetSwapInterval(1);

    glewExperimental = GL_TRUE;
    glewInit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // imgui.ini dosyasını %APPDATA%\Yazit\ altına yönlendir
    static std::string iniPath;
    {
        char appdata[MAX_PATH];
        if (GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH)) {
            std::string dir = std::string(appdata) + "\\YAZIT";
            CreateDirectoryA(dir.c_str(), nullptr);
            iniPath = dir + "\\imgui.ini";
        }
    }
    if (!iniPath.empty())
        io.IniFilename = iniPath.c_str();

    // ── Türkçe karakter destekli font ────────────────────────────────────────
    static const ImWchar kTurkishRanges[] = {
        0x0020, 0x00FF,
        0x0100, 0x017F,
        0,
    };
    const char* fontCandidates[] = {
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        nullptr
    };
    bool fontLoaded = false;
    for (int i = 0; fontCandidates[i] && !fontLoaded; i++)
        fontLoaded = (io.Fonts->AddFontFromFileTTF(
            fontCandidates[i], 15.0f, nullptr, kTurkishRanges) != nullptr);
    if (!fontLoaded)
        io.Fonts->AddFontDefault();

    ImGui_ImplSDL2_InitForOpenGL(win, glctx);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    App app;
    app.Init(win);

    // ── Win32 sürükle-bırak kurulumu ─────────────────────────────────────────
    // SDL_DROPFILE'a güvenmek yerine WM_DROPFILES'ı doğrudan yakalıyoruz.
    {
        SDL_SysWMinfo wm;
        SDL_VERSION(&wm.version);
        if (SDL_GetWindowWMInfo(win, &wm))
        {
            HWND hwnd = wm.info.win.window;

            // UIPI: Explorer (orta bütünlük) → uygulama (yüksek bütünlük) arasında
            // WM_DROPFILES'ın geçmesine izin ver
            ChangeWindowMessageFilterEx(hwnd, WM_DROPFILES, MSGFLT_ALLOW, nullptr);
            ChangeWindowMessageFilterEx(hwnd, WM_COPYDATA,  MSGFLT_ALLOW, nullptr);
            ChangeWindowMessageFilterEx(hwnd, 0x0049,       MSGFLT_ALLOW, nullptr); // WM_COPYGLOBALDATA

            // Shell'e bu pencereye dosya bırakılabileceğini bildir
            DragAcceptFiles(hwnd, TRUE);

            // WndProc alt sınıfla: WM_DROPFILES'ı biz hallederiz,
            // diğer her şeyi SDL'e iletiyoruz
            g_DropApp     = &app;
            g_OrigWndProc = reinterpret_cast<WNDPROC>(
                                SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                    reinterpret_cast<LONG_PTR>(DropWndProc)));
        }
    }

    // ── Komut satırından gelen dosyaları aç (UTF-16) ─────────────────────────
    {
        int wargc = 0;
        wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
        if (wargv && wargc > 1) {
            app.tabs.clear();
            for (int i = 1; i < wargc; i++)
                app.OpenFile(WideToUtf8(wargv[i]));
        }
        if (wargv) LocalFree(wargv);
    }

    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT)
                app.RequestExit(running);
            // Not: SDL_DROPFILE artık WndProc tarafından ele alındığı için
            // buraya ulaşmaz; ama yedek olarak bırakıyoruz.
            else if (ev.type == SDL_DROPFILE && ev.drop.file) {
                app.OpenFile(ev.drop.file);
                SDL_free(ev.drop.file);
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        app.Draw(running);

        ImGui::Render();
        int w, h;
        SDL_GetWindowSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(win);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(glctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
