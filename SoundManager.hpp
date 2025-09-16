#pragma once

#include "Sound.hpp"

namespace SoundManager {
    std::shared_ptr< Sound::PlayingSample > play_sfx(
        const std::vector< Sound::Sample > *samples, 
        float cooldown, 
        float elapsed,
        float vary=(0.F),
        float volume=(1.0F), 
        float pan=(0.0F)
    );

    std::shared_ptr< Sound::PlayingSample > play_sfx_3D(
        const std::vector< Sound::Sample > *samples, 
        float cooldown, 
        float elapsed,
        const glm::vec3 &position,
        float half_volume_radius,
        float vary=(0.F),
        float volume=(1.0F)
    );
}