//
//  main.cpp
//  Fluid
//
//  Created by Hyun Joon Shin on 2021/05/31.
//

#include <iostream>
#include <JGL/JGL.hpp>
#include <JGL/JGL_Button.hpp>
#include <JGL/JGL_Slider.hpp>
#include <JGL/JGL__Valued.hpp>

#include "GLTools.hpp"
#include "TexView.hpp"
#include "GaussianBlur.hpp"
#include <algorithm>
#include "WaterColor.hpp"

using namespace glm;
using namespace JGL;

Grid* grid;
TexView* view;
Window* window;
PigmentInfo pigment;
const int GRID_W = 256;  // width of grid
const int GRID_H = 256;  // height of grid
int radius = 10;         // brush's radius
int play = 0;            // simulation on off
int Speed = 1;          // simulation speed

float kappaV = .15f;     // velocity kinetic viscosity
float kappaW = .15f;     // water velocity kinetic viscosity
float kappaP = .2f;     // pigment velocity kinetic viscosity
float Alpha = 0.05f;       // apsorption from surface layer to capillary layer
float Epsilon = 0.5f;     // minimum water to diffuse in capillary layer
float Sigma = 0.1f;       // sigma < water -> wetMask
float Granule = 0.2f; // pigment granulation
float Density = 0.4f;     // pigment density
float Staining = 1.1f;    // pigment staining power
float quantityW = 2.f;   // quantity of water
float quantityP = 0.2;   // qauntity of pigment

int flag = 0;
float maxWater = 10;
float minWater = 0.1;
float maxPig = 2.1f;

ivec2 mousePt;
ivec2 lastPt;
bool pressed = false;

#define IX(X,Y) ((std::min(GRID_H-1,(std::max(0,Y)))*GRID_W)+std::min(GRID_W-1,(std::max(0,X))))
//#define IX(X,Y) ((GRID_W)*(Y) + (X))

void init() {
    grid->init();
    pigment.QuinacridoneMagenta();
}

void UIinit() {
    window->alignment(JGL::ALIGN_ALL);
    JGL::Aligner* imageAligner = new JGL::Aligner(0, 0, window->w(), window->h());
    imageAligner->type(Aligner::HORIZONTAL);
    
    view = new TexView(0,0,1024,1024);
    imageAligner->resizable(view);
    JGL::Aligner* sliderAligner = new JGL::Aligner(0, 0, window->w() - view->w(), window->h());
    sliderAligner->type(JGL::Aligner::VERTICAL);
    sliderAligner->padding(5);
    sliderAligner->topPadding(10);
    sliderAligner->spacing(4);
    sliderAligner->alignment(JGL::ALIGN_TOP | JGL::ALIGN_SIDE);
    
    JGL::Slider<int>* speed = new JGL::Slider<int>(0, 0, sliderAligner->w(), JGL::_size_button_height(), "Speed");
    speed->range(1, 20);
    speed->autoValue(Speed);
    
    JGL::Slider<float>* kappav = new JGL::Slider<float>(0, 0, sliderAligner->w(), JGL::_size_button_height(), "kappaV");
    kappav->range(0, 1);
    kappav->autoValue(kappaV);
    
    JGL::Slider<float>* kappaw = new JGL::Slider<float>(0, 0, sliderAligner->w(), JGL::_size_button_height(), "kappaW");
    kappaw->range(0, 1);
    kappaw->autoValue(kappaW);
    
    JGL::Slider<float>* kappap = new JGL::Slider<float>(0, 0, sliderAligner->w(), JGL::_size_button_height(), "kappaP");
    kappap->range(0, 5);
    kappap->autoValue(kappaP);
    
    JGL::Slider<float>* alpha = new JGL::Slider<float>(0, 0, sliderAligner->w(), JGL::_size_button_height(), "Alpha");
    alpha->range(0, 0.5);
    alpha->autoValue(Alpha);
    
    JGL::Slider<float>* epslion = new JGL::Slider<float>(0, 0, sliderAligner->w(), JGL::_size_button_height(), "Epsilon");
    epslion->range(0, 1);
    epslion->autoValue(Epsilon);
    
    JGL::Slider<float>* sigma = new JGL::Slider<float>(0, 0, sliderAligner->w(), JGL::_size_button_height(), "Sigma");
    sigma->range(0, 1);
    sigma->autoValue(Sigma);
    
    JGL::Slider<float>* granule = new JGL::Slider<float>(0, 0, sliderAligner->w(), JGL::_size_button_height(), "Granule");
    granule->range(0, 1);
    granule->autoValue(Granule);
    
    JGL::Slider<float>* density = new JGL::Slider<float>(0, 0, sliderAligner->w(), JGL::_size_button_height(), "Density");
    density->range(0, 1);
    density->autoValue(Density);
    
    JGL::Slider<float>* staining = new JGL::Slider<float>(0, 0, sliderAligner->w(), JGL::_size_button_height(), "Staining");
    staining->range(0, 10);
    staining->autoValue(Staining);
    
    JGL::Slider<float>* quantityw = new JGL::Slider<float>(0, 0, sliderAligner->w(), JGL::_size_button_height(), "Water");
    quantityw->range(0, 10);
    quantityw->autoValue(quantityW);
    
    JGL::Slider<float>* quantityp = new JGL::Slider<float>(0, 0, sliderAligner->w(), JGL::_size_button_height(), "Pigment");
    quantityp->range(0, 1);
    quantityp->autoValue(quantityP);
}

template<typename T> T sample( T* buf, int w, int h, const vec2& pp ) {
    vec2 p = max(vec2(0,0), min(vec2(w-1.000001, h-1.000001),pp));
    int x = int(floor(p.x));
    int y = int(floor(p.y));
    float s = p.x - x;
    float t = p.y - y;
    T v1 = buf[IX(x,y)]*(1-s) + buf[IX(x+1,y)]*s;
    T v2 = buf[IX(x,y+1)]*(1-s) + buf[IX(x+1,y+1)]*s;
    return v1*(1-t)+v2*t;
}

template<typename T>
void advection( T* D, T* D2, glm::vec2* v, float dt ) {
    for( auto y = 0; y < GRID_H; y++ ) for( auto x = 0; x < GRID_W; x++ ){
            D2[IX(x,y)] = sample( D, GRID_W, GRID_H, glm::vec2(x,y) - dt*v[IX(x,y)]);
    }
    for( auto i = 0; i < GRID_W*GRID_H; i++)
        D[i] = D2[i];
}


// 내일 디퓨즈 고치기
template<typename T>
void diffuse( float k, T* D, float* m, float dt ) {
    for( auto iter = 0; iter < 10; iter++)
        for( auto y = 1; y < GRID_H - 1; y++ ) for( auto x = 1; x < GRID_W - 1; x++ ){
            if( m[IX(x,y)] )
                D[IX(x,y)] = ( D[IX(x, y)] +
                              k*dt*( D[IX(x-1,y)] + D[IX(x+1,y)] + D[IX(x,y-1)] + D[IX(x,y+1)] ) ) / (1+4*k*dt);
            
//            if( !m[IX(x,y)] ) {
//                if( m[IX(x+1,y)] || m[IX(x-1,y)] || m[IX(x,y+1)] || m[IX(x,y-1)] ){
//                    float size = m[IX(x+1,y)] + m[IX(x-1,y)] + m[IX(x,y+1)] + m[IX(x,y-1)];
//                    D[IX(x-1,y)] += m[IX(x-1,y)] * D[IX(x,y)] / size;
//                    D[IX(x+1,y)] += m[IX(x+1,y)] * D[IX(x,y)] / size;
//                    D[IX(x,y+1)] += m[IX(x,y+1)] * D[IX(x,y)] / size;
//                    D[IX(x,y-1)] += m[IX(x,y-1)] * D[IX(x,y)] / size;
//                }
//            }
        }
}

template<typename T>
void waterAdvection( T* D, T* D2, glm::vec2* v, float* m, float dt) {
    for(int i = 0; i < GRID_H*GRID_W; i++) D2[i] = D[i];
    
    for( auto y = 1; y < GRID_H - 1; y++ ) for( auto x = 1; x < GRID_W - 1; x++ ){
        
        float flux, vx1, vx2, vy1, vy2;
        
        vx1 = m[IX(x,y)] * m[IX(x+1,y)] * ( v[IX(x,y)].x + v[IX(x+1,y)].x )/2.f;
        vx2 = m[IX(x,y)] * m[IX(x-1,y)] * ( v[IX(x,y)].x + v[IX(x-1,y)].x )/2.f;
        vy1 = m[IX(x,y)] * m[IX(x,y+1)] * ( v[IX(x,y)].y + v[IX(x,y+1)].y )/2.f;
        vy2 = m[IX(x,y)] * m[IX(x,y-1)] * ( v[IX(x,y)].y + v[IX(x,y-1)].y )/2.f;
        
        if( vx1 > 0 ) {
            flux = vx1 * dt * D[IX(x,y)] / 4;
            D2[IX(x,y)] -= std::min( abs(maxWater - D[IX(x+1,y)]), std::min( abs(flux), abs(minWater - D[IX(x,y)]) ) );
//            D2[IX(x,y)] -= flux;
        }
        else {
            flux = vx1 * dt * D[IX(x+1,y)] / 4;
            D2[IX(x,y)] += std::min( abs(maxWater - D[IX(x,y)]), std::min( abs(flux), abs(minWater - D[IX(x-1,y)]) ) );
//            D2[IX(x,y)] -= flux;
        }

        if( vx2 > 0 ) {
            flux = vx2 * dt * D[IX(x-1,y)] / 4;
            D2[IX(x,y)] += std::min( abs(maxWater - D[IX(x,y)]), std::min( abs(flux), abs(minWater - D[IX(x-1,y)]) ) );
//            D2[IX(x,y)] += flux;
        }
        else {
            flux = vx2 * dt * D[IX(x,y)] / 4;
            D2[IX(x,y)] -= std::min( abs(maxWater - D[IX(x-1,y)]), std::min( abs(flux), abs(minWater - D[IX(x,y)]) ) );
//            D2[IX(x,y)] -= flux;
        }

        if( vy1 > 0 ) {
            flux = vy1 * dt * D[IX(x,y)] / 4;
            D2[IX(x,y)] -= std::min( abs(maxWater - D[IX(x,y)]), std::min( abs(flux), abs(minWater - D[IX(x,y-1)]) ) );
//            D2[IX(x,y)] -= flux;
        }
        else {
            flux = vy1 * dt * D[IX(x,y+1)] / 4;
            D2[IX(x,y)] += std::min( abs(maxWater - D[IX(x,y-1)]), std::min( abs(flux), abs(minWater - D[IX(x,y)]) ) );
        }

        if( vy2 > 0 ) {
            flux = vy2 * dt * D[IX(x,y-1)] / 4;
            D2[IX(x,y)] += std::min( abs(maxWater - D[IX(x,y+1)]), std::min( abs(flux), abs(minWater - D[IX(x,y)]) ) );
        }
        else {
            flux = vy2 * dt * D[IX(x,y)] / 4;
            D2[IX(x,y)] -= std::min( abs(maxWater - D[IX(x,y)]), std::min( abs(flux), abs(minWater - D[IX(x,y+1)]) ) );
        }
        
        
        
//
//        if( vx2 > 0 ) D2[IX(x,y)] += vx2 * dt * D[IX(x,y)] / 4;
//        else D2[IX(x,y)] -= vx2 * dt * D[IX(x-1,y)] / 4;
//
//        if( vy1 > 0 ) D2[IX(x,y)] -= vy1 * dt * D[IX(x,y)] / 4;
//        else D2[IX(x,y)] += vy1 * dt * D[IX(x,y-1)] / 4;
//
//        if( vy2 > 0 ) D2[IX(x,y)] += vy2 * dt * D[IX(x,y)] / 4;
//        else D2[IX(x,y)] -= vy2 * dt * D[IX(x,y+1)] / 4;
    }

    for(int i = 0; i < GRID_H*GRID_W; i++) D[i] = D2[i];
}










void addHeightDifference() {
    for(int y = 1; y < GRID_H - 1; y++) for(int x = 1; x < GRID_W - 1; x++){
        vec2 diffVelocity;
        diffVelocity.x = ( grid->water[IX(x-1,y)] - grid->water[IX(x+1,y)] )/2.f;
        diffVelocity.y = ( grid->water[IX(x,y-1)] - grid->water[IX(x,y+1)] )/2.f;
        grid->velocity[IX(x,y)] = (0.9f * grid->velocity[IX(x,y)]) + (0.1f * diffVelocity);
//        grid->velocity[IX(x,y)] = vec2(0,10);
    }
}

void BoudaryCondition() {
    for (int x = 1; x < GRID_H - 1; x++) for (int y = 1; y < GRID_W-1; y++) {
//        if( !grid->wetAreaMask[IX(x+1,y)] || !grid->wetAreaMask[IX(x-1,y)] || !grid->wetAreaMask[IX(x,y+1)] || !grid->wetAreaMask[IX(x,y-1)])
        if( !grid->wetAreaMask[IX(x,y)] )
            grid->velocity[IX(x,y)] = vec2(0,0);
//        if( !grid->wetAreaMask[IX(x,y)]) grid->velocity[IX(x,y)] = vec2(0,0);
    }
}

void flowOutward()
{
    const float eta = 0.001f;
    const int gaussian_radius = 10;
    
    for(int i = 0; i < GRID_H*GRID_W; i++) grid->wetAreaMaskTemp[i] = grid->wetAreaMask[i];
    
    fast_gaussian_blur(grid->wetAreaMaskTemp, grid->evaporation, GRID_W, GRID_H, gaussian_radius);
    
    for (int i = 1; i < GRID_W-1; i++) for (int j = 1; j < GRID_H-1; j++) {
        const float blur = eta * (1.0f - grid->evaporation[IX(i,j)]) * grid->wetAreaMask[IX(i,j)];
        grid->water[IX(i,j)] -= blur;
        if( grid->water[IX(i,j)] < 0) grid->water[IX(i,j)] = 0;
        if( !grid->water[IX(i,j)] && grid->wetAreaMask[IX(i,j)] ) grid->saturation[IX(i,j)] -= 0.01f;
        if( grid->saturation[IX(i,j)] < Sigma) {
//            grid->saturation[IX(i,j)] = 0;
            grid->wetAreaMask[IX(i,j)] = 0;
        }
    }
}



void SurfaceLayer(float dt) {
    float adsorb, desorb;
    //    for (auto i = 0; i < pigment.length; i++)
    //    {
    
    for (int x = 1; x < GRID_W - 1; x++) for (int y = 1; y < GRID_H - 1; y++) {
        if ( !grid->wetAreaMask[IX(x,y)] )
            continue;
        adsorb =
        grid->pigment[IX(x,y)] * ( 1.0f - grid->heightMap[IX(x,y)]  * Granule ) * Density;
        desorb =
        grid->pigmentDeposit[IX(x,y)] * ( 1.0f - (1.0f - grid->heightMap[IX(x,y)]) * Granule ) * Density / Staining;
        
//        adsorb =
//        grid->pigment[IX(x,y)] * ( 1.0f - grid->water[IX(x,y)] ) * ( 1.0f - grid->heightMap[IX(x,y)] * Granule ) * Density;
//        desorb =
//        grid->pigmentDeposit[IX(x,y)] * grid->water[IX(x,y)] * ( 1.0f - (1.0f - grid->heightMap[IX(x,y)]) * Granule ) * Density / Staining;
        
        if ((grid->pigmentDeposit[IX(x,y)] + adsorb) > 1.0f)
            adsorb = std::max(0.0f, 1.0f - grid->pigmentDeposit[IX(x,y)]);
        if ((grid->pigment[IX(x,y)] + desorb) > 1.0f)
            desorb = std::max(0.0f, 1.0f - grid->pigment[IX(x,y)]);
        grid->pigmentDeposit[IX(x,y)] += adsorb - desorb;
        grid->pigment[IX(x,y)] += desorb - adsorb;
    }
    
    
    //    }
}

void CapillaryLayer() {
    for( auto y = 0; y < GRID_H; y++ ) for( auto x = 0; x < GRID_W; x++ ){
        if( grid->wetAreaMask[IX(x,y)] ) {
            float absorbedWater = std::max( 0.0f, std::min( Alpha, grid->capacity[IX(x,y)] - grid->saturation[IX(x,y)] ) );
            if( grid->water[IX(x,y)] < absorbedWater ) absorbedWater = grid->water[IX(x,y)];
            grid->saturation[IX(x,y)] += absorbedWater;
            grid->water[IX(x,y)] -= absorbedWater;
        }
    }
    for(int i = 0; i < GRID_H*GRID_W; i++) grid->saturationTemp[i] = grid->saturation[i];
    
    for( auto y = 1; y < GRID_H - 1; y++ ) for( auto x = 1; x < GRID_W - 1; x++ ) {
        float deltaX1(0), deltaX2(0), deltaY1(0), deltaY2(0);
        if( grid->saturation[IX(x,y)] > Epsilon && grid->saturation[IX(x,y)] > grid->saturation[IX(x,y+1)]) {
            deltaY1 =
            std::max(0.0f,std::min(grid->saturation[IX(x,y)] - grid->saturation[IX(x,y+1)], grid->capacity[IX(x,y+1)] - grid->saturation[IX(x,y+1)]) / 4.0f);
            grid->saturationTemp[IX(x,y)] -= deltaY1;
            grid->saturationTemp[IX(x,y+1)] += deltaY1;
        }
        if( grid->saturation[IX(x,y)] > Epsilon && grid->saturation[IX(x,y)] > grid->saturation[IX(x,y-1)]) {
            deltaY2 =
            std::max(0.0f,std::min(grid->saturation[IX(x,y)] - grid->saturation[IX(x,y-1)], grid->capacity[IX(x,y-1)] - grid->saturation[IX(x,y-1)]) / 4.0f);
            grid->saturationTemp[IX(x,y)] -= deltaY2;
            grid->saturationTemp[IX(x,y-1)] += deltaY2;
        }
        if( grid->saturation[IX(x,y)] > Epsilon && grid->saturation[IX(x,y)] > grid->saturation[IX(x+1,y)]) {
            deltaX1 =
            std::max(0.0f,std::min(grid->saturation[IX(x,y)] - grid->saturation[IX(x+1,y)], grid->capacity[IX(x+1,y)] - grid->saturation[IX(x+1,y)]) / 4.0f);
            grid->saturationTemp[IX(x,y)] -= deltaX1;
            grid->saturationTemp[IX(x+1,y)] += deltaX1;
        }
        if( grid->saturation[IX(x,y)] > Epsilon && grid->saturation[IX(x,y)] > grid->saturation[IX(x-1,y)]) {
            deltaX2 =
            std::max(0.0f,std::min(grid->saturation[IX(x,y)] - grid->saturation[IX(x-1,y)], grid->capacity[IX(x-1,y)] - grid->saturation[IX(x-1,y)]) / 4.0f);
            grid->saturationTemp[IX(x,y)] -= deltaX2;
            grid->saturationTemp[IX(x-1,y)] += deltaX2;
        }
//        grid->velocity[IX(x,y)] =  10.f*vec2(deltaX1 - deltaX2, deltaY1 - deltaY2);
        
    }
        
//        for (int yy = y - 1; yy < y + 2; yy += 2) {
//            if( grid->saturation[IX(x,y)] > Epsilon && grid->saturation[IX(x,y)] > grid->saturation[IX(x,yy)]) {
//                float deltaS =
//                std::max(0.0f,std::min(grid->saturation[IX(x,y)] - grid->saturation[IX(x,yy)], grid->capacity[IX(x,yy)] - grid->saturation[IX(x,yy)]) / 4.0f);
//                grid->saturationTemp[IX(x,y)] -= deltaS;
//                grid->saturationTemp[IX(x,yy)] += deltaS;
//            }
//        }
//        for (int xx = x - 1; xx < x + 2; xx += 2) {
//            if( grid->saturation[IX(x,y)] > Epsilon && grid->saturation[IX(x,y)] > grid->saturation[IX(xx,y)]) {
//                float deltaS =
//                std::max(0.0f,std::min(grid->saturation[IX(x,y)] - grid->saturation[IX(xx,y)], grid->capacity[IX(xx,y)] - grid->saturation[IX(xx,y)]) / 4.0f);
//                grid->saturationTemp[IX(x,y)] -= deltaS;
//                grid->saturationTemp[IX(xx,y)] += deltaS;
//            }
//        }
    
    
    for(int i = 0; i < GRID_H*GRID_W; i++) grid->saturation[i] = grid->saturationTemp[i];
    
    for( auto y = 0; y < GRID_H; y++ ) for( auto x = 0; x < GRID_W; x++ ){
        if( grid->saturation[IX(x,y)] > Sigma ) grid->wetAreaMask[IX(x,y)] = 1;
    }
}










void UpdateVelocity(float dt) {
    diffuse( kappaV, grid->velocity, grid->wetAreaMask, dt );
    advection( grid->velocity, grid->velocityTemp, grid->velocity, dt );
    addHeightDifference();
    BoudaryCondition();
}

void UpdateWater(float dt) {
    diffuse( kappaW, grid->water, grid->wetAreaMask, dt );
//    advection( grid->water, grid->waterTemp, grid->velocity, dt );
    waterAdvection( grid->water, grid->waterTemp, grid->velocity, grid->wetAreaMask, Speed*dt );
    flowOutward();
}

void UpdatePigment(float dt) {
    diffuse( kappaP, grid->pigment, grid->wetAreaMask, dt );
    waterAdvection( grid->pigment, grid->pigmentTemp, grid->velocity, grid->wetAreaMask, Speed*dt );
//    advection( grid->pigment, grid->pigmentTemp, grid->velocity, dt );
}








void frame(float dt) {
    if( pressed ){
        for(int y = 1; y < GRID_H - 1; y++) for(int x = 1; x < GRID_W - 1; x++) {
            if( sqrt( (mousePt.x-x)*(mousePt.x-x) + (mousePt.y-y)*(mousePt.y-y) ) < radius){
//                grid->velocity[IX(x,y)] += 0.01f * vec2(mousePt-lastPt);
//                grid->water[IX(x,y)] += quantityW;
//                grid->pigment[IX(x,y)] += quantityP;
//                if(grid->water[IX(x,y)] < maxWater - quantityW){
                    grid->water[IX(x,y)] = quantityW;
                    grid->saturation[IX(x,y)] = Sigma;
                    grid->wetAreaMask[IX(x,y)] = 1;
//                }
//                grid->water[IX(x,y)] += quantityW;
//                grid->saturation[IX(x,y)] = Sigma;
//                grid->wetAreaMask[IX(x,y)] = 1;
//                grid->water[IX(x,y)] = quantityW;
//                grid->saturation[IX(x,y)] = Sigma;
//                grid->wetAreaMask[IX(x,y)] = 1;
//                if( grid->pigment[IX(x,y)] < maxPig - quantityP )
                    grid->pigment[IX(x,y)] = quantityP;
//                grid->wetAreaMask[IX(x,y)] = 1;
                
            }
        }
    }
    
    if( play ){
        UpdateVelocity(dt);
        UpdateWater(dt);
        UpdatePigment(dt);
        SurfaceLayer(dt);
        CapillaryLayer();
    }
    
    for(int y = 0; y < GRID_H; y++) for(int x = 0; x < GRID_W; x++){
        
        if(flag == 0){
            grid->R[3*x + 0 + 3*y*GRID_W] = ((grid->pigmentDeposit[IX(x,y)]+grid->pigment[IX(x,y)])) * pigment.colorW.r + (1-(grid->pigmentDeposit[IX(x,y)] + grid->pigment[IX(x,y)])) * grid->paper[IX(x,y)];
            grid->R[3*x + 1 + 3*y*GRID_W] = ((grid->pigmentDeposit[IX(x,y)]+grid->pigment[IX(x,y)])) * pigment.colorW.g + (1-(grid->pigmentDeposit[IX(x,y)] + grid->pigment[IX(x,y)])) * grid->paper[IX(x,y)];
            grid->R[3*x + 2 + 3*y*GRID_W] = ((grid->pigmentDeposit[IX(x,y)]+grid->pigment[IX(x,y)])) * pigment.colorW.b + (1-(grid->pigmentDeposit[IX(x,y)] + grid->pigment[IX(x,y)])) * grid->paper[IX(x,y)];
        }
        else if(flag == 1){
            grid->R[3*x + 0 + 3*y*GRID_W] = grid->water[IX(x,y)];
            grid->R[3*x + 1 + 3*y*GRID_W] = grid->water[IX(x,y)];
            grid->R[3*x + 2 + 3*y*GRID_W] = grid->water[IX(x,y)];
        }
        else if(flag == 2){
            grid->R[3*x + 0 + 3*y*GRID_W] = grid->saturation[IX(x,y)];
            grid->R[3*x + 1 + 3*y*GRID_W] = grid->saturation[IX(x,y)];
            grid->R[3*x + 2 + 3*y*GRID_W] = grid->saturation[IX(x,y)];
        }
        else if(flag == 3){
            grid->R[3*x + 0 + 3*y*GRID_W] = grid->velocity[IX(x,y)].x;
            grid->R[3*x + 1 + 3*y*GRID_W] = grid->velocity[IX(x,y)].x;
            grid->R[3*x + 2 + 3*y*GRID_W] = grid->velocity[IX(x,y)].x;
        }
        else if(flag == 4){
            grid->R[3*x + 0 + 3*y*GRID_W] = grid->wetAreaMask[IX(x,y)];
            grid->R[3*x + 1 + 3*y*GRID_W] = grid->wetAreaMask[IX(x,y)];
            grid->R[3*x + 2 + 3*y*GRID_W] = grid->wetAreaMask[IX(x,y)];
        }
        else if(flag == 5){
            grid->R[3*x + 0 + 3*y*GRID_W] = grid->evaporation[IX(x,y)];
            grid->R[3*x + 1 + 3*y*GRID_W] = grid->evaporation[IX(x,y)];
            grid->R[3*x + 2 + 3*y*GRID_W] = grid->evaporation[IX(x,y)];
        }
        if(flag == 6){
            grid->R[3*x + 0 + 3*y*GRID_W] = (grid->pigmentDeposit[IX(x,y)]) * pigment.colorW.r + (1 - grid->pigmentDeposit[IX(x,y)]) * grid->paper[IX(x,y)];
            grid->R[3*x + 1 + 3*y*GRID_W] = (grid->pigmentDeposit[IX(x,y)]) * pigment.colorW.g + (1 - grid->pigmentDeposit[IX(x,y)]) * grid->paper[IX(x,y)];
            grid->R[3*x + 2 + 3*y*GRID_W] = (grid->pigmentDeposit[IX(x,y)]) * pigment.colorW.b + (1 - grid->pigmentDeposit[IX(x,y)]) * grid->paper[IX(x,y)];
        }
        if(flag == 7){
            grid->R[3*x + 0 + 3*y*GRID_W] = (grid->pigment[IX(x,y)]) * pigment.colorW.r + (1 - grid->pigment[IX(x,y)]) * grid->paper[IX(x,y)];
            grid->R[3*x + 1 + 3*y*GRID_W] = (grid->pigment[IX(x,y)]) * pigment.colorW.g + (1 - grid->pigment[IX(x,y)]) * grid->paper[IX(x,y)];
            grid->R[3*x + 2 + 3*y*GRID_W] = (grid->pigment[IX(x,y)]) * pigment.colorW.b + (1 - grid->pigment[IX(x,y)]) * grid->paper[IX(x,y)];
        }
//        grid->R[3*x + 0 + 3*y*GRID_W] += 0.2*(grid->water[IX(x,y)] + grid->saturation[IX(x,y)]);
//        grid->R[3*x + 1 + 3*y*GRID_W] += 0.2*(grid->water[IX(x,y)] + grid->saturation[IX(x,y)]);
//        grid->R[3*x + 2 + 3*y*GRID_W] += 0.2*(grid->water[IX(x,y)] + grid->saturation[IX(x,y)]);
        
        
//                grid->R[3*x + 0 + 3*y*GRID_W] = grid->pigment[IX(x,y)] * pigment.colorW.r + (1 - grid->pigment[IX(x,y)]) * grid->paper[IX(x,y)];
//                grid->R[3*x + 1 + 3*y*GRID_W] = grid->pigment[IX(x,y)] * pigment.colorW.g + (1 - grid->pigment[IX(x,y)]) * grid->paper[IX(x,y)];
//                grid->R[3*x + 2 + 3*y*GRID_W] = grid->pigment[IX(x,y)] * pigment.colorW.b + (1 - grid->pigment[IX(x,y)]) * grid->paper[IX(x,y)];
//        grid->R[3*x + 0 + 3*y*GRID_W] = grid->pigmentDeposit[IX(x,y)] * pigment.colorW.r + (1 - grid->pigmentDeposit[IX(x,y)]) * grid->paper[IX(x,y)];
//        grid->R[3*x + 1 + 3*y*GRID_W] = grid->pigmentDeposit[IX(x,y)] * pigment.colorW.g + (1 - grid->pigmentDeposit[IX(x,y)]) * grid->paper[IX(x,y)];
//        grid->R[3*x + 2 + 3*y*GRID_W] = grid->pigmentDeposit[IX(x,y)] * pigment.colorW.b + (1 - grid->pigmentDeposit[IX(x,y)]) * grid->paper[IX(x,y)];
//        grid->R[3*x + 0 + 3*y*GRID_W] = grid->water[IX(x,y)] + grid->saturation[IX(x,y)];
//        grid->R[3*x + 1 + 3*y*GRID_W] = grid->water[IX(x,y)] + grid->saturation[IX(x,y)];
//        grid->R[3*x + 2 + 3*y*GRID_W] = grid->water[IX(x,y)] + grid->saturation[IX(x,y)];
//        grid->R[3*x + 0 + 3*y*GRID_W] = grid->wetAreaMask[IX(x,y)];
//        grid->R[3*x + 1 + 3*y*GRID_W] = grid->wetAreaMask[IX(x,y)];
//        grid->R[3*x + 2 + 3*y*GRID_W] = grid->wetAreaMask[IX(x,y)];
//        grid->R[3*x + 0 + 3*y*GRID_W] = grid->velocity[IX(x,y)].x;// + grid->velocity[IX(x,y)].y;
//        grid->R[3*x + 1 + 3*y*GRID_W] = grid->water[IX(x,y)];// + grid->velocity[IX(x,y)].y;
//        grid->R[3*x + 2 + 3*y*GRID_W] = 0;// + grid->velocity[IX(x,y)].y;
    }
}

void push( float x, float y ) {
    mousePt = glm::ivec2( x*GRID_W, y*GRID_H );
    pressed = true;
}

void release() {
    pressed = false;
}

void move1( float x, float y ) {
    lastPt = mousePt;
    mousePt = ivec2( x*GRID_W, y*GRID_H );
}

void drag( float x, float y ) {
    mousePt = glm::ivec2( x*GRID_W, y*GRID_H );
}

void update(Tex& R) {
    R.create(GRID_W,GRID_H,GL_RGB,GL_FLOAT, grid->R );
    //    R.create(GRID_W,GRID_H,GL_RED,GL_FLOAT, grid->water );
}

void colorSelect( int i ) {
    switch (i) {
        case 1:
            flag = 0;
//            pigment.QuinacridoneMagenta(); // Quinacridone Magenta
            break;
        case 2:
//            pigment.IndianRed();
            flag = 1;
            break;
        case 3:
            flag = 2;
//            pigment.CadmiumYellow();
            break;
        case 4:
            flag = 3;
//            pigment.HookersGreen();
            break;
        case 5:
            flag = 4;
//            pigment.CeruleanBlue();
            break;
        case 6:
            flag = 5;
//            pigment.BurntUmber();
            break;
        case 7:
            flag = 6;
//            pigment.CadmiumRed();
            break;
        case 8:
            flag = 7;
//            pigment.InterferenceLilac();
            break;
        case 9:
            pigment.FrenchUltramarine();
            break;
        default:
            pigment.IndianRed();
            break;
    }
    //    for(auto i = 0; i < GRID_W * GRID_H; i++){
    //        grid->colorChangedMask[i] = false;
    //    }
}

void setRadius(int i) {
    if (i == 1) radius++;
    else if (i == 2) {
        if(radius > 2) radius--;
    }
}
void Simulation() {
    play = play == 1 ? 0 : 1;
    if(play)
        cout << "On" << endl;
    else
        cout << "Off" << endl;
    float min;
    for(int i = 0; i < GRID_W*GRID_H; i++){
//        if(min > grid->water[i]) min = grid->water[i];
        min += grid->pigment[i];
    }
    cout << min << endl;
}

int main(int argc, const char * argv[]) {
    
    grid = new Grid(GRID_W, GRID_H);
    
    init();
    
    window = new JGL::Window(1024+200,1024,"WaterColor");
    UIinit();

    view->initFunction = init;
    view->updateFunction = update;
    view->frameFunction = frame;
    view->dragFunction = drag;
    view->moveFunction = move1;
    view->pushFunction = push;
    view->releaseFunction = release;
    view->colorFunction = colorSelect;
    view->radiusFunction = setRadius;
    view->playFunction = Simulation;
//    window->end();
    window->show();
    
    JGL::_JGL::run();
    return 0;
}
