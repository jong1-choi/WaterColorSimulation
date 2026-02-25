//
// KubelkaMunk.cpp
// WaterColorSimulation
//
// Pigment preset data and KM optical mixing implementation.
//
#include "KubelkaMunk.h"

#include <cmath>
#include <algorithm>

// --- PigmentInfo presets -------------------------------------------------------
// Reflectance values from measured pigment samples; K/S derived from colorB/colorW.

void PigmentInfo::setQuinacridoneMagenta() {
    colorB = glm::vec3(30 / 255.f,  5 / 255.f,  15 / 255.f);
    colorW = glm::vec3(140 / 255.f, 40 / 255.f,  70 / 255.f);
    K = glm::vec3(0.22f, 1.47f, 0.57f);
    S = glm::vec3(0.05f, 0.003f, 0.03f);
}

void PigmentInfo::setIndianRed() {
    colorB = glm::vec3(120 / 255.f, 60 / 255.f, 40 / 255.f);
    colorW = glm::vec3(130 / 255.f, 70 / 255.f, 50 / 255.f);
    K = glm::vec3(0.46f, 1.07f, 1.50f);
    S = glm::vec3(1.28f, 0.38f, 0.21f);
}

void PigmentInfo::setCadmiumYellow() {
    colorB = glm::vec3(140 / 255.f, 100 / 255.f, 40 / 255.f);
    colorW = glm::vec3(180 / 255.f, 140 / 255.f, 50 / 255.f);
    K = glm::vec3(0.10f, 0.36f, 3.45f);
    S = glm::vec3(0.97f, 0.65f, 0.007f);
}

void PigmentInfo::setHookersGreen() {
    colorB = glm::vec3(2 / 255.f,  5 / 255.f,  1 / 255.f);
    colorW = glm::vec3(30 / 255.f, 80 / 255.f, 25 / 255.f);
    K = glm::vec3(1.62f, 0.61f, 1.64f);
    S = glm::vec3(0.01f, 0.012f, 0.003f);
}

void PigmentInfo::setCeruleanBlue() {
    colorB = glm::vec3(30 / 255.f,  70 / 255.f,  90 / 255.f);
    colorW = glm::vec3(50 / 255.f, 120 / 255.f, 130 / 255.f);
    K = glm::vec3(1.52f, 0.32f, 0.25f);
    S = glm::vec3(0.06f, 0.26f, 0.40f);
}

void PigmentInfo::setBurntUmber() {
    colorB = glm::vec3(25 / 255.f, 10 / 255.f,  1 / 255.f);
    colorW = glm::vec3(70 / 255.f, 30 / 255.f, 15 / 255.f);
    K = glm::vec3(0.74f, 1.54f, 2.10f);
    S = glm::vec3(0.09f, 0.004f, 0.09f);
}

void PigmentInfo::setCadmiumRed() {
    colorB = glm::vec3(120 / 255.f, 30 / 255.f, 15 / 255.f);
    colorW = glm::vec3(160 / 255.f, 50 / 255.f, 25 / 255.f);
    K = glm::vec3(0.14f, 1.08f, 1.68f);
    S = glm::vec3(0.77f, 0.015f, 0.018f);
}

void PigmentInfo::setInterferenceLilac() {
    colorB = glm::vec3(140 / 255.f, 100 / 255.f, 150 / 255.f);
    colorW = glm::vec3(190 / 255.f, 180 / 255.f, 190 / 255.f);
    K = glm::vec3(0.08f, 0.11f, 0.07f);
    S = glm::vec3(1.25f, 0.42f, 1.43f);
}

void PigmentInfo::setFrenchUltramarine() {
    colorB = glm::vec3(10 / 255.f, 10 / 255.f,  40 / 255.f);
    colorW = glm::vec3(30 / 255.f, 30 / 255.f, 180 / 255.f);
    K = glm::vec3(0.86f, 0.86f, 0.06f);
    S = glm::vec3(0.005f, 0.005f, 0.09f);
}

// --- PixelInfo KM mixing -------------------------------------------------------

void PixelInfo::addPigment(const PigmentInfo& pigment) {
    pigmentLayers.push_back(pigment);
}

void PixelInfo::getReflectance() {
    if (pigmentLayers.empty()) return;

    if (pigmentLayers.size() == 1) {
        reflectance = pigmentLayers[0].colorW;
        return;
    }

    // Iteratively mix layers from top (last) to bottom using KM two-flux equations
    int index = static_cast<int>(pigmentLayers.size()) - 1;
    while (index > 0) {
        glm::vec3 r1, t1, r2, t2;

        if (index == static_cast<int>(pigmentLayers.size()) - 1) {
            // First iteration: compute the top-most layer's R and T from scratch
            glm::vec3 a = computeA(pigmentLayers[index].colorW, pigmentLayers[index].colorB);
            glm::vec3 b = computeB(a);
            glm::vec3 c = computeC(a, b, pigmentLayers[index].S, 1.0f);
            r1 = computeLayerR(b, c, pigmentLayers[index].S, 1.0f);
            t1 = computeLayerT(b, c);
        } else {
            // Subsequent iterations: carry the accumulated R/T from the previous step
            r1 = mixedR;
            t1 = mixedT;
        }

        glm::vec3 a2 = computeA(pigmentLayers[index - 1].colorW, pigmentLayers[index - 1].colorB);
        glm::vec3 b2 = computeB(a2);
        glm::vec3 c2 = computeC(a2, b2, pigmentLayers[index - 1].S, 1.0f);
        r2 = computeLayerR(b2, c2, pigmentLayers[index - 1].S, 1.0f);
        t2 = computeLayerT(b2, c2);

        mixedR = mixReflectance(r1, r2, t1);
        mixedT = mixTransmittance(r1, r2, t1, t2);
        reflectance = mixedR;

        --index;
    }
}

glm::vec3 PixelInfo::computeA(const glm::vec3& rWhite, const glm::vec3& rBlack) const {
    return glm::vec3(
        0.5f * (rWhite.r + (rBlack.r - rWhite.r + 1.0f) / rBlack.r),
        0.5f * (rWhite.g + (rBlack.g - rWhite.g + 1.0f) / rBlack.g),
        0.5f * (rWhite.b + (rBlack.b - rWhite.b + 1.0f) / rBlack.b));
}

glm::vec3 PixelInfo::computeB(const glm::vec3& a) const {
    return glm::vec3(
        std::sqrt(a.r * a.r - 1.0f),
        std::sqrt(a.g * a.g - 1.0f),
        std::sqrt(a.b * a.b - 1.0f));
}

glm::vec3 PixelInfo::computeC(const glm::vec3& a, const glm::vec3& b,
                               const glm::vec3& S, float thickness) const {
    return glm::vec3(
        a.r * std::sinh(b.r * S.r * thickness) + b.r * std::cosh(b.r * S.r * thickness),
        a.g * std::sinh(b.g * S.g * thickness) + b.g * std::cosh(b.g * S.g * thickness),
        a.b * std::sinh(b.b * S.b * thickness) + b.b * std::cosh(b.b * S.b * thickness));
}

glm::vec3 PixelInfo::computeLayerR(const glm::vec3& b, const glm::vec3& c,
                                    const glm::vec3& S, float thickness) const {
    return glm::vec3(
        std::sinh(b.r * S.r * thickness) / c.r,
        std::sinh(b.g * S.g * thickness) / c.g,
        std::sinh(b.b * S.b * thickness) / c.b);
}

glm::vec3 PixelInfo::computeLayerT(const glm::vec3& b, const glm::vec3& c) const {
    return glm::vec3(b.r / c.r, b.g / c.g, b.b / c.b);
}

glm::vec3 PixelInfo::mixReflectance(const glm::vec3& r1, const glm::vec3& r2,
                                     const glm::vec3& t1) const {
    return glm::vec3(
        r1.r + t1.r * t1.r * r2.r / (1.0f - r1.r * r2.r),
        r1.g + t1.g * t1.g * r2.g / (1.0f - r1.g * r2.g),
        r1.b + t1.b * t1.b * r2.b / (1.0f - r1.b * r2.b));
}

glm::vec3 PixelInfo::mixTransmittance(const glm::vec3& r1, const glm::vec3& r2,
                                       const glm::vec3& t1, const glm::vec3& t2) const {
    return glm::vec3(
        t1.r * t2.r / (1.0f - r1.r * r2.r),
        t1.g * t2.g / (1.0f - r1.g * r2.g),
        t1.b * t2.b / (1.0f - r1.b * r2.b));
}
