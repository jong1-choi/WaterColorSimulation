//
// main.cpp
// WaterColorSimulation
//
// 진입점: GLFW/GLEW 윈도우 생성, Dear ImGui 패널 초기화, 메인 루프 실행
//
// 조작법:
//   마우스 드래그   - 캔버스에 그리기
//   Space          - 시뮬레이션 켜기/끄기
//   0              - 캔버스 초기화
//   1-8            - 표시 모드 전환
//   9              - 안료를 French Ultramarine으로 변경
//   위/아래 화살표  - 브러시 반경 조절
//
#include <iostream>
#include <string>

// GLEW는 다른 OpenGL 헤더보다 먼저 포함해야 함
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Dear ImGui (GLFW + OpenGL3 백엔드)
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/imgui_impl_glfw.h"
#include "../third_party/imgui/imgui_impl_opengl3.h"

#include "Grid.h"
#include "Simulation.h"
#include "Renderer.h"
#include "KubelkaMunk.h"

// --- 상수 --------------------------------------------------------------------
static constexpr int  GRID_W      = 256;   // 시뮬레이션 격자 너비
static constexpr int  GRID_H      = 256;   // 시뮬레이션 격자 높이
static constexpr int  CANVAS_SIZE = 1024;  // 캔버스 뷰포트 크기
static constexpr int  PANEL_W     = 260;   // ImGui 패널 너비
static constexpr int  WINDOW_W    = CANVAS_SIZE + PANEL_W;
static constexpr int  WINDOW_H    = CANVAS_SIZE;

// --- 애플리케이션 상태 (GLFW 콜백에서 사용) ----------------------------------
struct AppState {
    Grid*        grid        = nullptr;
    Simulation*  sim         = nullptr;
    PigmentInfo  pigment;
    DisplayMode  displayMode  = DisplayMode::Composite;
    bool         isSimulating = false;
    bool         isMouseDown  = false;
    float        mouseNormX   = 0.0f;
    float        mouseNormY   = 0.0f;
};

static AppState g_app;

// --- GLFW 콜백 ---------------------------------------------------------------

static void onMouseButton(GLFWwindow* /*win*/, int button, int action, int /*mods*/) {
    if (ImGui::GetIO().WantCaptureMouse) return;  // ImGui가 먼저 처리
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        g_app.isMouseDown = (action == GLFW_PRESS);
}

static void onCursorPos(GLFWwindow* /*win*/, double x, double y) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    // 캔버스 영역 기준 [0,1]로 정규화
    g_app.mouseNormX = static_cast<float>(x) / static_cast<float>(CANVAS_SIZE);
    g_app.mouseNormY = static_cast<float>(y) / static_cast<float>(CANVAS_SIZE);
}

static void onKey(GLFWwindow* /*win*/, int key, int /*scancode*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS) return;

    switch (key) {
    case GLFW_KEY_0:     g_app.grid->init(); break;
    case GLFW_KEY_SPACE:
        g_app.isSimulating = !g_app.isSimulating;
        std::cout << "Simulation: " << (g_app.isSimulating ? "ON" : "OFF") << "\n";
        break;
    // 1-8: 표시 모드 전환
    case GLFW_KEY_1: g_app.displayMode = DisplayMode::Composite;      break;
    case GLFW_KEY_2: g_app.displayMode = DisplayMode::Water;          break;
    case GLFW_KEY_3: g_app.displayMode = DisplayMode::Saturation;     break;
    case GLFW_KEY_4: g_app.displayMode = DisplayMode::VelocityX;      break;
    case GLFW_KEY_5: g_app.displayMode = DisplayMode::WetMask;        break;
    case GLFW_KEY_6: g_app.displayMode = DisplayMode::Evaporation;    break;
    case GLFW_KEY_7: g_app.displayMode = DisplayMode::Deposit;        break;
    case GLFW_KEY_8: g_app.displayMode = DisplayMode::SurfacePigment; break;
    case GLFW_KEY_9: g_app.pigment.setFrenchUltramarine();            break;
    case GLFW_KEY_UP:   g_app.sim->params.brushRadius++;              break;
    case GLFW_KEY_DOWN:
        if (g_app.sim->params.brushRadius > 2)
            --g_app.sim->params.brushRadius;
        break;
    default: break;
    }
}

// --- ImGui 파라미터 패널 -----------------------------------------------------

static void renderControlPanel(SimulationParams& p) {
    ImGui::SetNextWindowPos (ImVec2(static_cast<float>(CANVAS_SIZE), 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(PANEL_W), static_cast<float>(WINDOW_H)),
                              ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove
                           | ImGuiWindowFlags_NoResize
                           | ImGuiWindowFlags_NoTitleBar
                           | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Controls", nullptr, flags);

    ImGui::Text("WaterColor Simulation");
    ImGui::Separator();

    if (ImGui::Button(g_app.isSimulating ? "Stop  [Space]" : "Start [Space]", ImVec2(-1, 0)))
        g_app.isSimulating = !g_app.isSimulating;
    if (ImGui::Button("Reset [0]", ImVec2(-1, 0)))
        g_app.grid->init();

    ImGui::Separator();
    ImGui::SliderInt("Speed",        &p.speedMultiplier, 1, 20);
    ImGui::SliderInt("Brush Radius", &p.brushRadius,     2, 40);

    ImGui::Separator();
    ImGui::Text("Fluid Parameters");
    ImGui::SliderFloat("KappaV",   &p.velocityViscosity,  0.0f, 1.0f);
    ImGui::SliderFloat("KappaW",   &p.waterViscosity,     0.0f, 1.0f);
    ImGui::SliderFloat("KappaP",   &p.pigmentViscosity,   0.0f, 5.0f);

    ImGui::Separator();
    ImGui::Text("Capillary Layer");
    ImGui::SliderFloat("Alpha",    &p.absorption,         0.0f, 0.5f);
    ImGui::SliderFloat("Epsilon",  &p.capillaryThreshold, 0.0f, 1.0f);
    ImGui::SliderFloat("Sigma",    &p.wetMaskThreshold,   0.0f, 1.0f);

    ImGui::Separator();
    ImGui::Text("Pigment");
    ImGui::SliderFloat("Granule",  &p.granulation,   0.0f,  1.0f);
    ImGui::SliderFloat("Density",  &p.density,       0.0f,  1.0f);
    ImGui::SliderFloat("Staining", &p.staining,      0.0f, 10.0f);
    ImGui::SliderFloat("Water",    &p.waterAmount,   0.0f, 10.0f);
    ImGui::SliderFloat("Pigment",  &p.pigmentAmount, 0.0f,  1.0f);

    ImGui::Separator();
    ImGui::Text("Display Mode [1-8]");
    const char* modeNames[] = {
        "1: Composite", "2: Water", "3: Saturation", "4: Velocity X",
        "5: Wet Mask",  "6: Evaporation", "7: Deposit", "8: Surface Pigment"
    };
    int modeIdx = static_cast<int>(g_app.displayMode);
    if (ImGui::Combo("##Mode", &modeIdx, modeNames, 8))
        g_app.displayMode = static_cast<DisplayMode>(modeIdx);

    ImGui::Separator();
    ImGui::Text("Pigment [9=Ultramarine]");
    if (ImGui::Button("Quinacridone Magenta", ImVec2(-1, 0))) g_app.pigment.setQuinacridoneMagenta();
    if (ImGui::Button("Indian Red",           ImVec2(-1, 0))) g_app.pigment.setIndianRed();
    if (ImGui::Button("Cadmium Yellow",       ImVec2(-1, 0))) g_app.pigment.setCadmiumYellow();
    if (ImGui::Button("Hooker's Green",       ImVec2(-1, 0))) g_app.pigment.setHookersGreen();
    if (ImGui::Button("Cerulean Blue",        ImVec2(-1, 0))) g_app.pigment.setCeruleanBlue();
    if (ImGui::Button("Burnt Umber",          ImVec2(-1, 0))) g_app.pigment.setBurntUmber();
    if (ImGui::Button("Cadmium Red",          ImVec2(-1, 0))) g_app.pigment.setCadmiumRed();
    if (ImGui::Button("Interference Lilac",   ImVec2(-1, 0))) g_app.pigment.setInterferenceLilac();
    if (ImGui::Button("French Ultramarine",   ImVec2(-1, 0))) g_app.pigment.setFrenchUltramarine();

    ImGui::End();
}

// --- 진입점 ------------------------------------------------------------------

int main() {
    // GLFW 초기화
    if (!glfwInit()) {
        std::cerr << "[FATAL] glfwInit failed\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_W, WINDOW_H,
                                           "WaterColor Simulation", nullptr, nullptr);
    if (!window) {
        std::cerr << "[FATAL] Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // V-Sync

    // GLEW 초기화
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "[FATAL] glewInit failed\n";
        glfwTerminate();
        return 1;
    }

    // 앱 콜백을 먼저 등록한 뒤 ImGui 초기화
    // (ImGui_ImplGlfw_InitForOpenGL이 기존 콜백을 저장하고 체이닝하므로 순서 중요)
    glfwSetMouseButtonCallback(window, onMouseButton);
    glfwSetCursorPosCallback  (window, onCursorPos);
    glfwSetKeyCallback        (window, onKey);

    // Dear ImGui 초기화
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");

    // 시뮬레이션 초기화
    Grid         grid(GRID_W, GRID_H);
    SimulationParams params;
    Simulation   sim(grid, params);
    Renderer     renderer;

    g_app.grid = &grid;
    g_app.sim  = &sim;
    g_app.pigment.setQuinacridoneMagenta();  // 기본 안료

    grid.init();
    renderer.init("res/shader.vert", "res/shader.frag");

    float lastTime = static_cast<float>(glfwGetTime());

    // 메인 루프
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        float currentTime = static_cast<float>(glfwGetTime());
        float dt          = currentTime - lastTime;
        lastTime          = currentTime;

        // 브러시 적용 (현재 선택된 안료 색상 전달)
        sim.applyBrush(g_app.mouseNormX, g_app.mouseNormY, g_app.isMouseDown,
                       g_app.pigment.colorW);

        // 시뮬레이션 진행
        if (g_app.isSimulating) {
            for (int i = 0; i < sim.params.speedMultiplier; ++i)
                sim.step(dt);
        }

        // 렌더 버퍼 갱신 후 캔버스 영역에 렌더링
        sim.updateRenderBuffer(g_app.displayMode);
        glViewport(0, 0, CANVAS_SIZE, CANVAS_SIZE);
        renderer.render(grid.renderBuffer.data(), GRID_W, GRID_H);

        // ImGui 프레임
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        renderControlPanel(sim.params);
        ImGui::Render();
        glViewport(0, 0, WINDOW_W, WINDOW_H);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // 정리
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
