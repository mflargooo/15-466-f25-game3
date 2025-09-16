#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include "Collision.hpp"
#include "Interactable.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, lshift, space, interact, q;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// if time allows, potentially maintain structure to register and play sfx
	// std::map< std::string, std::vector< Sound::Sample *>> sfx;

	std::vector< Collider * > colliders;

	struct Player {
		Scene::Transform transform;
		std::vector< Scene::Drawable > drawables;

		Collider col = Collider(&transform);

		float INTERACT_RANGE = 6.f;
		const float MIN_TO_ENCHANT_STATUS = .25f;
		bool dead = false;
		bool win = false;

		bool is_hovering = false;

		void add_enchanted(float delta);
		float get_enchanted();

		void reset_disenchanted_timer();
		float get_disenchanted_timer();
		void update(float elapsed);

		private:
			float enchanted = 0.f;
			float DISENCHANT_COOLDOWN = 4.0f;
			float time_until_can_disenchant = 0.f;

	} player;

	struct Siren {
		Scene::Transform transform;
		std::shared_ptr< Sound::PlayingSample > screech;
		std::shared_ptr< Sound::PlayingSample > song;
		bool active = false;

		// for debug purposes
		Collider col = Collider(&transform);


		void set_volume(float enchantedness);
		void reposition_relative_to(glm::vec3 position, float radius);
		void activate();
		void deactivate();
		void update(float elapsed);

		private:
			float MAX_VOLUME = .25f;
			float ACTIVATE_COOLDOWN = 30.f;
			float time_until_active = 10.f;
	} siren;
	
	//camera:
	Scene::Camera *camera = nullptr;
	struct CameraInfo {
		float yaw = 0.f;
		float pitch = glm::radians(90.f);
	} cam_info;

	// order of levers is red, green, blue, orange, purple
	std::vector< Lever > levers;
	std::vector< size_t > solution;

	float MIN_MUFFLED_SOUND_COEFF = .2f;
};
