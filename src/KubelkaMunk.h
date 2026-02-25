//
// KubelkaMunk.h
// WaterColorSimulation
//
// Kubelka-Munk (KM) optical paint model for physically-based pigment color mixing.
// Stores per-pigment K (absorption) and S (scattering) coefficients alongside
// measured reflectance values for both thick (colorB) and thin (colorW) paint layers.
// The PixelInfo struct uses the KM two-flux equations to mix stacked pigment layers.
//
#pragma once

#include <vector>
#include <glm/glm.hpp>

// Physical parameters of a single watercolor pigment based on the Kubelka-Munk model.
// colorW = measured reflectance at full dilution (thin layer over white background).
// colorB = measured reflectance over black background (used to derive K and S).
// K = absorption coefficient (per channel), S = scattering coefficient (per channel).
struct PigmentInfo {
    glm::vec3 colorB;   // Reflectance over black background
    glm::vec3 colorW;   // Reflectance over white background (diluted)
    glm::vec3 K;        // Absorption coefficient (R, G, B)
    glm::vec3 S;        // Scattering coefficient (R, G, B)

    void setQuinacridoneMagenta();
    void setIndianRed();
    void setCadmiumYellow();
    void setHookersGreen();
    void setCeruleanBlue();
    void setBurntUmber();
    void setCadmiumRed();
    void setInterferenceLilac();
    void setFrenchUltramarine();
};

// Per-pixel pigment state supporting multi-layer KM mixing.
// Call addPigment() to push layers and getReflectance() to compute the final color.
struct PixelInfo {
    std::vector<PigmentInfo> pigmentLayers;  // Stack of pigment layers (bottom to top)
    glm::vec3 reflectance;                   // Final computed reflectance
    glm::vec3 mixedR;                        // Intermediate total reflectance
    glm::vec3 mixedT;                        // Intermediate total transmittance

    void addPigment(const PigmentInfo& pigment);

    // Computes the final reflectance by iteratively applying KM mixing equations
    // across all stacked pigment layers.
    void getReflectance();

private:
    // KM auxiliary parameter: a = (1/2)(Rw + (Rb - Rw + 1)/Rb)
    glm::vec3 computeA(const glm::vec3& rWhite, const glm::vec3& rBlack) const;
    // b = sqrt(a^2 - 1)
    glm::vec3 computeB(const glm::vec3& a) const;
    // c = a*sinh(b*S*x) + b*cosh(b*S*x)  (denominator term)
    glm::vec3 computeC(const glm::vec3& a, const glm::vec3& b,
                       const glm::vec3& S, float thickness) const;
    // Single-layer reflectance: R = sinh(b*S*x) / c
    glm::vec3 computeLayerR(const glm::vec3& b, const glm::vec3& c,
                             const glm::vec3& S, float thickness) const;
    // Single-layer transmittance: T = b / c
    glm::vec3 computeLayerT(const glm::vec3& b, const glm::vec3& c) const;
    // Two-layer stack reflectance: R = R1 + T1^2*R2 / (1 - R1*R2)
    glm::vec3 mixReflectance(const glm::vec3& r1, const glm::vec3& r2,
                              const glm::vec3& t1) const;
    // Two-layer stack transmittance: T = T1*T2 / (1 - R1*R2)
    glm::vec3 mixTransmittance(const glm::vec3& r1, const glm::vec3& r2,
                                const glm::vec3& t1, const glm::vec3& t2) const;
};
