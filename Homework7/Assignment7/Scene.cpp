//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


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
Vector3f Scene::castRay(const Ray& ray, int depth) const
{
    // TO DO Implement Path Tracing Algorithm here
    Intersection isect = intersect(ray);
    if (!isect.happened)
        return Vector3f();

    Vector3f L_dir, L_indir;
    Intersection isect_light;
    float pdf_light;
    sampleLight(isect_light, pdf_light);

    Ray ray_p_x(isect.coords, (isect_light.coords - isect.coords).normalized());
    Intersection isect_p_x = intersect(ray_p_x);
    if (isect_p_x.happened && isect_p_x.m->hasEmission())
        L_dir = isect_p_x.m->getEmission() * isect.m->eval(ray.direction, ray_p_x.direction, isect.normal) * dotProduct(ray_p_x.direction, isect.normal) * dotProduct(-ray_p_x.direction, isect_p_x.normal) / isect_p_x.distance / isect_p_x.distance / pdf_light;

    if (get_random_float() < RussianRoulette) {
        Vector3f dir_in = isect.m->sample(ray.direction, isect.normal).normalized();
        Ray ray_in(isect.coords, dir_in);
        Intersection isect_in = intersect(ray_in);
        if (isect_in.happened && !isect_in.m->hasEmission())
            L_indir = castRay(ray_in, depth + 1) * isect.m->eval(ray.direction, dir_in, isect.normal) * dotProduct(dir_in, isect.normal) / isect.m->pdf(ray.direction, dir_in, isect.normal) / RussianRoulette;
    }
    return isect.m->getEmission() + L_dir + L_indir;
}
