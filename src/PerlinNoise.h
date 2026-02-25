//
// PerlinNoise.h
// WaterColorSimulation
//
// C++ translation of Ken Perlin's improved noise (2002) reference implementation.
// Original Java copyright 2002 Ken Perlin (http://mrl.nyu.edu/~perlin/noise/).
// Permutation vector randomization added for seeded initialization.
//
#pragma once

#include <vector>

// Generates smooth pseudo-random noise values via Ken Perlin's improved algorithm.
// Use noise(x, y, z) to sample the field; z=0 produces a 2D slice.
class PerlinNoise {
public:
    // Initialize with the standard reference permutation vector
    PerlinNoise();

    // Initialize with a custom seed, producing a different but reproducible noise field
    explicit PerlinNoise(unsigned int seed);

    // Sample the noise field at (x, y, z). Returns a value in [0, 1].
    double noise(double x, double y, double z) const;

private:
    std::vector<int> m_permutation;  // Double-length permutation table for wrap-around

    double fade(double t) const;
    double lerp(double t, double a, double b) const;
    double grad(int hash, double x, double y, double z) const;
};
