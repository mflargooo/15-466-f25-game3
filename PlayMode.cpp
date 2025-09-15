#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include "Collision.hpp"
#include "SoundManager.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;

Load< MeshBuffer > oil_rig_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("oil_rig.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > oil_rig_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("oil_rig.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = oil_rig_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

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

PlayMode::PlayMode() : scene(*oil_rig_scene) {

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	/*
	//start music loop playing:
	// (note: position will be over-ridden in update())
	leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);
	*/
	player.transform.position = glm::vec3(0.f, 0.f, 0.f);
	camera->transform->parent = &player.transform;
	camera->transform->position = glm::vec3(0.f, 0.f, 2.f);

	colliders.emplace_back(&player.col);

	// setup fence colliders
	colliders.emplace_back(new Collider(glm::vec3(21.3f, 0.f, 0.f), glm::vec3(.5f, 20.f, 1.f)));
	colliders.emplace_back(new Collider(glm::vec3(-21.f, 0.f, 0.f), glm::vec3(.5f, 20.f, 1.f)));
	colliders.emplace_back(new Collider(glm::vec3(0.f, 21.5f, 0.f), glm::vec3(20.f, .5f, 1.f)));
	colliders.emplace_back(new Collider(glm::vec3(0.f, -21.25f, 0.f), glm::vec3(20.f, .5f, 1.f)));
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
	SoundManager::play_sfx(rig_samples, 45.f, elapsed, 20.f, .5f);

	//move camera:
	float cos_yaw = std::cosf(cam_info.yaw);
	float sin_yaw = std::sinf(cam_info.yaw);
	// float cos_pitch = std::cosf(cam_info.pitch);
	// float sin_pitch = std::sinf(cam_info.pitch);
	{
		camera->transform->rotation = glm::quat( glm::vec3(cam_info.pitch, 0.0f, cam_info.yaw));
	}

	// handle player movement
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 5.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;


		if (move != glm::vec2(0.f)) {
			//make it so that moving diagonally doesn't go faster:
			glm::vec3 dir = glm::normalize(glm::vec3(
				move.x * cos_yaw - move.y * sin_yaw, 
				move.x * sin_yaw + move.y * cos_yaw, 
				0.f
			));
			float dist = PlayerSpeed * elapsed * (lshift.pressed ? 1.5f : 1.0f);

			// clip player movement if collision
			for (Collider *col : colliders) {
				if (col == &player.col) continue;
				player.col.clip_movement(*col, dir, dist);
			}
			player.transform.position += dir * dist;
			SoundManager::play_sfx(footsteps_samples, lshift.pressed ? .3f : .4f, elapsed);
		}
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
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
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		
		// world drawing for physics debugging.
		DrawLines world(glm::mat3x4(camera->make_projection()) * camera->transform->make_local_from_world());
		for (Collider *col : colliders) {
			world.draw_box(col->get_transformation_matrix(), glm::u8vec4(255, 0, 0, 255));
		}
	}
	GL_ERRORS();
}