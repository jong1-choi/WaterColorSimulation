//
// Simulation.cpp
// WaterColorSimulation
//
// 수채화 유체 시뮬레이션 구현 (Van Laerhoven 2004)
//
#include "Simulation.h"
#include "GaussianBlur.h"

#include <algorithm>
#include <cmath>
#include <iostream>

Simulation::Simulation(Grid& grid, const SimulationParams& params)
    : m_grid(grid), params(params) {}

// --- 공개 인터페이스 ----------------------------------------------------------

void Simulation::applyBrush(float normX, float normY, bool isPressed,
                             const glm::vec3& pigmentColor) {
    if (!isPressed) return;

    const int cx = static_cast<int>(normX * m_grid.width);
    const int cy = static_cast<int>(normY * m_grid.height);
    const int r  = params.brushRadius;

    for (int y = 1; y < m_grid.height - 1; ++y) {
        for (int x = 1; x < m_grid.width - 1; ++x) {
            const float dist = std::sqrt(static_cast<float>((cx - x) * (cx - x)
                                                           + (cy - y) * (cy - y)));
            if (dist < r) {
                int idx = m_grid.index(x, y);
                m_grid.water[idx]        = params.waterAmount;
                m_grid.saturation[idx]   = params.wetMaskThreshold;
                m_grid.wetAreaMask[idx]  = 1.0f;
                m_grid.pigment[idx]      = params.pigmentAmount;
                // 사전 곱셈(premultiplied) 저장: 색상 × 농도
                // 빈 셀(색=0)과 이류 보간 시 색이 희석되지 않도록 하기 위함
                m_grid.surfaceColor[idx] = pigmentColor * params.pigmentAmount;
            }
        }
    }
}

void Simulation::step(float dt) {
    updateVelocity(dt);
    updateWater(dt);
    updatePigment(dt);
    updateSurfaceLayer(dt);
    updateCapillaryLayer();
}

void Simulation::updateRenderBuffer(DisplayMode mode) {
    const int w = m_grid.width;
    const int h = m_grid.height;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int idx    = m_grid.index(x, y);
            const int rgbIdx = 3 * x + 3 * y * w;

            float r = 0.0f, g = 0.0f, b = 0.0f;

            // 사전 곱셈 합성: out = premulColor + (1 - 농도) × 종이색
            switch (mode) {
            case DisplayMode::Composite: {
                float total = m_grid.pigmentDeposit[idx] + m_grid.pigment[idx];
                glm::vec3 combined = m_grid.depositColor[idx] + m_grid.surfaceColor[idx];
                float paper = m_grid.paper[rgbIdx];
                r = combined.r + (1.0f - total) * paper;
                g = combined.g + (1.0f - total) * paper;
                b = combined.b + (1.0f - total) * paper;
                break;
            }
            case DisplayMode::Water:
                r = g = b = m_grid.water[idx];
                break;
            case DisplayMode::Saturation:
                r = g = b = m_grid.saturation[idx];
                break;
            case DisplayMode::VelocityX:
                r = g = b = m_grid.velocity[idx].x;
                break;
            case DisplayMode::WetMask:
                r = g = b = m_grid.wetAreaMask[idx];
                break;
            case DisplayMode::Evaporation:
                r = g = b = m_grid.evaporation[idx];
                break;
            case DisplayMode::Deposit: {
                float paper = m_grid.paper[rgbIdx];
                float d     = m_grid.pigmentDeposit[idx];
                r = m_grid.depositColor[idx].r + (1.0f - d) * paper;
                g = m_grid.depositColor[idx].g + (1.0f - d) * paper;
                b = m_grid.depositColor[idx].b + (1.0f - d) * paper;
                break;
            }
            case DisplayMode::SurfacePigment: {
                float paper = m_grid.paper[rgbIdx];
                float p     = m_grid.pigment[idx];
                r = m_grid.surfaceColor[idx].r + (1.0f - p) * paper;
                g = m_grid.surfaceColor[idx].g + (1.0f - p) * paper;
                b = m_grid.surfaceColor[idx].b + (1.0f - p) * paper;
                break;
            }
            }

            m_grid.renderBuffer[rgbIdx + 0] = r;
            m_grid.renderBuffer[rgbIdx + 1] = g;
            m_grid.renderBuffer[rgbIdx + 2] = b;
        }
    }
}

// --- 결합 업데이트 스텝 -------------------------------------------------------

void Simulation::updateVelocity(float dt) {
    diffuse(params.velocityViscosity, m_grid.velocity.data(),
            m_grid.wetAreaMask.data(), dt);
    advect(m_grid.velocity.data(), m_grid.velocityTemp.data(),
           m_grid.velocity.data(), dt);
    addHeightDifferenceVelocity();
    applyBoundaryConditions();
}

void Simulation::updateWater(float dt) {
    diffuse(params.waterViscosity, m_grid.water.data(),
            m_grid.wetAreaMask.data(), dt);
    waterAdvect(m_grid.water.data(), m_grid.waterTemp.data(),
                m_grid.velocity.data(), m_grid.wetAreaMask.data(),
                static_cast<float>(params.speedMultiplier) * dt);
    flowOutward();
}

void Simulation::updatePigment(float dt) {
    // 안료 농도와 색상을 함께 확산 (색 경계 유지)
    diffuse(params.pigmentViscosity, m_grid.pigment.data(),
            m_grid.wetAreaMask.data(), dt);
    diffuse(params.pigmentViscosity, m_grid.surfaceColor.data(),
            m_grid.wetAreaMask.data(), dt);

    waterAdvect(m_grid.pigment.data(), m_grid.pigmentTemp.data(),
                m_grid.velocity.data(), m_grid.wetAreaMask.data(),
                static_cast<float>(params.speedMultiplier) * dt);
    advect(m_grid.surfaceColor.data(), m_grid.surfaceColorTemp.data(),
           m_grid.velocity.data(),
           static_cast<float>(params.speedMultiplier) * dt);
}

// --- 유체 솔버 ----------------------------------------------------------------

template<typename T>
T Simulation::sampleBilinear(const T* field, const glm::vec2& pos) const {
    const float wx = static_cast<float>(m_grid.width  - 1) - 1e-6f;
    const float wy = static_cast<float>(m_grid.height - 1) - 1e-6f;
    glm::vec2 p = glm::max(glm::vec2(0.0f), glm::min(glm::vec2(wx, wy), pos));

    const int   x = static_cast<int>(std::floor(p.x));
    const int   y = static_cast<int>(std::floor(p.y));
    const float s = p.x - static_cast<float>(x);
    const float t = p.y - static_cast<float>(y);

    T v0 = field[m_grid.index(x,     y)] * (1.0f - s)
         + field[m_grid.index(x + 1, y)] * s;
    T v1 = field[m_grid.index(x,     y + 1)] * (1.0f - s)
         + field[m_grid.index(x + 1, y + 1)] * s;
    return v0 * (1.0f - t) + v1 * t;
}

template<typename T>
void Simulation::advect(T* field, T* tempBuffer,
                         const glm::vec2* vel, float dt) {
    const int w = m_grid.width;
    const int h = m_grid.height;

    // 각 셀을 역추적해 출발점에서 샘플링
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            glm::vec2 departure = glm::vec2(static_cast<float>(x),
                                            static_cast<float>(y))
                                  - dt * vel[m_grid.index(x, y)];
            tempBuffer[m_grid.index(x, y)] = sampleBilinear(field, departure);
        }
    }
    for (int i = 0; i < w * h; ++i) field[i] = tempBuffer[i];
}

template<typename T>
void Simulation::diffuse(float k, T* field, const float* mask, float dt) {
    const int w = m_grid.width;
    const int h = m_grid.height;

    // 가우스-자이델 10회 반복 (k*dt가 작을 때 충분히 수렴)
    for (int iter = 0; iter < 10; ++iter) {
        for (int y = 1; y < h - 1; ++y) {
            for (int x = 1; x < w - 1; ++x) {
                if (!mask[m_grid.index(x, y)]) continue;
                // 암묵적 스킴: D_new = (D + k*dt * 이웃합) / (1 + 4*k*dt)
                field[m_grid.index(x, y)] =
                    (field[m_grid.index(x, y)]
                     + k * dt * (field[m_grid.index(x - 1, y)]
                                + field[m_grid.index(x + 1, y)]
                                + field[m_grid.index(x, y - 1)]
                                + field[m_grid.index(x, y + 1)]))
                    / (1.0f + 4.0f * k * dt);
            }
        }
    }
}

template<typename T>
void Simulation::waterAdvect(T* field, T* tempBuffer,
                              const glm::vec2* vel, const float* mask,
                              float dt) {
    const int w = m_grid.width;
    const int h = m_grid.height;

    for (int i = 0; i < w * h; ++i) tempBuffer[i] = field[i];

    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            const int c  = m_grid.index(x,     y);
            const int xp = m_grid.index(x + 1, y);
            const int xm = m_grid.index(x - 1, y);
            const int yp = m_grid.index(x,     y + 1);
            const int ym = m_grid.index(x,     y - 1);

            // 면 중심 속도: 인접 셀 평균, 건조 경계는 0으로 차단
            float vx1 = mask[c] * mask[xp] * (vel[c].x + vel[xp].x) * 0.5f;
            float vx2 = mask[c] * mask[xm] * (vel[c].x + vel[xm].x) * 0.5f;
            float vy1 = mask[c] * mask[yp] * (vel[c].y + vel[yp].y) * 0.5f;
            float vy2 = mask[c] * mask[ym] * (vel[c].y + vel[ym].y) * 0.5f;

            float flux = 0.0f;

            // +x 면
            if (vx1 > 0.0f) {
                flux = vx1 * dt * field[c] / 4.0f;
                tempBuffer[c] -= std::min(std::abs(k_maxWater - field[xp]),
                                          std::min(std::abs(flux), std::abs(k_minWater - field[c])));
            } else {
                flux = vx1 * dt * field[xp] / 4.0f;
                tempBuffer[c] += std::min(std::abs(k_maxWater - field[c]),
                                          std::min(std::abs(flux), std::abs(k_minWater - field[xm])));
            }

            // -x 면
            if (vx2 > 0.0f) {
                flux = vx2 * dt * field[xm] / 4.0f;
                tempBuffer[c] += std::min(std::abs(k_maxWater - field[c]),
                                          std::min(std::abs(flux), std::abs(k_minWater - field[xm])));
            } else {
                flux = vx2 * dt * field[c] / 4.0f;
                tempBuffer[c] -= std::min(std::abs(k_maxWater - field[xm]),
                                          std::min(std::abs(flux), std::abs(k_minWater - field[c])));
            }

            // +y 면
            if (vy1 > 0.0f) {
                flux = vy1 * dt * field[c] / 4.0f;
                tempBuffer[c] -= std::min(std::abs(k_maxWater - field[c]),
                                          std::min(std::abs(flux), std::abs(k_minWater - field[ym])));
            } else {
                flux = vy1 * dt * field[yp] / 4.0f;
                tempBuffer[c] += std::min(std::abs(k_maxWater - field[ym]),
                                          std::min(std::abs(flux), std::abs(k_minWater - field[c])));
            }

            // -y 면
            if (vy2 > 0.0f) {
                flux = vy2 * dt * field[ym] / 4.0f;
                tempBuffer[c] += std::min(std::abs(k_maxWater - field[yp]),
                                          std::min(std::abs(flux), std::abs(k_minWater - field[c])));
            } else {
                flux = vy2 * dt * field[c] / 4.0f;
                tempBuffer[c] -= std::min(std::abs(k_maxWater - field[c]),
                                          std::min(std::abs(flux), std::abs(k_minWater - field[yp])));
            }
        }
    }

    for (int i = 0; i < w * h; ++i) field[i] = tempBuffer[i];
}

// --- 시뮬레이션 서브스텝 ------------------------------------------------------

// 수위 기울기로 속도 갱신 (물이 낮은 쪽으로 흐름)
void Simulation::addHeightDifferenceVelocity() {
    const int w = m_grid.width;
    const int h = m_grid.height;

    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            const int c  = m_grid.index(x, y);
            glm::vec2 grad;
            grad.x = (m_grid.water[m_grid.index(x - 1, y)]
                     - m_grid.water[m_grid.index(x + 1, y)]) * 0.5f;
            grad.y = (m_grid.water[m_grid.index(x, y - 1)]
                     - m_grid.water[m_grid.index(x, y + 1)]) * 0.5f;
            // 이전 속도 90% + 수위 기반 성분 10%
            m_grid.velocity[c] = 0.9f * m_grid.velocity[c] + 0.1f * grad;
        }
    }
}

// 건조 셀 속도 = 0 (no-slip 경계 조건)
void Simulation::applyBoundaryConditions() {
    const int w = m_grid.width;
    const int h = m_grid.height;

    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            if (!m_grid.wetAreaMask[m_grid.index(x, y)])
                m_grid.velocity[m_grid.index(x, y)] = glm::vec2(0.0f);
        }
    }
}

// 경계 증발 처리: 가장자리 셀이 더 빨리 건조되어 외향 모세관류 발생
void Simulation::flowOutward() {
    const float evapRate   = 0.002f;  // 경계 증발 속도
    const int   blurRadius = 15;      // 경계 지시자 블러 반경
    const int   w          = m_grid.width;
    const int   h          = m_grid.height;

    for (int i = 0; i < w * h; ++i)
        m_grid.wetAreaMaskTemp[i] = m_grid.wetAreaMask[i];

    // 젖은 마스크를 블러 → 경계에서 0, 내부에서 1인 부드러운 지시자
    float* blurIn  = m_grid.wetAreaMaskTemp.data();
    float* blurOut = m_grid.evaporation.data();
    fastGaussianBlur(blurIn, blurOut, w, h, static_cast<float>(blurRadius));

    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            const int c = m_grid.index(x, y);
            // 경계(evaporation≈0)일수록 물 손실이 큼 → 안료가 가장자리로 집중
            float loss = evapRate * (1.0f - m_grid.evaporation[c]) * m_grid.wetAreaMask[c];
            m_grid.water[c] -= loss;
            if (m_grid.water[c] < 0.0f) m_grid.water[c] = 0.0f;

            // 물이 없는 젖은 셀은 포화도를 서서히 감소
            if (m_grid.water[c] == 0.0f && m_grid.wetAreaMask[c])
                m_grid.saturation[c] -= 0.01f;

            // 포화도가 임계값 이하이면 건조 처리
            if (m_grid.saturation[c] < params.wetMaskThreshold)
                m_grid.wetAreaMask[c] = 0.0f;
        }
    }
}

// 안료 흡착(수면→종이) / 탈착(종이→수면) 교환
void Simulation::updateSurfaceLayer(float dt) {
    const int w = m_grid.width;
    const int h = m_grid.height;

    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            const int c = m_grid.index(x, y);
            if (!m_grid.wetAreaMask[c]) continue;

            // 경계 강화 인자 (edge darkening, Van Laerhoven §3.3):
            // 경계(evaporation≈0)에서 흡착이 강해져 특유의 어두운 테두리 생성
            float boundaryFactor = 1.0f
                + std::max(0.0f, 1.0f - m_grid.evaporation[c]) * 3.0f;

            // 흡착: 종이 골(높이 낮음)에 더 많이 침착
            float adsorb = m_grid.pigment[c]
                           * (1.0f - m_grid.heightMap[c] * params.granulation)
                           * params.density
                           * boundaryFactor;

            // 탈착: 종이 봉우리(높이 높음)에서 재용출, 착색력으로 억제
            float desorb = m_grid.pigmentDeposit[c]
                           * (1.0f - (1.0f - m_grid.heightMap[c]) * params.granulation)
                           * params.density / params.staining;

            // 각 레이어가 1.0을 초과하지 않도록 클램프
            if (m_grid.pigmentDeposit[c] + adsorb > 1.0f)
                adsorb = std::max(0.0f, 1.0f - m_grid.pigmentDeposit[c]);
            if (m_grid.pigment[c] + desorb > 1.0f)
                desorb = std::max(0.0f, 1.0f - m_grid.pigment[c]);

            // 사전 곱셈 색상 이전: 농도 비율만큼 색상도 비례 이동
            if (adsorb > 0.0f && m_grid.pigment[c] > 0.001f) {
                float ratio = adsorb / m_grid.pigment[c];
                glm::vec3 transfer = m_grid.surfaceColor[c] * ratio;
                m_grid.depositColor[c] += transfer;
                m_grid.surfaceColor[c] -= transfer;
            }
            if (desorb > 0.0f && m_grid.pigmentDeposit[c] > 0.001f) {
                float ratio = desorb / m_grid.pigmentDeposit[c];
                glm::vec3 transfer = m_grid.depositColor[c] * ratio;
                m_grid.surfaceColor[c] += transfer;
                m_grid.depositColor[c] -= transfer;
            }

            m_grid.pigmentDeposit[c] += adsorb - desorb;
            m_grid.pigment[c]        += desorb - adsorb;
        }
    }
}

// 모세관층: 표면물 흡수 → 포화도 확산 → 젖은 마스크 갱신
void Simulation::updateCapillaryLayer() {
    const int   w     = m_grid.width;
    const int   h     = m_grid.height;
    const float sigma = params.wetMaskThreshold;
    const float eps   = params.capillaryThreshold;

    // 표면물을 모세관층으로 흡수 (남은 용량 한도 내)
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int c = m_grid.index(x, y);
            if (!m_grid.wetAreaMask[c]) continue;

            float absorbed = std::max(0.0f,
                std::min(params.absorption,
                         m_grid.capacity[c] - m_grid.saturation[c]));
            absorbed = std::min(absorbed, m_grid.water[c]);

            m_grid.saturation[c] += absorbed;
            m_grid.water[c]      -= absorbed;
        }
    }

    // 동시 업데이트를 위해 임시 버퍼로 복사
    for (int i = 0; i < w * h; ++i)
        m_grid.saturationTemp[i] = m_grid.saturation[i];

    // 포화도가 임계값 초과인 셀에서 이웃으로 확산
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            const int c = m_grid.index(x, y);
            if (m_grid.saturation[c] <= eps) continue;

            auto flowTo = [&](int neighbour) {
                if (m_grid.saturation[c] <= m_grid.saturation[neighbour]) return;
                float delta = std::max(0.0f,
                    std::min(m_grid.saturation[c] - m_grid.saturation[neighbour],
                             m_grid.capacity[neighbour] - m_grid.saturation[neighbour])
                    / 4.0f);
                m_grid.saturationTemp[c]         -= delta;
                m_grid.saturationTemp[neighbour] += delta;
            };

            flowTo(m_grid.index(x + 1, y));
            flowTo(m_grid.index(x - 1, y));
            flowTo(m_grid.index(x, y + 1));
            flowTo(m_grid.index(x, y - 1));
        }
    }

    for (int i = 0; i < w * h; ++i)
        m_grid.saturation[i] = m_grid.saturationTemp[i];

    // 포화도가 σ 초과인 셀을 젖은 상태로 표시
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (m_grid.saturation[m_grid.index(x, y)] > sigma)
                m_grid.wetAreaMask[m_grid.index(x, y)] = 1.0f;
        }
    }
}

// --- 템플릿 명시적 인스턴스화 -------------------------------------------------
// 템플릿 정의가 .cpp에 있으므로 필요한 타입을 명시적으로 인스턴스화
template void Simulation::advect<float>     (float*,      float*,      const glm::vec2*, float);
template void Simulation::advect<glm::vec2> (glm::vec2*,  glm::vec2*,  const glm::vec2*, float);
template void Simulation::advect<glm::vec3> (glm::vec3*,  glm::vec3*,  const glm::vec2*, float);
template void Simulation::diffuse<float>    (float, float*,      const float*, float);
template void Simulation::diffuse<glm::vec2>(float, glm::vec2*,  const float*, float);
template void Simulation::diffuse<glm::vec3>(float, glm::vec3*,  const float*, float);
template void Simulation::waterAdvect<float>(float*, float*, const glm::vec2*, const float*, float);
