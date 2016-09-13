//
//  rendering_system.cpp
//  OGLHCubeView
//
//  Created by Gabriele on 02/07/16.
//  Copyright � 2016 Gabriele. All rights reserved.
//
#include <transform.h>
#include <camera.h>
#include <entity.h>
#include <rendering_system.h>
#include <resources_manager.h>
#include <gameobject.h>

namespace hcube
{

	void rendering_system::on_attach(system_manager& sm)
	{
		for (auto& e : sm.get_entities()) on_add_entity(e);
	}

	void rendering_system::on_detach()
	{
		m_camera = nullptr;
		m_scene.clear();
	}

	void rendering_system::on_add_entity(entity::ptr e)
	{
		//copy ref
		if (e->has_component<camera>())     m_camera = e;
		else							    m_scene.m_pool.push(e);
	}

	void rendering_system::on_remove_entity(entity::ptr e)
	{
		//remove camera
		if (m_camera == e) m_camera = nullptr;
		//remove obj
		else               m_scene.m_pool.remove(e);
	}

	void rendering_system::on_update(double deltatime)
	{
		draw();
	}

	void rendering_system::set_clear_color(const vec4& clear_color)
	{
		m_clear_color = clear_color;
	}

	void rendering_system::set_ambient_color(const vec4& ambient_color)
	{
		m_ambient_color = ambient_color;
	}

	void rendering_system::add_rendering_pass(rendering_pass_ptr pass)
	{
		m_rendering_pass.push_back(pass);
	}

	void rendering_system::add_shadow_rendering_pass(rendering_pass_ptr pass)
	{
		m_shadow_pass = pass;
	}

	rendering_pass_shadow::rendering_pass_shadow(resources_manager& resources)
	{
		m_effect					 = resources.get_effect("build_shadow");
		m_technique_shadow_spot      = m_effect->get_technique("shadow_spot");
		m_technique_shadow_point     = m_effect->get_technique("shadow_point");
		m_technique_shadow_direction = m_effect->get_technique("shadow_direction");
        //params
        m_mask        = m_effect->get_parameter("mask");
        m_diffuse_map = m_effect->get_parameter("diffuse_map");
        //get uniform mask
        if(m_technique_shadow_spot && (*m_technique_shadow_spot).size())
        {
            m_shadow_spot_mask       = (*m_technique_shadow_spot)[0].m_shader->get_uniform("mask");
			m_shadow_spot_projection = (*m_technique_shadow_spot)[0].m_shader->get_uniform("projection");
			m_shadow_spot_view       = (*m_technique_shadow_spot)[0].m_shader->get_uniform("view");
			m_shadow_spot_model		 = (*m_technique_shadow_spot)[0].m_shader->get_uniform("model");
        }
        if(m_technique_shadow_point && (*m_technique_shadow_point).size())
        {
			m_shadow_point_mask			  = (*m_technique_shadow_point)[0].m_shader->get_uniform("mask");
			m_shadow_point_light_position = (*m_technique_shadow_point)[0].m_shader->get_uniform("light_position");
			m_shadow_point_far_plane	  = (*m_technique_shadow_point)[0].m_shader->get_uniform("far_plane");
			m_shadow_point_projection     = (*m_technique_shadow_point)[0].m_shader->get_uniform("projection");
			m_shadow_point_view			  = (*m_technique_shadow_point)[0].m_shader->get_uniform("view[0]");
			m_shadow_point_model	      = (*m_technique_shadow_point)[0].m_shader->get_uniform("model");
        }
        if(m_technique_shadow_direction && (*m_technique_shadow_direction).size())
        {
            m_shadow_direction_mask		    = (*m_technique_shadow_direction)[0].m_shader->get_uniform("mask");
			m_shadow_direction_projection	= (*m_technique_shadow_direction)[0].m_shader->get_uniform("projection");
			m_shadow_direction_view			= (*m_technique_shadow_direction)[0].m_shader->get_uniform("view[0]");
			m_shadow_direction_model		= (*m_technique_shadow_direction)[0].m_shader->get_uniform("model");
        }
	}

	void rendering_pass_shadow::draw_pass
	(
		vec4&  clear_color,
		vec4&  ambient_color,
		entity::ptr e_light,
		render_scene& rscene
	)
	{
		//shadow map
		auto l_light = e_light->get_component<light>();
		//enable shadow?
		if (!l_light->is_enable_shadow()) return;
		//current technique
		effect::technique* current_technique = nullptr;
		//current mask uniform
		uniform* u_shadow_mask = nullptr;
		//current u
		uniform* u_shadow_projection = nullptr;
		uniform* u_shadow_view	     = nullptr;
		uniform* u_shadow_model		 = nullptr;
		//type
		switch (l_light->get_type())
		{
			case light::SPOT_LIGHT: 
				current_technique   = m_technique_shadow_spot;
				u_shadow_mask	    = m_shadow_spot_mask;
				u_shadow_projection = m_shadow_spot_projection;
				u_shadow_view	    = m_shadow_spot_view;
				u_shadow_model		= m_shadow_spot_model;
			break;
			case light::POINT_LIGHT: 
				current_technique = m_technique_shadow_point;
				u_shadow_mask	  = m_shadow_point_mask;
				u_shadow_projection = m_shadow_point_projection;
				u_shadow_view       = m_shadow_point_view;
				u_shadow_model      = m_shadow_point_model;
			break;
			default: return;
		}
		//pass
		effect::pass& shadow_pass = (*current_technique)[0];
		//enable shadow buffer/texture
		l_light->get_shadow_buffer().bind();
		//clear
		render::clear();
		//applay pass
		auto state = shadow_pass.safe_bind();
		//default uniform
		u_shadow_projection->set_value(l_light->get_projection());
		//////////////////////////////////////////////////////////////////////////////
        //queue
        render_queue shadow_queue;
		//spot light vs point lights
		switch (l_light->get_type())
		{
		case light::SPOT_LIGHT:
			//update view frustum and queue
			rscene.m_pool.compute_opaque_queue
            (
                shadow_queue,
                l_light->update_frustum()
            );
			//set view
			u_shadow_view->set_value(l_light->get_view());
		break;
		case light::POINT_LIGHT:
			//build queue by sphere
			rscene.m_pool.compute_opaque_queue
            (
                shadow_queue,
				e_light->get_component<transform>()->get_global_position(),
				l_light->get_radius()
			);
			//uniform
			u_shadow_view->set_value(l_light->get_cube_view());
			//position
			if (m_shadow_point_light_position)
				m_shadow_point_light_position->set_value(
					e_light->get_component<transform>()->get_global_position()
			    );
			//radius
			if (m_shadow_point_far_plane)
				m_shadow_point_far_plane->set_value(l_light->get_radius());
		break;
		default: break;
		}
		//////////////////////////////////////////////////////////////////////////////
		//default texture
		texture::ptr default_texture = m_diffuse_map->get_texture();
		//set viewport
		render::set_viewport_state({ l_light->get_viewport() });
		//draw objs
		HCUBE_FOREACH_QUEUE(weak_element, shadow_queue.get_first())
		{
			auto entity     = weak_element->lock();
			auto t_entity   = entity->get_component<transform>();
			auto r_entity   = entity->get_component<renderable>();
            //events
            bool do_default_tex = true;
            bool do_default_mask= true;
			//test
			if (auto e_material = r_entity->get_material())
            {
                if (auto p_texture  = e_material->get_default_parameter(material::MAT_DEFAULT_DIFFUSE_MAP))
                if (auto t_texture  = p_texture->get_texture())
                {
                    //diffuse map
                    render::bind_texture(t_texture->get_context_texture(), 0);
                    //not bind default
                    do_default_tex = false;
                }
                if (auto p_mask  = e_material->get_default_parameter(material::MAT_DEFAULT_MASK))
                {
                    //uniform
					u_shadow_mask->set_value(p_mask->get_float());
                    //not bind default
                    do_default_mask = false;
                }
            }
            //diffuse map
            if(do_default_tex)  render::bind_texture(default_texture->get_context_texture(), 0);
            //uniform
            if(do_default_mask) u_shadow_mask->set_value(m_mask->get_float());
			//set transform
			u_shadow_model->set_value(t_entity->get_matrix());
			//draw
			r_entity->draw();
		}
		//disable shadow buffer/texture
		l_light->get_shadow_buffer().unbind();
		//disable
		shadow_pass.safe_unbind(state);
	}
    
    rendering_pass_debug_spot_lights::rendering_pass_debug_spot_lights(resources_manager& resources)
    {
        m_effect = resources.get_effect("debug_lights");
        ////// SPHERE //////////////////////////////////////////////////////////////////////////////
        //get mesh
        auto root_sphere = resources.get_prefab("sphere")->instantiate();
        //get sphere
        auto e_sphere = root_sphere->get_childs().begin()->second;
        //get component
        m_sphere = std::static_pointer_cast<renderable>( e_sphere->get_component<renderable>()->copy() );
        ////// CONE //////////////////////////////////////////////////////////////////////////////
        //get mesh
        auto root_cone = resources.get_prefab("cone")->instantiate();
        //get sphere
        auto e_cone = root_cone->get_childs().begin()->second;
        //get component
        m_cone = std::static_pointer_cast<renderable>(e_cone->get_component<renderable>()->copy());
    }
    
    void rendering_pass_debug_spot_lights::draw_pass
    (
        vec4&  clear_color,
        vec4&  ambient_color,
        entity::ptr e_camera,
        render_scene& rscene
    )
    {
        
        effect::pass& pass = (*m_effect->get_technique("forward"))[0];

		auto c_camera = e_camera->get_component<camera>();
		auto t_camera = e_camera->get_component<transform>();
        auto state = render::get_render_state();
        //viewport
        render::set_viewport_state({ c_camera->get_viewport() });
        //bind
        pass.bind();
        
        //for all lights
        HCUBE_FOREACH_QUEUE(weak_light, rscene.m_queues[RQ_SPOT_LIGHT].get_first())
        {
            auto e_light    = weak_light->lock();
            auto l_light    = e_light->get_component<light>();
            auto t_light    = e_light->get_component<transform>();
            
            l_light->update_projection_matrix();
            
            pass.m_shader
                ->get_uniform("viewport")
                ->set_value(c_camera->get_viewport());
            pass.m_shader
				->get_uniform("projection")
				->set_value(c_camera->get_projection());
			pass.m_shader
				->get_uniform("view")
				->set_value(c_camera->get_view());
            
            mat4
            model  = translate(mat4(1), t_light->get_global_position());
            model *= mat4_cast(t_light->get_global_rotation());
            model  = scale(model, { l_light->get_radius(), l_light->get_radius(),l_light->get_radius() });
            
			pass.m_shader->get_uniform("model")->set_value(model);

            m_cone->draw();
        }
        HCUBE_FOREACH_QUEUE(weak_light, rscene.m_queues[RQ_POINT_LIGHT].get_first())
        {
            auto e_light    = weak_light->lock();
            auto l_light    = e_light->get_component<light>();
            auto t_light    = e_light->get_component<transform>();
            
            l_light->update_projection_matrix();
            
            pass.m_shader
                ->get_uniform("viewport")
                ->set_value(c_camera->get_viewport());
            pass.m_shader
                ->get_uniform("projection")
                ->set_value(c_camera->get_projection());
            pass.m_shader
                ->get_uniform("view")
                ->set_value(c_camera->get_view());
            
            mat4
            model  = translate(mat4(1), t_light->get_global_position());
            model  = scale(model, { l_light->get_radius(), l_light->get_radius(),l_light->get_radius() });
            
            pass.m_shader->get_uniform("model")->set_value(model);
            
            m_sphere->draw();
        }
        
        
        pass.unbind();
        //end
        render::set_render_state(state);
    }

	void rendering_system::draw()
	{
		//culling
		camera::ptr   c_camera = m_camera->get_component<camera>();
		frustum&      f_camera = c_camera->get_frustum();
		//update view frustum
		f_camera.update_frustum(c_camera->get_projection()*c_camera->get_view());
		//update queue
        m_scene.m_pool.compute_light_queue( m_scene.m_queues[RQ_SPOT_LIGHT]
                                           ,m_scene.m_queues[RQ_POINT_LIGHT]
                                           ,m_scene.m_queues[RQ_DIRECTION_LIGHT]
                                           ,f_camera);
        //build shadow map
		if (m_shadow_pass)
		{
			//spot lights
			HCUBE_FOREACH_QUEUE(weak_light, m_scene.m_queues[RQ_SPOT_LIGHT].get_first())
			m_shadow_pass->draw_pass
			(
				m_clear_color,
				m_ambient_color,
                weak_light->lock(),
                m_scene
			);
			//point lights
			HCUBE_FOREACH_QUEUE(weak_light, m_scene.m_queues[RQ_POINT_LIGHT].get_first())
			m_shadow_pass->draw_pass
			(
				m_clear_color,
				m_ambient_color,
                weak_light->lock(),
                m_scene
			);
		}
        //obj
        m_scene.m_pool.compute_opaque_queue( m_scene.m_queues[RQ_OPAQUE], f_camera );
        m_scene.m_pool.compute_translucent_queue( m_scene.m_queues[RQ_TRANSLUCENT], f_camera );
		//all passes
		for (rendering_pass_ptr& pass : m_rendering_pass)
		{
			pass->draw_pass
			(
				m_clear_color,
				m_ambient_color,
				m_camera,
				m_scene
			);
		}
	}

	const vec4& rendering_system::get_clear_color() const
	{
		return m_clear_color;
	}

	const vec4& rendering_system::get_ambient_color() const
	{
		return m_ambient_color;
	}

	entity::ptr rendering_system::get_camera() const
	{
		return m_camera;
	}

	const std::vector< rendering_pass_ptr >& rendering_system::get_rendering_pass() const
	{
		return m_rendering_pass;
	}
	
	void rendering_system::stop_update_frustum(bool stop_update)
	{
		m_update_frustum = !stop_update;
	}

	void rendering_system::stop_frustum_culling(bool stop_culling)
	{
		m_stop_frustum_culling = stop_culling;
	}
}