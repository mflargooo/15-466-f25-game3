#pragma once

#include "Scene.hpp"
#include <iostream>

struct Interactable {
    Scene::Drawable *drawable;   
    glm::vec3 offset = glm::vec3(0.f, 0.f, -.25f);

    float interact_angle = glm::radians(10.f);
    virtual void interact() {
        std::cout << "Interaction not yet implemented!" << std::endl;
    };
    virtual void update(float elapsed) {
        std::cout << "Update not yet implemented!" << std::endl;
     };
};

struct Lever : Interactable {
    size_t state = 0;
    void interact() override;
    void update(float elapsed) override;
};