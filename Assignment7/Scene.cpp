//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"
#include <fstream>


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth, std::ofstream& out_file) const
{
    // TO DO Implement Path Tracing Algorithm here
    Vector3f res(0);

    // prevent infinite recursive
    // if (depth > 10) return res;
    // RussianRoulette
    float Russian_float = get_random_float();

    if (debug) {
        out_file << "castRay depth: " << depth << " Russian_float: " << Russian_float << std::endl;
    }

    

    Intersection intersect = this->bvh->Intersect(ray);

    if (debug) {
        out_file << " intersect.happened: " << intersect.happened << std::endl;
    }
    
    if (intersect.happened) {
        Vector3f L0_light(0), L_diffuse(0);
        // 0. directly shoot on light
        // PS: is this case need to reflect diffuse ray? In real physics, no, this point will be the source of light
        if (intersect.m->hasEmission()) {
            return intersect.m->getEmission();
        }


        // 1.sample light and compute direct light contribution
        Intersection light_pos_intersect;
        float light_pdf;
        sampleLight(light_pos_intersect, light_pdf);

        if (debug) {
            out_file << "intersect.coords: " << intersect.coords << "  light_pos_intersect.coords: " << light_pos_intersect.coords 
                <<  " light_pdf: " << light_pdf << " material: " << intersect.m->getType() << std::endl;
        }

        // 1.1 compute light_pos vector
        Vector3f light_dir = light_pos_intersect.coords - intersect.coords;
        float light_distance = dotProduct(light_dir, light_dir);
        light_dir = light_dir.normalized();
        // 1.2 compute if this position is in shadow
        Vector3f shadowPointorig = (dotProduct(ray.direction, intersect.normal) < 0) ? 
                                    intersect.coords + intersect.normal * EPSILON :
                                    intersect.coords - intersect.normal * EPSILON ;

        Ray light_ray(shadowPointorig, light_dir);
        Intersection shadow_intersect = this->bvh->Intersect(light_ray);
        bool inshadow = false;
        float shadow_distance = 0.0f;
        float light_shadow_distance = 0.0f;
        if (shadow_intersect.happened) {
            Vector3f light_shadowPoint_dir = light_pos_intersect.coords - shadowPointorig;
            Vector3f shadow_dir = shadow_intersect.coords - intersect.coords;
            shadow_distance = dotProduct(shadow_dir, shadow_dir);
            light_shadow_distance = dotProduct(light_shadowPoint_dir, light_shadowPoint_dir);
            if (shadow_distance + 0.01f < light_shadow_distance) { 
                inshadow = true;
            }
        }
        if (debug) {
            out_file << "inshadow:  " << inshadow  << "  shadow_intersect.coords: " << shadow_intersect.coords 
                <<  " shadow_distance: " << shadow_distance << " light_distance: " << light_distance << std::endl;
        }
        // 1.3 if not in shadow, compute direct light value
        Vector3f brdf_light = intersect.m->eval(-1.0*ray.direction, light_dir, intersect.normal, out_file);
        if (!inshadow) {
            L0_light = light_pos_intersect.emit * brdf_light *
                    dotProduct(intersect.normal, light_dir) * dotProduct(light_pos_intersect.normal, -1.0f*light_dir) / light_distance / light_pdf;
        }

        if (debug) {
            out_file << "L0_light:  " << L0_light << std::endl;
            if (debug_detail) {
                out_file << "light_pos_intersect.emit:  " << light_pos_intersect.emit << "  eval: " << brdf_light
                    << "  dotProduct(intersect.normal, light_dir): " << dotProduct(intersect.normal, light_dir)
                    << "  dotProduct(light_pos_intersect.normal, -1.0f*light_dir): " << dotProduct(light_pos_intersect.normal, -1.0f*light_dir) 
                    << "  light_distance: "<< light_distance << std::endl;
            }
        }
        
        // 2.compute the diffuse ray value reflected by this point intersection which hit other objects
        if (Russian_float <= RussianRoulette) {
            Vector3f wo = intersect.m->sample(ray.direction, intersect.normal);
            wo = wo.normalized();
            Ray new_ray(intersect.coords, wo, 0);
            Intersection reflect_intersect = this->bvh->Intersect(new_ray);
            if (reflect_intersect.happened && !reflect_intersect.m->hasEmission()) {
                Vector3f brdf_diffuse = intersect.m->eval(-1.0*ray.direction, wo, intersect.normal, out_file);
                L_diffuse = castRay(new_ray, depth+1, out_file) * brdf_diffuse * dotProduct(wo, intersect.normal)
                    / intersect.m->pdf(ray.direction, wo, intersect.normal) / RussianRoulette;
            }
        }
        

        res = L0_light + L_diffuse;

        if (debug) {
            out_file << "depth: " << depth << "  L_diffuse:  " << L_diffuse << "  res: " << res <<std::endl;
        }
    }

    return res;
}