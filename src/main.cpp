#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include "imgui_impl_sdl2.h"
#include "App.h"

int main(int, char**) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* win = SDL_CreateWindow("Yazit",
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

    // Tema App::Init() içinde SetTheme(Dark) ile uygulanır

    // ── Turkce karakter destekli font ────────────────────────────────────────
    // Latin-Extended-A blogu: Turkce ozgul harfler (g-breve, s-cedilla, vb.)
    static const ImWchar kTurkishRanges[] = {
        0x0020, 0x00FF,   // Temel Latin + Latin Ek (c-cedilla, u-umlaut, o-umlaut)
        0x0100, 0x017F,   // Latin Genisletilmis-A (g-breve, s-cedilla, i-dotless, I-dot)
        0,
    };
    // Konsolas yoksa Segoe UI, o da yoksa Arial, hepsi olmadıginda varsayilan
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

    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT) running = false;
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
