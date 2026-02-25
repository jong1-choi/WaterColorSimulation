//
// Simulation.h
// WaterColorSimulation
//
// 수채화 유체 시뮬레이션 (Van Laerhoven, 2004 기반)
// 매 스텝: UpdateVelocity → UpdateWater → UpdatePigment → SurfaceLayer → CapillaryLayer
//
#pragma once

#include <glm/glm.hpp>
#include "Grid.h"
#include "KubelkaMunk.h"

// 디버그용 렌더 채널 선택
enum class DisplayMode : int {
    Composite      = 0,  // 최종 합성 (침착 + 수면 안료)
    Water          = 1,  // 수면 물 양
    Saturation     = 2,  // 모세관 포화도
    VelocityX      = 3,  // 수평 속도
    WetMask        = 4,  // 젖은 영역 마스크
    Evaporation    = 5,  // 가우시안 블러된 경계 지시자
    Deposit        = 6,  // 침착된 안료
    SurfacePigment = 7,  // 수면 안료
};

// UI에서 조절 가능한 물리 파라미터 (Van Laerhoven 2004 기준값)
struct SimulationParams {
    float velocityViscosity  = 0.10f;  // κv – 속도장 점성
    float waterViscosity     = 0.10f;  // κw – 물 이동 점성
    float pigmentViscosity   = 0.20f;  // κp – 안료 이동 점성
    float absorption         = 0.03f;  // α  – 모세관 흡수율
    float capillaryThreshold = 0.30f;  // ε  – 모세관 확산 최소 포화도
    float wetMaskThreshold   = 0.06f;  // σ  – 건조 판정 포화도 임계값
    float granulation        = 0.20f;  // 안료 입자감 (종이 골에 침착)
    float density            = 0.30f;  // ρ  – 안료 밀도
    float staining           = 2.00f;  // 착색력 (탈착 저항)
    float waterAmount        = 2.00f;  // 브러시 1회 적용 물 양
    float pigmentAmount      = 0.20f;  // 브러시 1회 적용 안료 양
    int   brushRadius        = 10;     // 브러시 반경 (격자 셀 단위)
    int   speedMultiplier    = 1;      // 프레임당 시뮬레이션 스텝 수
};

// 수채화 시뮬레이션 전체 로직. Grid를 공유하며 step()을 매 프레임 호출.
class Simulation {
public:
    Simulation(Grid& grid, const SimulationParams& params);

    // 정규화 좌표 [0,1]에 브러시 적용. pigmentColor는 현재 선택된 안료 색상.
    void applyBrush(float normX, float normY, bool isPressed,
                    const glm::vec3& pigmentColor);

    // dt초만큼 시뮬레이션 진행 (프레임당 speedMultiplier회 호출됨)
    void step(float dt);

    // 현재 displayMode에 맞게 grid.renderBuffer를 갱신
    void updateRenderBuffer(DisplayMode mode);

    // UI 슬라이더가 직접 쓰는 파라미터
    SimulationParams params;

private:
    Grid& m_grid;

    // --- 유체 솔버 ---

    // 세미-라그랑지안 이류: 각 셀을 dt*vel만큼 역추적해 쌍선형 샘플링
    template<typename T>
    void advect(T* field, T* tempBuffer, const glm::vec2* vel, float dt);

    // 가우스-자이델 확산: D += k*dt * Laplacian(D), wetAreaMask 내부만 적용
    template<typename T>
    void diffuse(float k, T* field, const float* mask, float dt);

    // 보존적 물 이류: 최대/최소 수량 경계를 준수
    template<typename T>
    void waterAdvect(T* field, T* tempBuffer, const glm::vec2* vel,
                     const float* mask, float dt);

    // --- 시뮬레이션 서브스텝 ---

    void addHeightDifferenceVelocity(); // 수위 기울기 → 속도 추가
    void applyBoundaryConditions();     // 건조 셀 속도 = 0 (no-slip)
    void flowOutward();                 // 경계 증발 및 건조 처리
    void updateSurfaceLayer(float dt);  // 안료 흡착/탈착 (수면 ↔ 종이)
    void updateCapillaryLayer();        // 모세관 포화도 확산 및 흡수
    void updateVelocity(float dt);
    void updateWater(float dt);
    void updatePigment(float dt);

    // --- 헬퍼 ---

    // 연속 좌표 p에서 필드를 쌍선형 보간
    template<typename T>
    T sampleBilinear(const T* field, const glm::vec2& p) const;

    const float k_maxWater = 10.0f;
    const float k_minWater =  0.1f;
};
