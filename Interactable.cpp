#include "Interactable.hpp"
#include <glm/glm.hpp>

void Lever::interact() {
    state = (state + 1) % 5;
}

void Lever::update(float elapsed) {
    glm::vec3 euler = glm::eulerAngles(drawable->transform->rotation);
    drawable->transform->rotation = glm::quat( glm::vec3( euler.x * .9f + glm::radians(-15.f * (float)((size_t)state + 1) * .1f), euler.y, euler.z ) );
}