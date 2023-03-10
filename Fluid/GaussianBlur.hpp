//
//  GaussianBlur.hpp
//  Fluid
//
//  Created by 최종원 on 2023/02/08.
//

#ifndef GaussianBlur_hpp
#define GaussianBlur_hpp

#include <iostream>

/**
 * Approximate gaussian blur, from
 * http://blog.ivank.net/fastest-gaussian-blur.html.
 */
//void approximateGaussianBlur(float* scl, float* tcl, int w, int h, int r);
void fast_gaussian_blur(float *& in, float *& out, int w, int h, float sigma);

#endif
