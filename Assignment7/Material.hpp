//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_MATERIAL_H
#define RAYTRACING_MATERIAL_H

#include "Vector.hpp"
#include <fstream>

enum MaterialType { DIFFUSE, MICROFACET};

class Material{
private:

    // Compute reflection direction
    Vector3f reflect(const Vector3f &I, const Vector3f &N) const
    {
        return I - 2 * dotProduct(I, N) * N;
    }

    // Compute refraction direction using Snell's law
    //
    // We need to handle with care the two possible situations:
    //
    //    - When the ray is inside the object
    //
    //    - When the ray is outside.
    //
    // If the ray is outside, you need to make cosi positive cosi = -N.I
    //
    // If the ray is inside, you need to invert the refractive indices and negate the normal N
    Vector3f refract(const Vector3f &I, const Vector3f &N, const float &ior) const
    {
        float cosi = clamp(-1, 1, dotProduct(I, N));
        float etai = 1, etat = ior;
        Vector3f n = N;
        if (cosi < 0) { cosi = -cosi; } else { std::swap(etai, etat); n= -N; }
        float eta = etai / etat;
        float k = 1 - eta * eta * (1 - cosi * cosi);
        return k < 0 ? 0 : eta * I + (eta * cosi - sqrtf(k)) * n;
    }

    // Compute Fresnel equation
    //
    // \param I is the incident view direction
    //
    // \param N is the normal at the intersection point
    //
    // \param ior is the material refractive index
    //
    // \param[out] kr is the amount of light reflected
    void fresnel(const Vector3f &I, const Vector3f &N, const float &ior, float &kr) const
    {
        float cosi = clamp(-1, 1, dotProduct(I, N));
        float etai = 1, etat = ior;
        if (cosi > 0) {  std::swap(etai, etat); }
        // Compute sini using Snell's law
        float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
        // Total internal reflection
        if (sint >= 1) {
            kr = 1;
        }
        else {
            float cost = sqrtf(std::max(0.f, 1 - sint * sint));
            cosi = fabsf(cosi);
            float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
            float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
            kr = (Rs * Rs + Rp * Rp) / 2;
        }
        // As a consequence of the conservation of energy, transmittance is given by:
        // kt = 1 - kr;
    }

    Vector3f toWorld(const Vector3f &a, const Vector3f &N){
        Vector3f B, C;
        if (std::fabs(N.x) > std::fabs(N.y)){
            float invLen = 1.0f / std::sqrt(N.x * N.x + N.z * N.z);
            C = Vector3f(N.z * invLen, 0.0f, -N.x *invLen);
        }
        else {
            float invLen = 1.0f / std::sqrt(N.y * N.y + N.z * N.z);
            C = Vector3f(0.0f, N.z * invLen, -N.y *invLen);
        }
        B = crossProduct(C, N);
        return a.x * B + a.y * C + a.z * N;
    }

    // add by skc begin
    // belows are for MICROFACET material

    // GGX Trowbridge-Reitz function
    float D_GGX(Vector3f N, Vector3f h, float alpha) {
        float NdotH = std::max(dotProduct(N, h), 0.000001f);
        float alpha2 = alpha * alpha;
        float denom = NdotH * NdotH * (alpha2 - 1.0f) + 1.0f;
        return alpha2 / (M_PI * denom * denom);
    }

    float G1_GGX(Vector3f N, Vector3f v, float alpha) {
        float NdotV = std::max(dotProduct(N, v), 0.00001f);
        float alpha2 = alpha * alpha;
        float term = sqrt(alpha2 + (1 - alpha2) * NdotV * NdotV);
        return 2.0f * NdotV / (NdotV + term);
        // float k = (alpha + 1.0f) * (alpha + 1.0f) / 8;
        // float NdotV = std::max(dotProduct(N, v), 0.00001f);
        // return NdotV / (NdotV * (1-k) + k);
    }

    // Smith GGX
    float G_Smith_GGX(Vector3f N, Vector3f wi, Vector3f wo, float alpha) {
        float g1i = G1_GGX(N, wi, alpha);
        float g1o = G1_GGX(N, wo, alpha);
        return g1i * g1o;
    }
    // add by skc end

public:
    MaterialType m_type;
    //Vector3f m_color;
    Vector3f m_emission;
    float ior;
    Vector3f Kd, Ks;
    float specularExponent;
    float alpha; // degree of roughness
    //Texture tex;

    bool debug_once = false;

    inline Material(MaterialType t=DIFFUSE, Vector3f e=Vector3f(0,0,0));
    inline MaterialType getType();
    //inline Vector3f getColor();
    inline Vector3f getColorAt(double u, double v);
    inline Vector3f getEmission();
    inline bool hasEmission();

    // sample a ray by Material properties
    inline Vector3f sample(const Vector3f &wi, const Vector3f &N);
    // given a ray, calculate the PdF of this ray
    inline float pdf(const Vector3f &wi, const Vector3f &wo, const Vector3f &N);
    // given a ray, calculate the contribution of this ray
    inline Vector3f eval(const Vector3f &wi, const Vector3f &wo, const Vector3f &N, std::ofstream& out_file);

};

Material::Material(MaterialType t, Vector3f e){
    m_type = t;
    //m_color = c;
    m_emission = e;
}

MaterialType Material::getType(){return m_type;}
///Vector3f Material::getColor(){return m_color;}
Vector3f Material::getEmission() {return m_emission;}
bool Material::hasEmission() {
    if (m_emission.norm() > EPSILON) return true;
    else return false;
}

Vector3f Material::getColorAt(double u, double v) {
    return Vector3f();
}


Vector3f Material::sample(const Vector3f &wi, const Vector3f &N){
    switch(m_type){
        case MICROFACET:
        case DIFFUSE:
        {
            // uniform sample on the hemisphere
            float x_1 = get_random_float(), x_2 = get_random_float();
            float z = std::fabs(1.0f - 2.0f * x_1);
            float r = std::sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
            Vector3f localRay(r*std::cos(phi), r*std::sin(phi), z);
            return toWorld(localRay, N); 
            break;
        }
    }
}

float Material::pdf(const Vector3f &wi, const Vector3f &wo, const Vector3f &N){
    switch(m_type){
        case MICROFACET:
        case DIFFUSE:
        {
            // uniform sample probability 1 / (2 * PI)
            if (dotProduct(wo, N) > 0.0f)
                return 0.5f / M_PI;
            else
                return 0.0f;
            break;
        }
    }
}

Vector3f Material::eval(const Vector3f &wi, const Vector3f &wo, const Vector3f &N, std::ofstream& out_file){
    switch(m_type){
        case DIFFUSE:
        {
            // calculate the contribution of diffuse   model
            // if (debug_once) {
            //     out_file << "eval DIFFUSE wo: " << wo << " dotProduct(N, wo): " << dotProduct(N, wo) << std::endl;
            // }
            float cosalpha = dotProduct(N, wo);
            if (cosalpha > 0.0f) {
                Vector3f diffuse = Kd / M_PI;
                return diffuse;
            }
            else
                return Vector3f(0.0f);
            break;
        }
        case MICROFACET:
        {
            // need initialized with ior, alpha
            // Vector3f new_wo = -1.0f * wo;
            Vector3f new_wo = wo;
            Vector3f half_vector = (wi + new_wo).normalized();
            Vector3f res(0.0f);


            float fresnel_kr = 0.0f;
            fresnel(-1.0f*wi, N, ior, fresnel_kr);
            float d = D_GGX(N, half_vector, alpha);
            float G = G_Smith_GGX(N, wi, new_wo, alpha);
            float NdotI = std::max(dotProduct(N, wi), 0.00001f);
            float NdotO = std::max(dotProduct(N, new_wo), 0.00001f);
            res = fresnel_kr * G * d / 4.0f / NdotI / NdotO;
            if (debug_once) {
                out_file << "eval MICROFACET wo: " << wo << " dotProduct(N, wo): " << dotProduct(N, wo)  << "  dotProduct(N, wi): " << dotProduct(N, wi) << std::endl;
                out_file << "eval MICROFACET fresnel_kr: " <<  fresnel_kr << " d: " << d << " G:" << G << 
                " NdotI:" << NdotI  << " NdotO: " << NdotO  << " res: " << res << 
                "  Kd/M_PI*(Vector3f(1.0f) - fresnel_kr): " << Kd/M_PI*(Vector3f(1.0f) - fresnel_kr) <<std::endl;
            }
        
            return Kd/M_PI*(Vector3f(1.0f) - fresnel_kr) +  res;
        }
    }
}

#endif //RAYTRACING_MATERIAL_H
