//
// GaussianBlur.h
// WaterColorSimulation
//
// Fast approximate Gaussian blur using three consecutive box-blur passes.
// Algorithm reference: http://blog.ivank.net/fastest-gaussian-blur.html
// Standard deviation → box dimensions conversion:
//   https://www.peterkovesi.com/papers/FastGaussianSmoothing.pdf
//
#pragma once

// Performs a fast Gaussian blur approximation on a single-channel float image.
// Three box-blur passes approach a true Gaussian as sigma increases.
//
// After the call, the blurred result is in 'out'; 'in' may be modified as a
// working buffer. The caller owns both buffers.
//
// Parameters:
//   in    - source float buffer (width * height elements); modified during processing
//   out   - destination float buffer (width * height elements); receives the result
//   width - image width in pixels
//   height- image height in pixels
//   sigma - Gaussian standard deviation (controls blur radius)
void fastGaussianBlur(float*& in, float*& out, int width, int height, float sigma);
