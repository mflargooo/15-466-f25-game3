#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include "SoundManager.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;

Load< MeshBuffer > oil_rig_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("oil_rig.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

std::map< std::string, std::pair < const Mesh *, Scene::Transform * >> levers_mesh_transform;
std::vector< Scene::Drawable > hint_drawables;
std::map< std::string, std::vector< const Mesh * > > hint_meshes;
Load< Scene > oil_rig_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("oil_rig.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = oil_rig_meshes->lookup(mesh_name);
		std::cout << mesh_name << std::endl;

		if (mesh_name.find("lever") != std::string::npos) {
			levers_mesh_transform[mesh_name].first = &mesh;
			levers_mesh_transform[mesh_name].second = transform;
		} 
		else if (mesh_name.find("hintloc") != std::string::npos) {
			hint_drawables.emplace_back(transform);
		}
		else if (mesh_name.find("card") != std::string::npos) {
			size_t idx = std::stoull(mesh_name.substr(mesh_name.size() - 1, 1));
			std::string color = mesh_name.substr(5, mesh_name.size() - 7);

			if (hint_meshes.find(color) == hint_meshes.end()) {
				hint_meshes[color].resize(5);
			}
			hint_meshes[color][idx - 1] = &mesh;

		}
		else {
			scene.drawables.emplace_back(transform);
			Scene::Drawable &drawable = scene.drawables.back();

			drawable.pipeline = lit_color_texture_program_pipeline;

			drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
			drawable.pipeline.type = mesh.type;
			drawable.pipeline.start = mesh.start;
			drawable.pipeline.count = mesh.count;
		}
	});
});

Load< std::vector< Sound::Sample >> footsteps_samples(LoadTagDefault, []() -> std::vector< Sound::Sample > const * {
	std::vector< std::string > paths = { 
		"footsteps-01.wav", "footsteps-02.wav", "footsteps-03.wav",
		"footsteps-04.wav", "footsteps-05.wav", "footsteps-06.wav" };
	auto footsteps = new std::vector< Sound::Sample >();
	footsteps->reserve(paths.size());

	for (size_t i = 0; i < paths.size(); i++) {
		footsteps->emplace_back(Sound::Sample(data_path(paths[i])));
	}

	return footsteps;
});

Load< std::vector< Sound::Sample >> wind_samples(LoadTagDefault, []() -> std::vector< Sound::Sample > const * {
	std::vector< std::string > paths = { 
		"wind-01.wav", "wind-02.wav", "wind-03.wav", "wind-04.wav"};
	auto wind = new std::vector< Sound::Sample >();
	wind->reserve(paths.size());

	for (size_t i = 0; i < paths.size(); i++) {
		wind->emplace_back(Sound::Sample(data_path(paths[i])));
	}

	return wind;
});

Load< std::vector< Sound::Sample >> water_samples(LoadTagDefault, []() -> std::vector< Sound::Sample > const * {
	std::vector< std::string > paths = { 
		"water-01.wav", "water-02.wav", "water-03.wav",
		"water-04.wav", "water-05.wav", "water-06.wav",
		"water-07.wav", "water-08.wav", "water-09.wav",
		"water-10.wav", "water-11.wav", "water-12.wav",
		"water-13.wav" };
	auto water = new std::vector< Sound::Sample >();
	water->reserve(paths.size());

	for (size_t i = 0; i < paths.size(); i++) {
		water->emplace_back(Sound::Sample(data_path(paths[i])));
	}

	return water;
});

Load< std::vector< Sound::Sample >> rig_samples(LoadTagDefault, []() -> std::vector< Sound::Sample > const * {
	std::vector< std::string > paths = { 
		"falling_metal.wav", "pressure_release-01.wav", "pressure_release-02.wav"};
	auto rig = new std::vector< Sound::Sample >();
	rig->reserve(paths.size());

	for (size_t i = 0; i < paths.size(); i++) {
		rig->emplace_back(Sound::Sample(data_path(paths[i])));
	}

	return rig;
});

Load< std::vector< Sound::Sample >> siren_samples(LoadTagDefault, []() -> std::vector< Sound::Sample > const * {
	std::vector< std::string > paths = { 
		"siren_screech_loop.wav", "siren_song_loop.wav"};
	auto siren = new std::vector< Sound::Sample >();
	siren->reserve(paths.size());

	for (size_t i = 0; i < paths.size(); i++) {
		siren->emplace_back(Sound::Sample(data_path(paths[i])));
	}

	return siren;
});

void PlayMode::Player::update(float elapsed) {
	if (time_until_can_disenchant > 0.f) time_until_can_disenchant -= elapsed;
}

void PlayMode::Player::add_enchanted(float delta) {
	enchanted = std::max(0.f, std::min(enchanted + delta, 1.f));
}

float PlayMode::Player::get_enchanted() {
	return enchanted;
}

void PlayMode::Player::reset_disenchanted_timer() {
	time_until_can_disenchant = DISENCHANT_COOLDOWN;
}

float PlayMode::Player::get_disenchanted_timer() {
	return time_until_can_disenchant;
}

void PlayMode::Siren::set_volume(float enchantedness) {
	if (!active) return;

	enchantedness = std::max(0.f, std::min(enchantedness, 1.f));

	screech->set_volume(MAX_VOLUME * (1.f - enchantedness));
	song->set_volume(MAX_VOLUME * enchantedness);
}

void PlayMode::Siren::activate() {
	active = time_until_active <= 0.f;
}

void PlayMode::Siren::deactivate() {
	static std::random_device rd;
	static std::mt19937 rng { rd() };
	static std::uniform_real_distribution< float > dist(-1.f, 1.f);

	time_until_active = ACTIVATE_COOLDOWN + dist(rng);

	screech->set_volume(0.f);
	song->set_volume(0.f);
	
	active = false;
}

void PlayMode::Siren::reposition_relative_to(glm::vec3 position, float radius) {
	// assumes oil rig is centered at 0, 0, 0
	glm::vec3 dir;
	if (position.x == 0.f && position.y == 0.f) {

		std::random_device rd;
		std::mt19937 rng { rd() };
		std::uniform_real_distribution< float > dist(0, glm::radians(359.9f));
		float angle = dist(rng);
		dir = glm::vec3(std::cosf(angle), std::sinf(angle), 0.f);
	}
	else {
		dir = glm::normalize(position);
	}
	transform.position = dir * radius + glm::vec3(0.f, 0.f, position.z);

	// potentially use set_position, but it should not be the case that it
	// will be repositioned while volume > 0
	screech->position = transform.position - glm::vec3(0.f, 5.f, 0.f);
	song->position = transform.position - glm::vec3(0.f, 5.f, 0.f);
}

void PlayMode::Siren::update(float elapsed) {
	if (time_until_active > 0.f) time_until_active -= elapsed;
}

PlayMode::PlayMode() : scene(*oil_rig_scene) {

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	{
		player.transform.position = glm::vec3(0.f, 0.f, 0.f);
		camera->transform->parent = &player.transform;
		camera->transform->position = glm::vec3(0.f, 0.f, 2.f);

		player.reset_disenchanted_timer();
		}
	{
		// keep siren at player head level
		siren.transform.position = glm::vec3(0.f, 0.f, 2.f);
		siren.screech = Sound::loop_3D(siren_samples->at(0), 0.f, siren.transform.position - glm::vec3(0.f, 5.f, 0.f));
		siren.song = Sound::loop_3D(siren_samples->at(1), 0.f, siren.transform.position - glm::vec3(0.f, 5.f, 0.f));
		siren.reposition_relative_to(glm::vec3(0.f, 0.f, 2.f), 35);
	}


	// setup levers
	{
		for (size_t i = 0; i < 5; i++) {
			auto pair = levers_mesh_transform["lever.00" + std::to_string(i + 1)];
			
			scene.drawables.emplace_back(new Scene::Transform());
			levers.emplace_back();
			auto *lever = &levers.back();

			lever->drawable = &scene.drawables.back();

			*(lever->drawable->transform) = *(pair.second);
			lever->drawable->pipeline = lit_color_texture_program_pipeline;

			lever->drawable->pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
			lever->drawable->pipeline.start = pair.first->start; 
			lever->drawable->pipeline.count = pair.first->count; 
			lever->drawable->pipeline.type = pair.first->type; 
		}
	}

	// generate solution and populate hints
	{
		std::random_device rd;
		std::mt19937 rng{ rd() };
		std::uniform_int_distribution< size_t > dist(0, 4);
		solution.resize(levers.size());
		for (size_t i = 0; i < levers.size(); i++) {
			solution[i] = dist(rng);
		}

		std::vector< std::string > colors = { "red", "green", "blue", "orange", "purple" };
		std::shuffle(hint_drawables.begin(), hint_drawables.end(), rng);
		for (size_t i = 0; i < levers.size(); i++) {
			hint_drawables[i].pipeline = lit_color_texture_program_pipeline;
			hint_drawables[i].pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
			hint_drawables[i].pipeline.start = hint_meshes[colors[i]][solution[i]]->start;
			hint_drawables[i].pipeline.count = hint_meshes[colors[i]][solution[i]]->count;
			hint_drawables[i].pipeline.type = hint_meshes[colors[i]][solution[i]]->type;

			scene.drawables.emplace_back(hint_drawables[i]);
		}
	}
	

	// add colliders
	{
		colliders.emplace_back(&player.col);
		// colliders.emplace_back(&siren.col);

		// setup fence colliders
		colliders.emplace_back(new Collider(glm::vec3(21.3f, 0.f, 0.f), glm::vec3(.5f, 20.f, 1.f)));
		colliders.emplace_back(new Collider(glm::vec3(-21.f, 0.f, 0.f), glm::vec3(.5f, 20.f, 1.f)));
		colliders.emplace_back(new Collider(glm::vec3(0.f, 21.5f, 0.f), glm::vec3(20.f, .5f, 1.f)));
		colliders.emplace_back(new Collider(glm::vec3(0.f, -21.25f, 0.f), glm::vec3(20.f, .5f, 1.f)));

		// setup building colliders
		colliders.emplace_back(new Collider(glm::vec3(3.7f * 2.f, 0.f, 0.f), glm::vec3(.1f, 7.5f, 1.f)));
		colliders.emplace_back(new Collider(glm::vec3(-3.7f * 2.f, 0.f, 0.f), glm::vec3(.1f, 7.5f, 1.f)));
		colliders.emplace_back(new Collider(glm::vec3(-6.3f, 3.7f * 2.f, 0.f), glm::vec3(1.2f, .1f, 1.f)));
		colliders.emplace_back(new Collider(glm::vec3(3.75f, 3.7f * 2.f, 0.f), glm::vec3(3.75f, .1f, 1.f)));
		colliders.emplace_back(new Collider(glm::vec3(0.f, -3.7f * 2.f, 0.f), glm::vec3(7.5f, .1f, 1.f)));
		
		// terminal
		colliders.emplace_back(new Collider(glm::vec3(0.f, -6.5f, 0.f), glm::vec3(4.5f, 1.f, 1.f)));
		
		// boxes
		colliders.emplace_back(new Collider(glm::vec3(-1.5f, -8.5, 0.f), glm::vec3(3.5f, 1.f, 1.f)));
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_ESCAPE) {
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		} else if (evt.key.key == SDLK_A) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_LSHIFT) {
			lshift.downs += 1;
			lshift.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			space.downs = 1;
			space.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_F) {
			interact.downs = 1;
			interact.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_R) {
			Sound::stop_all_samples();
			return false;
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_A) {
			left.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_LSHIFT) {
			lshift.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			space.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_F) {
			interact.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == false) {
			SDL_SetWindowRelativeMouseMode(Mode::window, true);
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_MOTION) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == true) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);

			// yaw and pitch to avoid rolling, clamp each
			cam_info.yaw -= motion.x * camera->fovy;
			if (cam_info.yaw > (float)M_PI) cam_info.yaw -= (float)M_PI * 2.f;
			else if (cam_info.yaw < -(float)M_PI) cam_info.yaw += (float)M_PI * 2.f;

			cam_info.pitch += motion.y * camera->fovy;
			if (cam_info.pitch > (float)M_PI) cam_info.pitch = (float)M_PI;
			else if (cam_info.pitch < 0.f) cam_info.pitch = 0.f;

			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	if (player.dead) {
		static bool done_dead = false;
		if (!done_dead) {
			Sound::stop_all_samples();
			done_dead = true;
		}
		return;
	}
	else if (player.win) {
		static bool done_win = false;
		if(!done_win) {
			Sound::stop_all_samples();
			done_win = true;
		}
		return;
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	// handle siren activation and player-siren interaction
	{
		// only allow siren to enchant player every couple of frames so that player
		// can spam to disenchant self
		static size_t frame = 0;
		bool reposition = false;
		if (!siren.active) {
			siren.update(elapsed);
			siren.activate();
			reposition = false;
		}
		else {
			if (!reposition) {
				siren.reposition_relative_to(player.transform.position, 35);
				reposition = true;
			}

			// update necessary timers
			player.update(elapsed);
			
			float delta = player.get_disenchanted_timer() > 0.f ? (frame == 0 ? .2f : 0.f) :
				space.pressed ? -.75f : (frame == 0 ? .2f : 0.f);
			player.add_enchanted(elapsed * delta);
			siren.set_volume(player.get_enchanted());
			frame = (frame + 1) % 1;

			// after recovering from being enchanted, within .1
			if (player.get_enchanted() <= .05f && player.get_disenchanted_timer() <= 0.f) {
				siren.deactivate();
				player.reset_disenchanted_timer();
				reposition = false;
			}
		}
	}

	glm::vec3 diff = glm::vec3(siren.transform.position - player.transform.position);
	glm::vec3 to_siren = glm::normalize(diff);
	float mag_to_siren = glm::length(diff);
	to_siren.z = 0.f;
	
	float player_inf = 1.f - player.get_enchanted();
	float siren_inf = player.get_enchanted();
	float sound_muffler = std::max(player_inf, MIN_MUFFLED_SOUND_COEFF);

	//move camera:
	float cos_yaw = std::cosf(cam_info.yaw);
	float sin_yaw = std::sinf(cam_info.yaw);
	float cos_pitch = std::cosf(cam_info.pitch);
	float sin_pitch = std::sinf(cam_info.pitch);
	{	
		float yaw_to_siren = std::atan2(-to_siren.x, to_siren.y);
		float pitch_to_siren = glm::radians(90.f);

		if (player.get_enchanted() >= player.MIN_TO_ENCHANT_STATUS) {
			float siren_cam_inf = std::min(siren_inf, .9f);
			cam_info.yaw = cam_info.yaw * (1.f - siren_cam_inf * siren_cam_inf) + yaw_to_siren * siren_cam_inf * siren_cam_inf;
			cam_info.pitch = cam_info.pitch * (1.f - siren_cam_inf * siren_cam_inf) + pitch_to_siren * siren_cam_inf * siren_cam_inf;
		}

		camera->transform->rotation = glm::quat( glm::vec3(cam_info.pitch, 0.0f, cam_info.yaw) );
	}

	// handle player movement
	// player's ability to control movement is impaired by siren
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 5.0f;
		glm::vec2 move = glm::vec2(0.0f);
		glm::vec3 player_dir = glm::vec3(0.f);

		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;
		float dist = PlayerSpeed * elapsed * (lshift.pressed ? 1.5f : 1.0f);

		if (move != glm::vec2(0.f)) {
			//make it so that moving diagonally doesn't go faster:
			player_dir = glm::normalize(glm::vec3(
				move.x * cos_yaw - move.y * sin_yaw, 
				move.x * sin_yaw + move.y * cos_yaw, 
				0.f
			));
		}

		// add influence from siren
		bool play_footsteps = false;
		if (siren.active && to_siren != glm::vec3(0.f) && player.get_enchanted() > .05f) {
			// clip movement if collision
			for (Collider *col : colliders) {
				if (col == &player.col) continue;
				glm::vec3 tmp_siren = to_siren;
				if (player.col.clip_movement(*col, to_siren, dist) && 
					mag_to_siren <= 25.f &&
					((tmp_siren.x - to_siren.x > .75f) || tmp_siren.y - to_siren.y > .75f || tmp_siren.z - to_siren.z > .75f)) {
					player.dead = true;
					return;
				}
			}
			player.transform.position += to_siren * dist * player.get_enchanted() * .625f;
			play_footsteps = true;
		}
		if (player_dir != glm::vec3(0.f) && player.get_enchanted() < player.MIN_TO_ENCHANT_STATUS) {
			// clip player movement if collision
			for (Collider *col : colliders) {
				if (col == &player.col) continue;
				player.col.clip_movement(*col, player_dir, dist);
			}
			player.transform.position += player_dir * dist * player_inf;
			play_footsteps = true;
		}

		if (play_footsteps) SoundManager::play_sfx(footsteps_samples, (lshift.pressed ? .3f : .4f), elapsed, 0.f, .5f * sound_muffler);
	}

	glm::vec3 forward = glm::vec3(
		-sin_pitch * sin_yaw,
		sin_pitch * cos_yaw,
		-cos_pitch
	);
	{ // check if player is hoving for purpose of pop-up
		for (auto &lever : levers) {
			glm::vec3 to = ((lever.drawable->transform->make_world_from_local() * glm::vec4(lever.drawable->transform->position, 1.f) + lever.offset) 
				- player.transform.make_world_from_local() * glm::vec4(camera->transform->position, 1.f));
		
			if (glm::length(to) <= player.INTERACT_RANGE) {
				float resp = glm::dot(glm::normalize(to), forward);
				if (resp >= std::cos(lever.interact_angle)) {
					player.is_hovering = true;
					break;
				}
			}
			player.is_hovering = false;
		}
	}

	// handle interactions
	{
		static bool interacted = false;
		if (interact.pressed && !interacted && player.get_enchanted() < player.MIN_TO_ENCHANT_STATUS) {
			Lever *closest = nullptr;
			float closest_resp = 100.f;
			for (auto &lever : levers) {
				glm::vec3 to = ((lever.drawable->transform->make_world_from_local() * glm::vec4(lever.drawable->transform->position, 1.f) + lever.offset) 
					- player.transform.make_world_from_local() * glm::vec4(camera->transform->position, 1.f));

				if (glm::length(to) <= player.INTERACT_RANGE) {
					float resp = glm::dot(glm::normalize(to), forward);
					if (resp >= std::cos(lever.interact_angle) && resp <= closest_resp) {
						closest_resp = resp;
						closest = &lever;
					}
				}
			}           
			if (closest) closest->interact();
			interacted = true;
		}
		else if (!interact.pressed && interacted) {
			interacted = false;
		}

		// update levers
		for (auto lever : levers) {
			lever.update(elapsed);
		}
	}

	// check win
	{	
		size_t correct = 0;
		for (size_t i = 0; i < levers.size(); i++) {
			correct += solution[i] == levers[i].state ? 1 : 0;
		}

		player.win = correct == levers.size();
	}

	// ambient sounds
	{
		std::random_device rd;
		std::mt19937 rng { rd() };
		std::uniform_real_distribution< float > angle_dist(0, (float)M_PI * 2.f);
		std::uniform_real_distribution< float > radius_dist(0.f, 35.f);

		float angle = angle_dist(rng);
		float radius = radius_dist(rng);
		SoundManager::play_sfx_3D(rig_samples, 15.f, elapsed, 7.5f * glm::vec3(std::cosf(angle), radius / 17.5f, std::sinf(angle)), 1000.f, 11.f, .1f * sound_muffler);

		angle = angle_dist(rng);
		radius = radius_dist(rng);
		SoundManager::play_sfx_3D(wind_samples, 12.f, elapsed, 35.f * glm::vec3(std::cosf(angle), radius / 35.f * 3.f, std::sinf(angle)), 1000.f, 10.f, .5f * sound_muffler);
		
		angle = angle_dist(rng);
		radius = radius_dist(rng);
		SoundManager::play_sfx_3D(water_samples, 1.f, elapsed, radius * glm::vec3(std::cosf(angle), -5.f, std::sinf(angle)), 1000.f, 2.f, .1f * sound_muffler);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	lshift.downs = 0;
	space.downs = 0;
	interact.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(.05f, .05f, 0.05f)));
	glUseProgram(0);

	glClearColor(0.00125f, 0.0f, 0.0025f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Find the code for the correct lever configuration to repair the oil rig! Beware of the siren's song!",
			glm::vec3(-aspect + 0.1f * H, -.98 + 0.1f * H, 0.0),
			glm::vec3(H * .9f, 0.0f, 0.0f), glm::vec3(0.0f, H  * .9f, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Find the code for the correct lever configuration to repair the oil rig! Beware of the siren's song!",
			glm::vec3(-aspect + 0.1f * H + ofs, -.98 + 0.1f * H + ofs, 0.0),
			glm::vec3(H * .9f, 0.0f, 0.0f), glm::vec3(0.0f, H * .9f, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		static size_t frame = 0;
		static bool flip = false;
		if (frame == 0) {
			flip = !flip;
		}
		frame = (frame + 1) % 10;

		if (player.dead) {
			lines.draw_text("The Song Claimed You.",
				glm::vec3(-H * 8.f, 0.f, 0.0),
				glm::vec3(H * 2.f, 0.0f, 0.0f), glm::vec3(0.0f, H * 2.f, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw_text("The Song Claimed You.",
				glm::vec3(ofs - H * 8.f, ofs, 0.0),
				glm::vec3(H * 2.f, 0.0f, 0.0f), glm::vec3(0.0f, H * 2.f, 0.0f),
				glm::u8vec4(0xff, 0x00, 0x00, 0xff));
			lines.draw_text("R - Restart",
				glm::vec3(-.2, -.2f, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x0, 0x0, 0x0, 0x00));
			lines.draw_text("R - Restart",
				glm::vec3(ofs -.2, ofs -.2f, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0xff));

			lines.draw_text("Q - Quit",
				glm::vec3(-.15, -.35, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x0, 0x0, 0x0, 0x00));
			lines.draw_text("Q - Quit",
				glm::vec3(ofs -.15, ofs - .35f, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0xff));

		} else if (player.win) {
			lines.draw_text("You Repaired the Oil Rig!",
				glm::vec3(-H * 9.f, 0.f, 0.0),
				glm::vec3(H * 2.f, 0.0f, 0.0f), glm::vec3(0.0f, H * 2.f, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw_text("You Repaired the Oil Rig!",
				glm::vec3(ofs - H * 9.f, ofs, 0.0),
				glm::vec3(H * 2.f, 0.0f, 0.0f), glm::vec3(0.0f, H * 2.f, 0.0f),
				glm::u8vec4(0x00, 0xff, 0xff, 0xff));
			lines.draw_text("R - Play Again",
				glm::vec3(-.2, -.2f, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x0, 0x0, 0x0, 0x00));
			lines.draw_text("R - Play Again",
				glm::vec3(ofs -.2, ofs -.2f, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));

			lines.draw_text("Q - Quit",
				glm::vec3(-.15, -.35, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x0, 0x0, 0x0, 0x00));
			lines.draw_text("Q - Quit",
				glm::vec3(ofs -.15, ofs - .35f, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		}
		else if (siren.active && player.get_disenchanted_timer() <= 0.f) {
			if (flip) {
				lines.draw_text("SPACE",
					glm::vec3(-H, 0.f, 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0x00, 0x00, 0x00, 0x00));
				lines.draw_text("SPACE",
					glm::vec3(ofs - H, ofs, 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			}
		}
		else if (player.get_enchanted() < player.MIN_TO_ENCHANT_STATUS)  {
			// draw crosshair
			constexpr float Cursor_size = .15f;
			lines.draw_text(".",
				glm::vec3(0.f, 0.f, 0.0),
				glm::vec3(Cursor_size, 0.0f, 0.0f), glm::vec3(0.0f, Cursor_size, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw_text(".",
				glm::vec3(0.f, 0.f, 0.0),
				glm::vec3(Cursor_size, 0.0f, 0.0f), glm::vec3(0.0f, Cursor_size, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0xff));
			if (player.is_hovering) {
				lines.draw_text("F - interact", 
					glm::vec3(-H, -.9f * H, 0.0),
					glm::vec3(H * .625f, 0.0f, 0.0f), glm::vec3(0.0f, H * .625f, 0.0f),
					glm::u8vec4(0x00, 0x00, 0x00, 0x00));
				lines.draw_text("F - interact",
					glm::vec3(ofs - H, -.9f * H + ofs, 0.0),
					glm::vec3(H * .625f, 0.0f, 0.0f), glm::vec3(0.0f, H * .625, 0.0f),
					glm::u8vec4(0xff, 0xff, 0xff, 0xff));
			}
		}
		
		// world drawing for physics debugging.
		DrawLines world(glm::mat3x4(camera->make_projection()) * camera->transform->make_local_from_world());
		for (Collider *col : colliders) {
			world.draw_box(col->get_transformation_matrix(), glm::u8vec4(255, 0, 0, 255));
		}
	}
	GL_ERRORS();
}