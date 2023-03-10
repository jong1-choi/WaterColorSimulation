//
//  WaterColor.hpp
//  Fluid
//
//  Created by 최종원 on 2022/11/29.
//

#ifndef WaterColor_hpp
#define WaterColor_hpp
#include "PerlinNoise.h"
#include "KubelkaMunk.hpp"
#include <iostream>
#include <algorithm>
#include <stdio.h>
#include <JGL/JGL.hpp>
#include <cstdlib>
#include <ctime>

struct Grid {
    const int width = 0;
    const int height = 0;
    const float maxWater = 1;
    const float minWater = .1;
    PixelInfo* pixel = nullptr;
    float* R  = nullptr;
    float* paper  = nullptr;
    float* pigment = nullptr;
    float* pigmentTemp = nullptr;
    float* pigmentDeposit = nullptr;
    float* saturation = nullptr;
    float* saturationTemp = nullptr;
    float* water = nullptr;
    float* waterTemp = nullptr;
    float* capacity = nullptr;
    glm::vec2* velocity = nullptr;
    glm::vec2* velocityTemp = nullptr;
    float* wetAreaMask = nullptr;
    float* wetAreaMaskTemp = nullptr;
    float* evaporation = nullptr;
    float* heightMap = nullptr;

    
    //    Grid(int w, int h, float* pp, float* p, float* pT, glm::vec2* v, glm::vec2* vT, bool* a, float* hM) : width(w), height(h), pressure(pp),  pigment(p), pigmentTemp(pT), velocity(v), velocityTemp(vT), wetAreaMask(a), heightMap(hM) {}
    Grid(int w, int h) : width(w), height(h) {}
    
    void init() {
        pixel = new PixelInfo[width * height]{};
        pigment = new float[width * height]{};
        pigmentTemp = new float[width * height]{};
        pigmentDeposit = new float[width * height]{};
        water = new float[width * height]{};
        waterTemp = new float[width * height]{};
        saturation = new float[width * height]{};
        saturationTemp = new float[width * height]{};
        velocity = new glm::vec2[width * height]{};
        velocityTemp = new glm::vec2[width * height]{};
        capacity = new float[width * height]{};
        heightMap = new float[width * height]{};
        wetAreaMask = new float[width * height]{};
        wetAreaMaskTemp = new float[width * height]{};
        evaporation = new float[width * height]{};
//        isBlack = new bool[width * height]{};
        paper = new float[3 * width * height]{};
        R = new float[3 * width * height]{};
        
//        for(int y = 0; y < height; y++) for(int x = width/3; x < width - width/3; x++){
//            isBlack[width * y + x] = true;
//        }
        generateHM();
        
        for(auto i = 0; i < width * height; i++){
//            T[3*i] = 0.f;
//            T[3*i+1] = 0.f;
//            T[3*i+2] = 0.f;
//            if(isBlack[i] == true){
//                R[3*i] = 20/255.f;
//                R[3*i+1] = 20/255.f;
//                R[3*i+2] = 20/255.f;
//            }
//            else {
//                R[3*i] = 250/255.f;
//                R[3*i+1] = 250/255.f;
//                R[3*i+2] = 250/255.f;
//            }
            paper[3*i] = 1 - heightMap[i] * 0.05f;
            paper[3*i+1] = 1 - heightMap[i] * 0.05f;
            paper[3*i+2] = 1 - heightMap[i] * 0.05f;
//            paper[3*i] = heightMap[i];
//            paper[3*i+1] = heightMap[i];
//            paper[3*i+2] = heightMap[i];
    
            R[3*i] = 1;
            R[3*i+1] = 1;
            R[3*i+2] = 1;
            
            wetAreaMask[i] = 0;
            capacity[i] = heightMap[i] * (0.7 - 0.2) + 0.2;
        }
        
    }
    
    
    
    void generateHM() {
//        unsigned int seed = 237;
        PerlinNoise pn;

        unsigned int kk = 0;
        // Visit every pixel of the image and assign a color generated with Perlin noise
        for(unsigned int i = 0; i < height; ++i) {     // y
            for(unsigned int j = 0; j < width; ++j) {  // x
                double x = (double)j/((double)width);
                double y = (double)i/((double)height);

                // Typical Perlin noise
                double n = pn.noise(200*x, 200*y, 0);
                heightMap[kk] = n;
//                heightMap[kk] = 1 - n * 0.05f;
                kk++;
            }
        }
    }
    
    
};



#endif /* WaterColor_hpp */
