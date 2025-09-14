#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include "Collision.hpp"

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
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > leg_tip_loop;

	//car honk sound:
	std::shared_ptr< Sound::PlayingSample > honk_oneshot;

	std::vector< Collider * > colliders;

	struct Player {
		Scene::Transform transform;
		std::vector< Scene::Drawable > drawables;

		Collider col = Collider(&transform);

	} player;
	
	//camera:
	Scene::Camera *camera = nullptr;
	struct CameraInfo {
		float yaw = 0.f;
		float pitch = 0.f;
	} cam_info;

};
