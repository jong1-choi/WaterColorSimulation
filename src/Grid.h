//
// Grid.h
// WaterColorSimulation
//
// 수채화 시뮬레이션의 모든 레이어를 보관하는 중심 데이터 구조
// 버퍼는 행 우선(row-major) 1차원 배열: index = y * width + x
//
#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "KubelkaMunk.h"
#include "PerlinNoise.h"

class Grid {
public:
    const int width;   // 격자 열 수
    const int height;  // 격자 행 수

    // --- 출력 ---
    std::vector<float> renderBuffer;   // RGB 합성 결과 (3 * width * height)

    // --- 종이 ---
    std::vector<float> paper;      // 종이 기본색 RGB (3 * width * height)
    std::vector<float> heightMap;  // 펄린 노이즈 높이맵 [0,1]
    std::vector<float> capacity;   // 모세관 흡수 용량 (높이에서 파생)

    // --- 수면층 ---
    std::vector<float>      water;        // 셀당 물 양
    std::vector<float>      waterTemp;    // 이류 임시 버퍼
    std::vector<glm::vec2>  velocity;     // 유체 속도 (u, v)
    std::vector<glm::vec2>  velocityTemp; // 이류 임시 버퍼

    // --- 젖은 영역 마스크 ---
    std::vector<float> wetAreaMask;     // 1 = 젖음, 0 = 건조
    std::vector<float> wetAreaMaskTemp; // 임시 버퍼
    std::vector<float> evaporation;     // 블러된 젖은 마스크 (경계 지시자)

    // --- 모세관층 ---
    std::vector<float> saturation;     // 종이 섬유 흡수 포화도
    std::vector<float> saturationTemp; // 확산 임시 버퍼

    // --- 안료 ---
    std::vector<float> pigment;        // 수면층 안료 농도
    std::vector<float> pigmentTemp;    // 이류 임시 버퍼
    std::vector<float> pigmentDeposit; // 종이 표면에 침착된 안료 농도

    // 셀별 안료 색상 (사전 곱셈 저장: 실제색 × 농도)
    // 농도 스칼라와 분리 저장하여 여러 색상이 공존 가능
    std::vector<glm::vec3> surfaceColor;      // 수면층 안료 색상
    std::vector<glm::vec3> surfaceColorTemp;  // 이류 임시 버퍼
    std::vector<glm::vec3> depositColor;      // 침착 안료 색상

    // --- KM 픽셀 데이터 ---
    std::vector<PixelInfo> pixelData;

    Grid(int w, int h);

    // 모든 버퍼 할당 및 초기화. 여러 번 호출 가능 (리셋).
    void init();

    // (x, y)에 대한 클램프된 1차원 인덱스 반환
    inline int index(int x, int y) const {
        int cx = (x < 0) ? 0 : (x >= width  ? width  - 1 : x);
        int cy = (y < 0) ? 0 : (y >= height ? height - 1 : y);
        return cy * width + cx;
    }

private:
    // 높이맵을 펄린 노이즈로 채우고 종이색/용량을 유도
    void generateHeightMap();
};
