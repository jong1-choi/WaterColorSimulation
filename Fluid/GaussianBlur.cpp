//
//  GaussianBlur.cpp
//  Fluid
//
//  Created by 최종원 on 2023/02/08.
//

#include "GaussianBlur.hpp"
#include <cmath>
#include <vector>

const int GRID_W = 256;
const int GRID_H = 256;

//!
//! \fn void std_to_box(int boxes[], float sigma, int n)
//!
//! \brief this function converts the standard deviation of
//! Gaussian blur into dimensions of boxes for box blur. For
//! further details please refer to :
//! https://www.peterkovesi.com/matlabfns/#integral
//! https://www.peterkovesi.com/papers/FastGaussianSmoothing.pdf
//!
//! \param[out] boxes   boxes dimensions
//! \param[in] sigma    Gaussian standard deviation
//! \param[in] n        number of boxes
//!
void std_to_box(int boxes[], float sigma, int n)
{
    // ideal filter width
    float wi = std::sqrt((12*sigma*sigma/n)+1);
    int wl = std::floor(wi);
    if(wl%2==0) wl--;
    int wu = wl+2;
                
    float mi = (12*sigma*sigma - n*wl*wl - 4*n*wl - 3*n)/(-4*wl - 4);
    int m = std::round(mi);
                
    for(int i=0; i<n; i++)
        boxes[i] = ((i < m ? wl : wu) - 1) / 2;
}

//!
//! \fn void horizontal_blur(float * in, float * out, int w, int h, int r)
//!
//! \brief this function performs the horizontal blur pass for box blur.
//!
//! \param[in,out] in       source channel
//! \param[in,out] out      target channel
//! \param[in] w            image width
//! \param[in] h            image height
//! \param[in] r            box dimension
//!
void horizontal_blur(float * in, float * out, int w, int h, int r)
{
    float iarr = 1.f / (r+r+1);
    #pragma omp parallel for
    for(int i=0; i<h; i++)
    {
        int ti = i*w, li = ti, ri = ti+r;
        float fv = in[ti], lv = in[ti+w-1], val = (r+1)*fv;

        for(int j=0; j<r; j++) val += in[ti+j];
        for(int j=0  ; j<=r ; j++) { val += in[ri++] - fv      ; out[ti++] = val*iarr; }
        for(int j=r+1; j<w-r; j++) { val += in[ri++] - in[li++]; out[ti++] = val*iarr; }
        for(int j=w-r; j<w  ; j++) { val += lv       - in[li++]; out[ti++] = val*iarr; }
    }
}

//!
//! \fn void total_blur(float * in, float * out, int w, int h, int r)
//!
//! \brief this function performs the total blur pass for box blur.
//!
//! \param[in,out] in       source channel
//! \param[in,out] out      target channel
//! \param[in] w            image width
//! \param[in] h            image height
//! \param[in] r            box dimension
//!
void total_blur(float * in, float * out, int w, int h, int r)
{
    float iarr = 1.f / (r+r+1);
    #pragma omp parallel for
    for(int i=0; i<w; i++)
    {
        int ti = i, li = ti, ri = ti+r*w;
        float fv = in[ti], lv = in[ti+w*(h-1)], val = (r+1)*fv;
        for(int j=0; j<r; j++) val += in[ti+j*w];
        for(int j=0  ; j<=r ; j++) { val += in[ri] - fv    ; out[ti] = val*iarr; ri+=w; ti+=w; }
        for(int j=r+1; j<h-r; j++) { val += in[ri] - in[li]; out[ti] = val*iarr; li+=w; ri+=w; ti+=w; }
        for(int j=h-r; j<h  ; j++) { val += lv     - in[li]; out[ti] = val*iarr; li+=w; ti+=w; }
    }
}

//!
//! \fn void box_blur(float * in, float * out, int w, int h, int r)
//!
//! \brief this function performs a box blur pass.
//!
//! \param[in,out] in       source channel
//! \param[in,out] out      target channel
//! \param[in] w            image width
//! \param[in] h            image height
//! \param[in] r            box dimension
//!
void box_blur(float *& in, float *& out, int w, int h, int r)
{
    std::swap(in, out);
    horizontal_blur(out, in, w, h, r);
    total_blur(in, out, w, h, r);
    // Note to myself :
    // here we could go anisotropic with different radiis rx,ry in HBlur and TBlur
}

//!
//! \fn void fast_gaussian_blur(float * in, float * out, int w, int h, float sigma)
//!
//! \brief this function performs a fast Gaussian blur. Applying several
//! times box blur tends towards a true Gaussian blur. Three passes are sufficient
//! for good results. For further details please refer to :
//! http://blog.ivank.net/fastest-gaussian-blur.html
//!
//! \param[in,out] in       source channel
//! \param[in,out] out      target channel
//! \param[in] w            image width
//! \param[in] h            image height
//! \param[in] r            box dimension
//!
void fast_gaussian_blur(float *& in, float *& out, int w, int h, float sigma)
{
    // sigma conversion to box dimensions
    int boxes[3];
    std_to_box(boxes, sigma, 3);
    box_blur(in, out, w, h, boxes[0]);
    box_blur(out, in, w, h, boxes[1]);
    box_blur(in, out, w, h, boxes[2]);
}

//
//using std::vector;
//#define IX(X,Y) ((std::min(GRID_H-1,(std::max(0,Y)))*GRID_W)+std::min(GRID_W-1,(std::max(0,X))))
//
//
//vector<int> boxesForGauss(int sigma, int n)
//{
//    const float w_ideal = std::sqrt(12.0*sigma*sigma/(float)n + 1.0);
//    int wl = std::floor(w_ideal);
//    if (wl % 2 == 0)
//        wl--;
//    const int wu = wl + 2;
//
//    const int m = std::round((float)(12*sigma*sigma - n*wl*wl - 4*n*wl - 3*n) / (-4*wl - 4));
//
//    vector<int> sizes;
//    for (int i = 0; i < n; i++)
//        sizes.push_back(i < m ? wl : wu);
//    return sizes;
//}
//
//void boxBlurH_4(float* scl, float* tcl, int w, int h, int r)
//{
//    const float iarr = 1.0f / (r+r+1);
//    for (int j = 0; j < h; j++)
//    {
//        const float fv = scl[IX(0,j)];
//        const float lv = scl[IX(w-1, j)];
//        int ti = 0; // pointer to target index
//        int li = 0; // pointer to left index
//        int ri = r; // pointer to right index
//        float val = (r + 1) * fv;
//        int i;
//        for (i = 0; i < r; i++)
//            val += scl[IX(i, j)];
//        for (i = 0; i <= r; i++)
//        {
//            val += scl[IX(ri++, j)] - fv;           tcl[IX(ti++, j)] = val * iarr;
//        }
//        for (i = r + 1; i < w - r; i++)
//        {
//            val += scl[IX(ri++, j)] - scl[IX(li++, j)]; tcl[IX(ti++, j)] = val * iarr;
//        }
//        for (i = w - r; i < w; i++)
//        {
//            val += lv  - scl[IX(li++, j)]; tcl[IX(ti++, j)] = val * iarr;
//        }
//    }
//}
//
//void boxBlurT_4(float* scl, float* tcl, int w, int h, int r)
//{
//    const float iarr = 1.0f / (r+r+1);
//    for (int i = 0; i < w; i++)
//    {
//        const float fv = scl[IX(i, 0)];
//        const float lv = scl[IX(i, h-1)];
//        int ti = 0; // pointer to target index
//        int li = 0; // pointer to left index
//        int ri = r; // pointer to right index
//        float val = (r + 1) * fv;
//        int j;
//        for (j = 0; j < r; j++)
//            val += scl[IX(i, j)];
//        for (j = 0; j <= r; j++)
//        {
//            val += scl[IX(i, ri++)] - fv;
//            tcl[IX(i, ti++)] = val * iarr;
//        }
//        for (j = r + 1; j < h - r; j++)
//        {
//            val += scl[IX(i, ri++)] - scl[IX(i, li++)];
//            tcl[IX(i, ti++)] = val * iarr;
//        }
//        for (j = h - r; j < h; j++)
//        {
//            val += lv - scl[IX(i, li++)];
//            tcl[IX(i, ti++)] = val * iarr;
//        }
//    }
//}
//
//void boxBlur_4(float* scl, float* tcl, int w, int h, int r)
//{
//    for(int i = 0; i < w; i++) for(int j = 0; j < h; j++) tcl[i] = scl[i];
//    boxBlurH_4(tcl, scl, w, h, r);
//    boxBlurT_4(scl, tcl, w, h, r);
//}
//
//// approximate gaussian blur by applying box blur 3 times
//void gaussBlur_4(float* scl, float* tcl, int w, int h, int r)
//{
//    vector<int> boxes = boxesForGauss(r, 3);
//    boxBlur_4(scl, tcl, w, h, (boxes[0] - 1) / 2);
//    boxBlur_4(tcl, scl, w, h, (boxes[1] - 1) / 2);
//    boxBlur_4(scl, tcl, w, h, (boxes[2] - 1) / 2);
//}
//
//void approximateGaussianBlur(float* scl, float* tcl, int w, int h, int r)
//{
//    gaussBlur_4(scl, tcl, w, h, r);
//}
