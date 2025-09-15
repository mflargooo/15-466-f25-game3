#pragma once
#include "SoundManager.hpp"

#include <random>
#include <map>
#include <iostream>

std::shared_ptr< Sound::PlayingSample > SoundManager::play_sfx(
    const std::vector< Sound::Sample > *samples, 
	float cooldown, 
	float elapsed,
	float vary, 
	float volume, 
	float pan
) {
	static std::random_device rd;
	static std::mt19937 rng { rd() };
	static std::map<const std::vector< Sound::Sample > *, float > timers;
	static std::map<const std::vector< Sound::Sample > *, std::uniform_int_distribution< size_t > > size_dists;
	static std::map<const std::vector< Sound::Sample > *, std::uniform_real_distribution< float > > vary_dists;

	if (timers.find(samples) == timers.end()) {
	    std::uniform_int_distribution< size_t > size_dist(0, samples->size() - 1);
        std::uniform_real_distribution< float > vary_dist(0, vary);
    
        size_dists[samples] = size_dist;
        vary_dists[samples] = vary_dist;
        timers[samples] = cooldown + (vary == 0.f ? 0.f : vary_dists[samples](rng));
    }

    if (timers[samples] <= 0.f) {
		timers[samples] = cooldown + vary_dists[samples](rng);
		return Sound::play(samples->at(size_dists[samples](rng)), volume, pan);
	}
	else {
		timers[samples] -= elapsed;
	}

    return nullptr;
}

std::shared_ptr< Sound::PlayingSample > SoundManager::play_sfx_3D(
    const std::vector< Sound::Sample > *samples, 
	float cooldown, 
	float elapsed,
	glm::vec3 &position,
	float vary, 
	float half_volume_radius,
	float volume
) {
	static std::random_device rd;
	static std::mt19937 rng { rd() };
	static std::map<const std::vector< Sound::Sample > *, float > timers;
	static std::map<const std::vector< Sound::Sample > *, std::uniform_int_distribution< size_t > > size_dists;
	static std::map<const std::vector< Sound::Sample > *, std::uniform_real_distribution< float > > vary_dists;

	if (timers.find(samples) == timers.end()) {
	    std::uniform_int_distribution< size_t > size_dist(0, samples->size() - 1);
        std::uniform_real_distribution< float > vary_dist(0, vary);
    
        size_dists[samples] = size_dist;
        vary_dists[samples] = vary_dist;
        timers[samples] = cooldown + (vary == 0.f ? 0.f : vary_dists[samples](rng));
    }
    
    if (timers[samples] <= 0.f) {
		timers[samples] = cooldown + vary_dists[samples](rng);
		return Sound::play_3D(samples->at(size_dists[samples](rng)), volume, position, half_volume_radius);
	}
	else {
		timers[samples] -= elapsed;
	}

    return nullptr;
}