//
//  app_basic.cpp
//  OGLHCubeView
//
//  Created by Gabriele on 07/07/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#include <regex>
#include <iostream>
#include <hcube/math/vector_math.h>
#include <hcube/core/filesystem.h>
#include <hcube/geometries/frustum.h>
#include <hcube/render/rendering_pass_deferred.h>
#include <hcube/render/rendering_pass_forward.h>
#include <hcube/render/rendering_pass_shadow.h>
#include <hcube/components/transform.h>
#include <hcube/components/renderable.h>
#include <hcube/utilities/gameobject.h>
#include <hcube/utilities/basic_meshs.h>

#include <app_basic.h>


namespace hcube
{
	enum 
	{
		SW_LOW,
		SW_MID,
		SW_HIG,
		SW_VERY_HIG,
	};
	const vec2 shadow_quality[] =
	{
		{ 256,256 },
		{ 512,512 },
		{ 1024,1024 },
		{ 2048,2048 },
	};

	void app_basic::key_event(application& app, int key, int scancode, int action, int mods)
	{

		//get model
		auto e_model = m_systems.get_entities_by_name("ship")[0];

		if (key == GLFW_KEY_ESCAPE)
		{
			m_loop = false;
			return;
		}
		else if (key == GLFW_KEY_O)
		{
			rendering_system*	r_system = m_systems.get_system<rendering_system>();
			rendering_pass_ptr	d_pass = r_system->get_rendering_pass()[0];
			auto p_deferred = std::static_pointer_cast<rendering_pass_deferred>(d_pass);
			p_deferred->set_ambient_occlusion({ true, 16, 0.3f });
		}
		else if (key == GLFW_KEY_K)
		{
			rendering_system*	r_system = m_systems.get_system<rendering_system>();
			rendering_pass_ptr	d_pass = r_system->get_rendering_pass()[0];
			auto p_deferred = std::static_pointer_cast<rendering_pass_deferred>(d_pass);
			p_deferred->set_ambient_occlusion({false,0,0});
		}
		else if (key == GLFW_KEY_U)
		{
			rendering_system*	r_system = m_systems.get_system<rendering_system>();
			r_system->stop_update_frustum(false);
		}
		else if (key == GLFW_KEY_H)
		{
			rendering_system*	r_system = m_systems.get_system<rendering_system>();
			r_system->stop_update_frustum(true);
		}
		else if (key == GLFW_KEY_I)
		{
			rendering_system*	r_system = m_systems.get_system<rendering_system>();
			r_system->stop_frustum_culling(false);
		}
		else if (key == GLFW_KEY_J)
		{
			rendering_system*	r_system = m_systems.get_system<rendering_system>();
			r_system->stop_frustum_culling(true);
		}
		else if (key == GLFW_KEY_UP)          e_model->get_component<transform>()->translation({ 0,0,1 });
		else if (key == GLFW_KEY_DOWN)        e_model->get_component<transform>()->translation({ 0,0,-1 });
		else if (key == GLFW_KEY_LEFT)        e_model->get_component<transform>()->translation({-1,0,0 });
		else if (key == GLFW_KEY_RIGHT)       e_model->get_component<transform>()->translation({ 1,0,0 });
		else if (key == GLFW_KEY_PAGE_UP)     e_model->get_component<transform>()->translation({ 0,1,0 });
		else if (key == GLFW_KEY_PAGE_DOWN)   e_model->get_component<transform>()->translation({ 0,-1,0 });
		else if (key == GLFW_KEY_W)    m_camera->get_component<transform>()->move({ 0,0,1 });
		else if (key == GLFW_KEY_S)    m_camera->get_component<transform>()->move({ 0,0,-1 });
		else if (key == GLFW_KEY_A)    m_camera->get_component<transform>()->move({-1,0,0 });
		else if (key == GLFW_KEY_D)    m_camera->get_component<transform>()->move({ 1,0,0 });
		else if (key == GLFW_KEY_R)    m_camera->get_component<transform>()->move({ 0,1,0 });
		else if (key == GLFW_KEY_F)    m_camera->get_component<transform>()->move({ 0,-1,0 });
		else if ((mods == GLFW_MOD_SUPER ||
				  mods == GLFW_MOD_CONTROL) &&
				  action == GLFW_PRESS)
		{
			if (key == GLFW_KEY_M)
			{
#if !defined(__APPLE__)
				if (app.is_fullscreen()) go_to_window_mode(app);
				else                     go_to_fullscreen(app);
#endif
			}
		}
	}

	void app_basic::cursor_position_event(application& app, const dvec2& in_mouse_pos)
	{
		camera_look_around(app);
	}

	void app_basic::mouse_button_event(application& app, int button, int action, int mods)
	{
	}

	void app_basic::scroll_event(application& application, const dvec2& scroll_offset)
	{

	}

	void app_basic::resize_event(application& application, const ivec2& size)
	{
		m_aspect = float(size.x) / float(size.y);
		//viewport
		m_camera->get_component<camera>()->set_viewport(ivec4{ 0, 0, size.x, size.y });
		//new perspective
		m_camera->get_component<camera>()->set_perspective(m_fov, m_aspect, 0.1, 500.0);
	}

	void app_basic::camera_look_around(application& app)
    {
		//center
		dvec2 mouse_center = (dvec2)app.get_window_size()*0.5;
		//mouse pos
		dvec2 mouse_pos = app.get_mouse_position();
		//direction
		dvec2 mouse_dir = mouse_pos - mouse_center;
		//compute speed
		double mouse_speed = length(mouse_dir / (dvec2)app.get_window_size()) * 100.0f;
		//do not
		if (mouse_speed <= 0.49) return;
		//norm direction
		dvec2 norm_mouse_dir = normalize(mouse_dir);
		//angle
		m_camera_angle += norm_mouse_dir * mouse_speed * app.get_last_delta_time();
		//rote
		m_camera->get_component<transform>()->rotation(quat{ vec3{ m_camera_angle.y, m_camera_angle.x,0.0} });
		//reset pos
		app.set_mouse_position(mouse_center);
	}

	void app_basic::start(application& app)
	{
		//set mouse at center
		app.set_mouse_position((dvec2)app.get_window_size()*0.5);
		//commond assets
		m_resources.add_resources_file("common.rs");
		//get info about window
		m_window_mode_info = window_info
		{
			app.get_window_size(),
			app.get_window_position()
		};
		//alloc all systems
		rendering_system::ptr m_rendering = rendering_system::snew();
		//add shadow support
		m_rendering->add_rendering_pass(rendering_pass_shadow::snew(m_resources));
		//add into system
		m_systems.add_system(m_rendering);
		m_rendering->add_rendering_pass(rendering_pass_deferred::snew(app.get_window_size(), m_resources));
		//or
		//m_rendering->add_rendering_pass(rendering_pass_forward::snew(RQ_OPAQUE));
		//debug
        //m_rendering->add_rendering_pass(rendering_pass_debug_spot_lights::snew(m_resources));
        //translucent
        m_rendering->add_rendering_pass(rendering_pass_forward::snew(RQ_TRANSLUCENT));
		//load assets
		m_resources.add_directory("assets/textures");
		m_resources.add_directory("assets/materials");
        m_resources.add_directory("assets/ship");
        m_resources.add_directory("assets/ship_fighter");
		m_resources.add_directory("assets/asteroid");
		/////// /////// /////// /////// /////// /////// /////// /////// ///////
		// build scene
		{
			//camera
			m_camera = gameobject::camera_new();
			auto c_camera = m_camera->get_component<camera>();
			auto t_camera = m_camera->get_component<transform>();
			vec2 size = app.get_window_size();
			m_aspect = float(size.x) / float(size.y);
			c_camera->set_viewport(ivec4{ 0, 0, size.x, size.y });
			c_camera->set_perspective(m_fov, m_aspect, 0.1, 500.0);
			t_camera->look_at(vec3{ 0.0f, 6.9f, -45.0f },
							  vec3{ 0.0f, 1.0f, 0.0f },
							  vec3{ 0.0f, 1.0f, 0.0f });
			//set camera
			m_systems.add_entity(m_camera);
			//add cube
			{
				auto cube_grid = gameobject::node_new(
					basic_meshs::cube({ 5,5,5 }, true)
				);
				cube_grid->get_component<renderable>()->set_material(
					m_resources.get_material("cross_mat")
				);
				cube_grid->get_component<transform>()->translation(
					vec3{ -5.0,-7.5,-10.0 }
				);
				cube_grid->get_component<transform>()->scale(
					vec3{ 1.0,1.0,1.0 }
				);
				cube_grid->set_name("cube1");
				m_systems.add_entity(cube_grid);
			}
			//add cube 2
			{
				auto cube_grid = gameobject::node_new(
					//basic_meshs::cube({ 5,5,5 }, true)
					basic_meshs::icosphere(2.5,true)
				);
				cube_grid->get_component<renderable>()->set_material(
					m_resources.get_material("box2_grid_mat")
				);
				cube_grid->get_component<transform>()->translation(
					vec3{ 5.0,-7.5,-10.0 }
				);
				cube_grid->set_name("cube2");
				m_systems.add_entity(cube_grid);
			}
            //add cube 3
            {
                auto cube_grid = gameobject::node_new(
                                                      basic_meshs::cube({ 5,5,5 }, true)
                                                      );
                cube_grid->get_component<renderable>()->set_material(
                        m_resources.get_material("window_mat")
                );
                cube_grid->get_component<transform>()->translation(
                        vec3{ 0.0,-7.5,-5.0 }
                );
                cube_grid->get_component<transform>()->scale(
                        vec3{ 1.0,1.0,1.0 }
                );
                cube_grid->set_name("window_cube1");
                m_systems.add_entity(cube_grid);
            }
			//cube floor
            for(int x=-2;x!=2;++x)
            for(int y=-2;y!=2;++y)
            {
                auto cube_floor = gameobject::node_new(
                    basic_meshs::cube({ 50,50,50 }, true)
                );
                cube_floor->get_component<renderable>()->set_material(
                    m_resources.get_material("rocks_mat")
                ); 
                cube_floor->get_component<transform>()->translation(
                    vec3{x*50.,-36,y*50.}
                );
                cube_floor->set_name("cube_floor");
                m_systems.add_entity(cube_floor);
            }
			//ship_fighter
            {
                auto e_ship_fighter = m_resources.get_prefab("ship_fighter")->instantiate();
                auto t_ship_fighter = e_ship_fighter->get_component<transform>();
                t_ship_fighter->position({ 0.0f, 0.0f, 30.0f });
                t_ship_fighter->rotation(quat({ radians(15.0), radians(180.0), 0.0 }));
                t_ship_fighter->scale({ 0.013f, 0.013f, 0.013f });
                //set name
                e_ship_fighter->set_name("ship_fighter");
                
                m_systems.add_entity(e_ship_fighter);
            }
			//ship
			m_model = m_resources.get_prefab("ship")->instantiate();
			auto t_model = m_model->get_component<transform>();
			//set name
			m_model->set_name("ship");
			//set info
			t_model->position({ 0.0f, -5.0f, 0.0f });
			t_model->rotation(quat({ radians(15.0), radians(0.0), 0.0 }));
			t_model->scale({ 0.2f, 0.2f, 0.2f });

			auto e_model_light = gameobject::light_new();
			auto l_model_light = e_model_light->get_component<light>();
			auto t_model_light = e_model_light->get_component<transform>();
			t_model_light->position(vec3{ 0,6.0f,-80 });
			l_model_light->point(
				{ 1.0f, 0.8f, 0.1f },
				{ 1.0f, 0.8f, 0.1f },
				1.0,
				2.0,
				8.0
			);
			m_model->add_child(e_model_light);

			auto e_model_light1 = gameobject::light_new();
			auto l_model_light1 = e_model_light1->get_component<light>();
			auto t_model_light1 = e_model_light1->get_component<transform>();
			t_model_light1->position(vec3{ -16.4,10.5f,18.0 });
			t_model_light1->rotation(quat({ radians(0.0), radians(0.0), 0.0 }));
			l_model_light1->spot({ 1.0f, 0.8f, 0.1f },
								 { 1.0f, 1.0f, 1.0f },
									1.0,
									30.0,
									40.0,
									radians(10.0),
									radians(15.0));
			l_model_light1->set_shadow(shadow_quality[SW_VERY_HIG]);
            e_model_light1->set_name("ship_light1");
            m_model->add_child(e_model_light1);

			auto e_model_light2 = gameobject::light_new();
			auto l_model_light2 = e_model_light2->get_component<light>();
			auto t_model_light2 = e_model_light2->get_component<transform>();
			t_model_light2->position(vec3{ 16.4,10.0f,19 });
			t_model_light2->rotation(quat({ radians(0.0), radians(0.0), 0.0 }));
			l_model_light2->spot({ 1.0f, 0.8f, 0.1f },
								 { 1.0f, 1.0f, 1.0f },
								   1.0,
								   30.0,
								   40.0,
								   radians(10.0),
								   radians(15.0));
			l_model_light2->set_shadow(shadow_quality[SW_VERY_HIG]);
            e_model_light2->set_name("ship_light2");
			m_model->add_child(e_model_light2);
			//add to render
			m_systems.add_entity(m_model);

			//lights
			entity::ptr e_lights[3] =
			{
				gameobject::light_new(),
				gameobject::light_new(),
				gameobject::light_new()
			};
			light_ptr l_lights[3] =
			{
				e_lights[0]->get_component<light>(),
				e_lights[1]->get_component<light>(),
				e_lights[2]->get_component<light>()
			};
			transform_ptr t_lights[3] =
			{
				e_lights[0]->get_component<transform>(),
				e_lights[1]->get_component<transform>(),
				e_lights[2]->get_component<transform>()
			};
			for (short i = 0; i != 3; ++i)
			{
				l_lights[i]->set_radius(
					8.0,
					33.0
				);
				l_lights[i]->set_shadow(shadow_quality[SW_VERY_HIG]);
			}

			l_lights[0]->set_color({ 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
			e_lights[0]->set_name("light_green");

			l_lights[1]->set_color({ 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
			e_lights[1]->set_name("light_red");

			l_lights[2]->set_color({ 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f });
			e_lights[2]->set_name("light_blue");


			for (int i = 1; i != 4; ++i)
			{
				t_lights[i - 1]->position({
                    std::sin((constants::pi<float>()*0.66)*i)*15.,
					5.0,
                    std::cos((constants::pi<float>()*0.66)*i)*15.,
				});
			}

			//node lights
			m_lights = gameobject::node_new();
			m_lights->add_child(e_lights[0]);
			m_lights->add_child(e_lights[1]);
			m_lights->add_child(e_lights[2]);

			//add to render
			m_systems.add_entity(m_lights);

			//ambient color
			m_rendering->set_ambient_color(vec4{ 0.16, 0.16, 0.16, 1.0 });

		}
	}

	bool app_basic::run(application& app, double delta_time)
    {
		//////////////////////////////////////////////////////////
		//update
		m_model->get_component<transform>()->turn(quat{ {0.0, radians(5.0*delta_time), 0.0} });
		//for all lights
		m_lights
			->get_component<transform>()
			->turn(quat{ {0.0, radians(20.0*delta_time), 0.0} });
		//////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////
		//draw
		m_systems.update(delta_time);
		//////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////
		//do loop
		return m_loop;
	}

	bool app_basic::end(application& app)
	{
		return true;
	}

	void app_basic::go_to_fullscreen(application& app)
	{
		//get info
		m_window_mode_info = window_info
		{
			app.get_window_size(),
			app.get_window_position()
		};
		//go to fullscreen mode
		app.set_fullscreen_size(app.get_screen_size());
	}

	void app_basic::go_to_window_mode(application& app)
	{
		app.set_window_size(m_window_mode_info.m_position, m_window_mode_info.m_size);
	}
	
}