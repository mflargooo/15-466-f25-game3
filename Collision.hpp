#pragma once

#include "glm/glm.hpp"
#include "Scene.hpp"

struct Collider {
    glm::vec3 size = glm::vec3(1.f);
    glm::vec3 offset = glm::vec3(0.f);

    glm::mat4x3 get_transformation_matrix();

    Collider(Scene::Transform *obj_transform) : obj_transform(obj_transform) { assert(obj_transform); };

    bool intersect(Collider other);
    // given a start position and end, if movement from "start" to "end" were to cross
    // the collider, clamp movement respectively, and account for collider width
    // updates the end position
    bool clip_movement(Collider other, glm::vec3 start, glm::vec3 *end);

    private:
        Scene::Transform *obj_transform;
};