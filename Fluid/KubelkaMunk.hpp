//
//  KubelkaMunk.hpp
//  Fluid
//
//  Created by 최종원 on 2023/01/30.
//

#ifndef KubelkaMunk_hpp
#define KubelkaMunk_hpp
#include <JGL/JGL.hpp>
#include <stdio.h>

struct PigmentInfo {
    glm::vec3 colorB;
    glm::vec3 colorW;
    glm::vec3 color;
    glm::vec3 K;
    glm::vec3 S;

    void QuinacridoneMagenta() {
        colorB = glm::vec3(30/255.f, 5/255.f, 15/255.f);
        colorW = glm::vec3(140/255.f, 40/255.f, 70/255.f);
        K.r = 0.22f;
        K.g = 1.47f;
        K.b = 0.57f;
        S.r = 0.05f;
        S.g = 0.003f;
        S.b = 0.03f;
    }
    void IndianRed() {
        colorB = glm::vec3(120/255.f, 60/255.f, 40/255.f);
        colorW = glm::vec3(130/255.f, 70/255.f, 50/255.f);
        K.r = 0.46f;
        K.g = 1.07f;
        K.b = 1.50f;
        S.r = 1.28f;
        S.g = 0.38f;
        S.b = 0.21f;
    }
    void CadmiumYellow() {
        colorB = glm::vec3(140/255.f, 100/255.f, 40/255.f);
        colorW = glm::vec3(180/255.f, 140/255.f, 50/255.f);
        K.r = 0.10f;
        K.g = 0.36f;
        K.b = 3.45f;
        S.r = 0.97f;
        S.g = 0.65f;
        S.b = 0.007f;
    }
    void HookersGreen() {
        colorB = glm::vec3(2/255.f, 5/255.f, 1/255.f);
        colorW = glm::vec3(30/255.f, 80/255.f, 25/255.f);
        K.r = 1.62f;
        K.g = 0.61f;
        K.b = 1.64f;
        S.r = 0.01f;
        S.g = 0.012f;
        S.b = 0.003f;
    }
    void CeruleanBlue() {
        colorB = glm::vec3(30/255.f, 70/255.f, 90/255.f);
        colorW = glm::vec3(50/255.f, 120/255.f, 130/255.f);
        K.r = 1.52f;
        K.g = 0.32f;
        K.b = 0.25f;
        S.r = 0.06f;
        S.g = 0.26f;
        S.b = 0.40f;
    }
    void BurntUmber() {
        colorB = glm::vec3(25/255.f, 10/255.f, 1/255.f);
        colorW = glm::vec3(70/255.f, 30/255.f, 15/255.f);
        K.r = 0.74f;
        K.g = 1.54f;
        K.b = 2.10f;
        S.r = 0.09f;
        S.g = 0.004f;
        S.b = 0.09f;
    }
    void CadmiumRed() {
        colorB = glm::vec3(120/255.f, 30/255.f, 15/255.f);
        colorW = glm::vec3(160/255.f, 50/255.f, 25/255.f);
        K.r = 0.14f;
        K.g = 1.08f;
        K.b = 1.68f;
        S.r = 0.77f;
        S.g = 0.015f;
        S.b = 0.018f;
    }
    void InterferenceLilac() {
        colorB = glm::vec3(140/255.f, 100/255.f, 150/255.f);
        colorW = glm::vec3(190/255.f, 180/255.f, 190/255.f);
        K.r = 0.08f;
        K.g = 0.11f;
        K.b = 0.07f;
        S.r = 1.25f;
        S.g = 0.42f;
        S.b = 1.43f;
    }
    void FrenchUltramarine() {
        colorB = glm::vec3(10/255.f, 10/255.f, 40/255.f);
        colorW = glm::vec3(30/255.f, 30/255.f, 180/255.f);
        K.r = 0.86f;
        K.g = 0.86f;
        K.b = 0.06f;
        S.r = 0.005f;
        S.g = 0.005f;
        S.b = 0.09f;
    }
};

struct PixelInfo {
    std::vector<PigmentInfo> pigment;
    glm::vec3 reflectance;
    glm::vec3 RR;
    glm::vec3 TT;
    
    void AddPigment(PigmentInfo p) {
        pigment.push_back(p);
    }
    
    glm::vec3 GetA(glm::vec3 Rw, glm::vec3 Rb) {
        return glm::vec3(1/2.f * (Rw.r + (Rb.r - Rw.r + 1) / Rb.r),
                         1/2.f * (Rw.g + (Rb.g - Rw.g + 1) / Rb.g),
                         1/2.f * (Rw.b + (Rb.b - Rw.b + 1) / Rb.b));
    }
    glm::vec3 GetB(glm::vec3 a) {
        return glm::vec3(std::sqrt(a.r * a.r - 1),
                         std::sqrt(a.g * a.g - 1),
                         std::sqrt(a.b * a.b - 1));
    }
    glm::vec3 GetC(glm::vec3 a, glm::vec3 b, glm::vec3 S, float x) {
        return glm::vec3(a.r * std::sinh(b.r * S.r * x) + b.r * std::cosh(b.r * S.r * x),
                         a.g * std::sinh(b.g * S.g * x) + b.g * std::cosh(b.g * S.g * x),
                         a.b * std::sinh(b.b * S.b * x) + b.b * std::cosh(b.b * S.b * x));
    }
    glm::vec3 GetR(glm::vec3 b, glm::vec3 c, glm::vec3 S, float x){
        return glm::vec3(std::sinh(b.r * S.r * x / c.r),
                         std::sinh(b.g * S.g * x / c.g),
                         std::sinh(b.b * S.b * x / c.b));
    }
    glm::vec3 GetT(glm::vec3 b, glm::vec3 c){
        return glm::vec3(b.r / c.r,
                         b.g / c.g,
                         b.b / c.b);
    }
    glm::vec3 MixR(glm::vec3 R1, glm::vec3 R2, glm::vec3 T1) {
        return glm::vec3(R1.r + T1.r * T1.r * R2.r / (1 - R1.r * R2.r),
                         R1.g + T1.g * T1.g * R2.g / (1 - R1.g * R2.g),
                         R1.b + T1.b * T1.b * R2.b / (1 - R1.b * R2.b));
    }
    glm::vec3 MixT(glm::vec3 R1, glm::vec3 R2, glm::vec3 T1, glm::vec3 T2) {
        return glm::vec3(T1.r * T2.r / (1 - R1.r * R2.r),
                         T1.g * T2.g / (1 - R1.g * R2.g),
                         T1.b * T2.b / (1 - R1.b * R2.b));
    }
//
//    void GetReflectance(bool isBlack){
    void GetReflectance(){
        if(pigment.size() == 0) return;
        else if(pigment.size() == 1) {
//            if(isBlack) reflectance = pigment[0].colorB;
//            else reflectance = pigment[0].colorW;
            reflectance = pigment[0].colorW;
        }
        else {
            int index = pigment.size() - 1;
            while( index > 0){
                glm::vec3 R,T, R1, R2, T1, T2, a, b, c;
                if(index == pigment.size() - 1) {
                    a = GetA(pigment[index].colorW, pigment[index].colorB);
                    b = GetB(a);
                    c = GetC(a, b, pigment[index].S, 1);
                    R1 = GetR(b, c, pigment[index].S, 1);
                    T1 = GetT(b,c);
                }
                else {
                    R1 = RR;
                    T1 = TT;
                }

                a = GetA(pigment[index-1].colorW, pigment[index-1].colorB);
                b = GetB(a);
                c = GetC(a, b, pigment[index-1].S, 1);
                R2 = GetR(b, c, pigment[index-1].S, 1);
                T2 = GetT(b,c);

                R = MixR(R1, R2, T1);
                T = MixT(R1, R2, T1, T2);
                
                RR = R;
                TT = T;
                
                reflectance = R;
                
                index--;
            }
        }
    }
    
    //    float CalcA(float channel) {
    //        if(channel == 0) channel = 0.0001f;
    //        return ((1.f - channel) * (1.f - channel)) / (2.f * channel);
    //    }
    //    void GetR(){
    //        if(pigment.size() == 0) return;
    //        else {
    //            float Ar = 0;
    //            float Ag = 0;
    //            float Ab = 0;
    //            float concentration = 1;
    //
    //            for(int i = pigment.size() - 1; i >= 0; i--){
    //                Ar += CalcA(pigment[i].colorW.r) * concentration;
    //                Ag += CalcA(pigment[i].colorW.g) * concentration;
    //                Ab += CalcA(pigment[i].colorW.b) * concentration;
    //            }
    //            reflectance = glm::vec3(1.f + Ar - std::sqrt( Ar*Ar + 2*Ar ),
    //                                    1.f + Ag - std::sqrt( Ag*Ag + 2*Ag ),
    //                                    1.f + Ab - std::sqrt( Ab*Ab + 2*Ab ));
    //        }
    //    }
};
#endif /* KubelkaMunk_hpp */
