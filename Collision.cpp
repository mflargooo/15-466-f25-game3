#include "Collision.hpp"
#include "DrawLines.hpp"

#include <iostream>

glm::mat4x3 Collider::get_transformation_matrix() {
    return obj_transform->make_world_from_local() *
        glm::mat4(
            size.x, 0.f, 0.f, offset.x,
            0.f, size.y, 0.f, offset.y,
            0.f, 0.f, size.z, offset.z,
            0.f, 0.f, 0.f, 1.f
        );
}

// https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection
bool Collider::intersect(Collider other) {
    glm::highp_vec3 min = obj_transform->position + offset - size;
    glm::highp_vec3 max = obj_transform->position + offset + size;

    glm::highp_vec3 other_min = other.obj_transform->position + other.offset - other.size;
    glm::highp_vec3 other_max = other.obj_transform->position + other.offset + other.size;

    return min.x <= other_max.x &&
        max.x >= other_min.x &&
        min.y <= other_max.y &&
        max.y >= other_min.y &&
        min.z <= other_max.z &&
        max.z >= other_min.z;
}

// width of box collider is accounted for it you swap the objects in the call, and swap the start and end.
//  i.e. for BOXES A and B, if A is moving towards static B,
//         old_target = target
//         A.clip_movement(B, A.position, &target) -> clips target
//         B.clip_movement(A, old_target, &A.position) -> clips A's start position
//         
//         This has the effect of making a line from one of A's faces to one of B's faces
//         The distance of this line can be computed to correctly compute the distance A is
//         allowed to move towrads B without clipping 
// (only problem is that this happens at the center only---if it causes problems for this game, perform this
//  over a small grid based on size of collider)
bool Collider::clip_movement(Collider other, glm::highp_vec3 start, glm::highp_vec3 *end) {
    if (*end - start == glm::highp_vec3(0.0f)) return false;
    if (!intersect(other)) return false;
    
    float dist = glm::length(*end - start);
    glm::highp_vec3 dir = glm::normalize(*end - start);

    // convert world direction and world position to this object space
    glm::highp_vec3 start_obj_space = obj_transform->make_local_from_world() * glm::highp_vec4(start, 0.f);
    glm::highp_vec3 dir_obj_space = obj_transform->make_local_from_world() * glm::highp_vec4(dir, 0.f);

    // my (modified) ray-bbox intersection code from 15-362 computer graphics
    // based on scratchpixel
    float time = 1.f;

    glm::highp_vec3 min = other.obj_transform->position + other.offset - other.size;
    glm::highp_vec3 max = other.obj_transform->position + other.offset + other.size;

    glm::highp_vec3 bounds[2] = { min, max };
    glm::highp_vec3 invdir = 1.f / dir_obj_space;

    int sign[3];
    sign[0] = dir_obj_space.x < 0;
    sign[1] = dir_obj_space.y < 0;
    sign[2] = dir_obj_space.z < 0;

    float tmin, tmax, tymin, tymax, tzmin, tzmax;
    if (dir_obj_space.x != 0.f) {
        tmin = (bounds[sign[0]].x - start_obj_space.x) * invdir.x;
        tmax = (bounds[1 - sign[0]].x - start_obj_space.x) * invdir.x;
    }
    else {
        tmin = -std::numeric_limits<float>::infinity();
        tmax = std::numeric_limits<float>::infinity();
    }

    if (dir_obj_space.y != 0.f) {
        tymin = (bounds[sign[1]].y - start_obj_space.y) * invdir.y;
        tymax = (bounds[1 - sign[1]].y - start_obj_space.y) * invdir.y;
    }
    else {
        tymin = -std::numeric_limits<float>::infinity();
        tymax = std::numeric_limits<float>::infinity();
    }

    if (tmin > tymax || tymin > tmax) {
        return false;
    }

    if (tymin > tmin) {
        tmin = tymin;
    }
    if (tymax < tmax) {
        tmax = tymax;
    }

    if (dir_obj_space.z != 0.f) {
        tzmin = (bounds[sign[2]].z - start_obj_space.z) * invdir.z;
        tzmax = (bounds[1 - sign[2]].z - start_obj_space.z) * invdir.z;
    }
    else {
        tzmin = -std::numeric_limits<float>::infinity();
        tzmax = std::numeric_limits<float>::infinity();
    }

    if (tmin > tzmax || tzmin > tmax) {
        return false;
    }

    if (tzmin > tmin) {
        tmin = tzmin;
    }
    if (tzmax < tmax) {
        tmax = tzmax;
    }

    // checking dist bounds
    if (tmax < 0.f || tmin > 1.f) {
        return false;
    }

    // normalize
    if (tmin > time)
        time = tmin;
    if (time < .01f) time = 0.f;

    // update end position
    *end = start + dir * time * dist;
    return true;
}
