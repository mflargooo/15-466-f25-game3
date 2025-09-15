#pragma once

#include "Scene.hpp"
#include <iostream>

struct Interactable {
    // update to use collider system if time allows
    float radius;
    Scene::Transform transform;   
    virtual void interact() {
        std::cout << "Interaction not yet implemented!" << std::endl;
    }
};

struct Lever : Interactable {
    uint8_t state = 0;
};
struct Pipe : Interactable {
    uint8_t state = 0;
};