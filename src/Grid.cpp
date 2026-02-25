//
// Grid.cpp
// WaterColorSimulation
//
// 격자 버퍼 할당, 초기화, 높이맵 생성
//
#include "Grid.h"

Grid::Grid(int w, int h) : width(w), height(h) {}

void Grid::init() {
    const int total    = width * height;
    const int totalRGB = 3 * total;

    // 모든 시뮬레이션 버퍼를 0으로 초기화
    renderBuffer    .assign(totalRGB, 0.0f);
    paper           .assign(totalRGB, 0.0f);
    heightMap       .assign(total, 0.0f);
    capacity        .assign(total, 0.0f);

    water           .assign(total, 0.0f);
    waterTemp       .assign(total, 0.0f);
    velocity        .assign(total, glm::vec2(0.0f));
    velocityTemp    .assign(total, glm::vec2(0.0f));

    wetAreaMask     .assign(total, 0.0f);
    wetAreaMaskTemp .assign(total, 0.0f);
    evaporation     .assign(total, 0.0f);

    saturation      .assign(total, 0.0f);
    saturationTemp  .assign(total, 0.0f);

    pigment         .assign(total, 0.0f);
    pigmentTemp     .assign(total, 0.0f);
    pigmentDeposit  .assign(total, 0.0f);

    surfaceColor    .assign(total, glm::vec3(0.0f));
    surfaceColorTemp.assign(total, glm::vec3(0.0f));
    depositColor    .assign(total, glm::vec3(0.0f));

    pixelData       .resize(total);

    generateHeightMap();

    // 렌더 버퍼를 흰색으로, 종이색/용량을 높이맵에서 유도
    for (int i = 0; i < total; ++i) {
        // 높이 봉우리일수록 약간 어둡게 (종이 질감 표현)
        float shade = 1.0f - heightMap[i] * 0.05f;
        paper[3 * i + 0] = shade;
        paper[3 * i + 1] = shade;
        paper[3 * i + 2] = shade;

        renderBuffer[3 * i + 0] = 1.0f;
        renderBuffer[3 * i + 1] = 1.0f;
        renderBuffer[3 * i + 2] = 1.0f;

        wetAreaMask[i] = 0.0f;

        // 깊은 섬유(높이 낮음)일수록 물을 더 많이 흡수
        capacity[i] = heightMap[i] * (0.7f - 0.2f) + 0.2f;
    }
}

void Grid::generateHeightMap() {
    PerlinNoise noise;

    int idx = 0;
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            double nx = static_cast<double>(col) / static_cast<double>(width);
            double ny = static_cast<double>(row) / static_cast<double>(height);
            // 고주파(200배) 노이즈로 종이 결 표현
            heightMap[idx++] = static_cast<float>(noise.noise(200.0 * nx, 200.0 * ny, 0.0));
        }
    }
}
