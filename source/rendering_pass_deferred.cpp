#include <rendering_pass_deferred.h>
#include <basic_meshs.h>
#include <transform.h>

void rendering_pass_deferred::uniform_light::get_uniform(int i, shader::ptr shader)
{
	std::string lights_i("lights[" + std::to_string(i) + "]");
	m_uniform_position = shader->get_shader_uniform_vec3((lights_i + ".m_position").c_str());
	m_uniform_diffuse = shader->get_shader_uniform_vec4((lights_i + ".m_diffuse").c_str());
	m_uniform_const = shader->get_shader_uniform_float((lights_i + ".m_const").c_str());
	m_uniform_linear = shader->get_shader_uniform_float((lights_i + ".m_linear").c_str());
	m_uniform_quadratic = shader->get_shader_uniform_float((lights_i + ".m_quadratic").c_str());
}

void rendering_pass_deferred::uniform_light::uniform(light_wptr weak_light,const glm::mat4& model)
{
    auto light = weak_light.lock();
    m_uniform_position->set_value((glm::vec3)(model * glm::vec4(0,0,0,1.0)));
	m_uniform_diffuse->set_value(light->m_diffuse);
	m_uniform_const->set_value(light->m_const);
	m_uniform_linear->set_value(light->m_linear);
	m_uniform_quadratic->set_value(light->m_quadratic);
}

rendering_pass_deferred::rendering_pass_deferred(entity::ptr e_camera, resources_manager& resources)
{
	m_g_buffer.init(e_camera->get_component<camera>()->get_viewport_size());
	m_square = basic_meshs::square3D({ 2.0,2.0 }, true);
	m_shader = resources.get_shader("deferred_light");
	m_vertex = m_shader->get_shader_uniform_int("g_vertex");
	m_normal = m_shader->get_shader_uniform_int("g_normal");
	m_albedo = m_shader->get_shader_uniform_int("g_albedo_spec");
	//uniforms
	m_view_pos = m_shader->get_shader_uniform_vec3("view_pos");
	m_ambient_light = m_shader->get_shader_uniform_vec4("ambient_light");
	m_uniform_n_lights_used = m_shader->get_shader_uniform_int("n_lights_used");
	//alloc lights
	m_uniform_lights.resize(m_max_lights);
	//get unfirom lights
    for (unsigned i = 0; i != m_max_lights; ++i)
    {
        m_uniform_lights[i].get_uniform(i, m_shader);
    }
}


void rendering_pass_deferred::draw_pass(glm::vec4&  clear_color,
                                        glm::vec4&  ambient_color,
                                        entity::ptr e_camera,
                                        std::vector< entity::wptr >& lights,
                                        std::vector< entity::wptr >& renderables,
                                        std::vector< entity::ptr >&  entities)
{
    camera::ptr   c_camera = e_camera->get_component<camera>();
    transform_ptr t_camera = e_camera->get_component<transform>();
	const glm::vec4& viewport = c_camera->get_viewport();
    glViewport(viewport.x, viewport.y, viewport.z, viewport.w);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);

	m_g_buffer.bind();
    
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    for (entity::wptr& weak_entity : renderables)
    {
        auto entity = weak_entity.lock();
        entity->get_component<renderable>()->draw(viewport,
                                                  c_camera->get_projection(),
                                                  t_camera->get_matrix_inv(),
                                                  entity->get_component<transform>()->get_matrix(),
                                                  entity->get_component<material>());
	}

	m_g_buffer.unbind();
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
	m_shader->bind();
	//set texture
	m_g_buffer.set_texture_buffer(g_buffer::G_BUFFER_TEXTURE_TYPE_POSITION);
	m_g_buffer.set_texture_buffer(g_buffer::G_BUFFER_TEXTURE_TYPE_NORMAL);
	m_g_buffer.set_texture_buffer(g_buffer::G_BUFFER_TEXTURE_TYPE_ALBEDO);
	//set uniform id
	m_vertex->set_value(0);
	m_normal->set_value(1);
	m_albedo->set_value(2);
	//add info
	m_view_pos->set_value(-t_camera->get_position());
	m_ambient_light->set_value(ambient_color);
    //compute max lights
    unsigned max_lights = std::min(m_max_lights,(unsigned)lights.size());
	m_uniform_n_lights_used->set_value(max_lights);
	//uniform lights
    for (unsigned i = 0; i != max_lights; ++i)
    {
        auto entity = lights[i].lock();
		m_uniform_lights[i].uniform( entity->get_component<light>(),
                                     entity->get_component<transform>()->get_matrix());
	}
	//draw squere
	m_square->draw();
	//bind
	m_shader->unbind();
}