#include <transform.h>
#include <rendering_pass_forward.h>

namespace hcube
{


	void rendering_pass_forward::draw_pass(
		vec4&  clear_color,
		vec4&  ambient_color,
		entity::ptr e_camera,
		render_queues& queues
	)
	{
		//save state
		auto render_state = render::get_render_state();
		//get camera
		camera::ptr   c_camera = e_camera->get_component<camera>();
		transform_ptr t_camera = e_camera->get_component<transform>();
		const vec4& viewport   = c_camera->get_viewport();
		//set state camera
		render::set_viewport_state({ viewport });
		render::set_clear_color_state({ clear_color });
		render::clear();
		//draw
		HCUBE_FOREACH_QUEUE(weak_element,queues.m_cull_opaque)
		{
			auto entity = weak_element->lock();
			auto t_entity = entity->get_component<transform>();
			auto r_entity = entity->get_component<renderable>();
			auto e_material = r_entity->get_material();
			//material
			if (e_material)
			{
				effect::ptr         mat_effect  = e_material->get_effect();
				effect::technique*  mat_forward = mat_effect->get_technique("forward");
				//applay all pass
				if (mat_forward) for (auto& pass : *mat_forward)
				{
					pass.bind(
						viewport,
						c_camera->get_projection(),
						t_camera->get_matrix_inv(),
						t_entity->get_matrix(),
						e_material->get_parameters()
					);
					//draw
					if (!pass.m_support_light)
					{
						//draw
						entity->get_component<renderable>()->draw();
					}
					//ambient draw
					else if (pass.m_uniform_ambient_light)
					{
						pass.m_uniform_ambient_light->set_value(ambient_color);
						//draw
						entity->get_component<renderable>()->draw();
					}
					//draw all spot lights
					else if(pass.m_uniform_spot.is_valid())
					{
						HCUBE_FOREACH_QUEUE(weak_light, queues.m_cull_light_spot)
						{
							auto e_light = weak_light->lock();
							auto l_light = e_light->get_component<light>();
							auto t_light = e_light->get_component<transform>();
							pass.m_uniform_spot.uniform(
								l_light,
								t_light ->get_matrix_inv(),
								t_camera->get_matrix_inv(),
								t_light ->get_matrix()
							);
							//draw
							entity->get_component<renderable>()->draw();
						}
                    }
                    //draw all point lights
                    else if(pass.m_uniform_point.is_valid())
                    {
                        HCUBE_FOREACH_QUEUE(weak_light, queues.m_cull_light_point)
                        {
                            auto e_light = weak_light->lock();
                            auto l_light = e_light->get_component<light>();
                            auto t_light = e_light->get_component<transform>();
                            pass.m_uniform_point.uniform(
                                                        l_light,
                                                        t_camera->get_matrix_inv(),
                                                        t_light ->get_matrix()
                                                        );
                            //draw
                            entity->get_component<renderable>()->draw();
                        }
                    }
                    //draw all direction lights
                    else if(pass.m_uniform_direction.is_valid())
                    {
                        HCUBE_FOREACH_QUEUE(weak_light, queues.m_cull_light_direction)
                        {
                            auto e_light = weak_light->lock();
                            auto l_light = e_light->get_component<light>();
                            auto t_light = e_light->get_component<transform>();
                            pass.m_uniform_direction.uniform(
                                                            l_light,
                                                            t_camera->get_matrix_inv(),
                                                            t_light ->get_matrix()
                                                            );
                            //draw
                            entity->get_component<renderable>()->draw();
                        }
                    }
					//end
					pass.unbind();
				
				}
			}
		}
		//reset state
		render::set_render_state(render_state);
	}
}