//
// GaussianBlur.cpp
// WaterColorSimulation
//
// Fast approximate Gaussian blur using three box-blur passes.
// Reference: http://blog.ivank.net/fastest-gaussian-blur.html
//
#include "GaussianBlur.h"

#include <cmath>
#include <algorithm>

namespace {

// Converts Gaussian sigma to the three box radii that approximate it.
// Fills 'boxes' with n box radii (each is (boxWidth-1)/2).
// Reference: https://www.peterkovesi.com/papers/FastGaussianSmoothing.pdf
void sigmaToBoxRadii(int boxes[], float sigma, int numBoxes) {
    float idealWidth = std::sqrt((12.0f * sigma * sigma / numBoxes) + 1.0f);
    int widthLow  = static_cast<int>(std::floor(idealWidth));
    if (widthLow % 2 == 0) --widthLow;
    int widthHigh = widthLow + 2;

    float midpoint = (12.0f * sigma * sigma - numBoxes * widthLow * widthLow
                      - 4.0f * numBoxes * widthLow - 3.0f * numBoxes)
                     / (-4.0f * widthLow - 4.0f);
    int m = static_cast<int>(std::round(midpoint));

    for (int i = 0; i < numBoxes; ++i)
        boxes[i] = ((i < m ? widthLow : widthHigh) - 1) / 2;
}

// Single horizontal box-blur pass over a width*height float image.
// Reads from 'in', writes to 'out'. r is the box half-width.
void horizontalBoxBlur(const float* in, float* out, int width, int height, int r) {
    const float invSize = 1.0f / static_cast<float>(r + r + 1);

    for (int row = 0; row < height; ++row) {
        int ti = row * width;   // write pointer
        int li = ti;            // left read pointer
        int ri = ti + r;        // right read pointer

        float firstVal = in[ti];
        float lastVal  = in[ti + width - 1];
        float val      = static_cast<float>(r + 1) * firstVal;

        for (int j = 0; j < r; ++j) val += in[ti + j];

        // Left ramp: right pointer runs ahead, left pointer is clamped at firstVal
        for (int j = 0; j <= r; ++j)       { val += in[ri++] - firstVal; out[ti++] = val * invSize; }
        // Main body: sliding window
        for (int j = r + 1; j < width - r; ++j) { val += in[ri++] - in[li++]; out[ti++] = val * invSize; }
        // Right ramp: right pointer clamped at lastVal
        for (int j = width - r; j < width; ++j) { val += lastVal - in[li++]; out[ti++] = val * invSize; }
    }
}

// Single vertical box-blur pass over a width*height float image.
// Reads from 'in', writes to 'out'. r is the box half-height.
void verticalBoxBlur(const float* in, float* out, int width, int height, int r) {
    const float invSize = 1.0f / static_cast<float>(r + r + 1);

    for (int col = 0; col < width; ++col) {
        int ti = col;           // write pointer
        int li = ti;            // top read pointer
        int ri = ti + r * width;// bottom read pointer

        float firstVal = in[ti];
        float lastVal  = in[ti + width * (height - 1)];
        float val      = static_cast<float>(r + 1) * firstVal;

        for (int j = 0; j < r; ++j) val += in[ti + j * width];

        for (int j = 0; j <= r;         ++j) { val += in[ri] - firstVal; out[ti] = val * invSize; ri += width; ti += width; }
        for (int j = r + 1; j < height - r; ++j) { val += in[ri] - in[li]; out[ti] = val * invSize; li += width; ri += width; ti += width; }
        for (int j = height - r; j < height; ++j) { val += lastVal - in[li]; out[ti] = val * invSize; li += width; ti += width; }
    }
}

// One full box-blur pass: horizontal then vertical, using in/out as ping-pong buffers.
// After the call the blurred data is in 'out'; the pointers are swapped internally.
void boxBlur(float*& in, float*& out, int width, int height, int r) {
    std::swap(in, out);
    horizontalBoxBlur(out, in, width, height, r);
    verticalBoxBlur(in, out, width, height, r);
    // Note: anisotropic blur is possible by using different r values for H vs V passes.
}

} // anonymous namespace

// Three box-blur passes give a good approximation to a true Gaussian for most sigmas.
void fastGaussianBlur(float*& in, float*& out, int width, int height, float sigma) {
    int boxes[3];
    sigmaToBoxRadii(boxes, sigma, 3);
    boxBlur(in,  out, width, height, boxes[0]);
    boxBlur(out, in,  width, height, boxes[1]);
    boxBlur(in,  out, width, height, boxes[2]);
}
