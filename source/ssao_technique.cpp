#include <OpenGL4.h>
#include <ssao_technique.h>

void ssao_technique::init(entity::ptr e_camera, resources_manager& resources)
{
	glm::ivec2 w_size = e_camera->get_component<camera>()->get_viewport_size();
	//load shader
	m_shader = resources.get_shader("ssao_pass");
	m_uniform_kernel = m_shader->get_shader_uniform_array_vec3("samples[0]");
	m_uniform_noise_scale = m_shader->get_shader_uniform_vec2("noise_scale");
	m_uniform_near_far = m_shader->get_shader_uniform_vec2("near_far");
	m_uniform_projection = m_shader->get_shader_uniform_mat4("projection");
	m_position = m_shader->get_shader_uniform_int("g_position");
	m_normal = m_shader->get_shader_uniform_int("g_normal");
	m_noise = m_shader->get_shader_uniform_int("t_noise");
	
	//fbo
	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	//gen texture
	glGenTextures(1, &m_ssao_texture);
	glBindTexture(GL_TEXTURE_2D, m_ssao_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w_size.x, w_size.y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//create frame buffer texture
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ssao_texture, 0);
	//clear
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	//unbind
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	//build kernel
	for (unsigned int i = 0; i != 64; ++i)
	{
		glm::vec3 sample((float)std::rand() / RAND_MAX * 2.0 - 1.0,
			(float)std::rand() / RAND_MAX * 2.0 - 1.0,
			(float)std::rand() / RAND_MAX);
		sample = glm::normalize(sample);
		sample *= (float)std::rand() / RAND_MAX;
		GLfloat scale = GLfloat(i) / 64.0;

		// Scale samples s.t. they're more aligned to center of kernel
		scale = glm::mix(0.1f, 1.0f, scale * scale);
		sample *= scale;
		m_kernel.push_back(sample);
	}

	//noise texture
	std::vector<glm::vec3> noise_buffer;
	for (GLuint i = 0; i < 16; i++)
	{
		noise_buffer.push_back(glm::vec3
		{
			(float)std::rand() / RAND_MAX * 2.0 - 1.0,
			(float)std::rand() / RAND_MAX * 2.0 - 1.0,
			0.0f
		});
	}
	//build noise texture
	glGenTextures(1, &m_noise_texture);
	glBindTexture(GL_TEXTURE_2D, m_noise_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise_buffer.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
}

void ssao_technique::clear()
{
	if (m_fbo)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		//clear
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		//unbind
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void ssao_technique::bind(entity::ptr e_camera, g_buffer& buffer)
{
	camera::ptr   c_camera = e_camera->get_component<camera>();
	//enable fbo
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	//bind shader
	m_shader->bind();
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//bind kernel/proj/scale noise
	m_uniform_kernel->set_value(&m_kernel[0], m_kernel.size());
	m_uniform_projection->set_value(c_camera->get_projection());
	m_uniform_near_far->set_value(c_camera->get_near_and_far());
	m_uniform_noise_scale->set_value((glm::vec2)c_camera->get_viewport_size() / glm::vec2(4, 4));
	//set g_buffer 
	buffer.set_texture_buffer(g_buffer::G_BUFFER_TEXTURE_TYPE_POSITION);//0
	buffer.set_texture_buffer(g_buffer::G_BUFFER_TEXTURE_TYPE_NORMAL);  //1
	//set noise												
	glActiveTexture(GL_TEXTURE0 + 2);//2
	glBindTexture(GL_TEXTURE_2D, m_noise_texture);
	//set uniform id
	m_position->set_value(0);
	m_normal->set_value(1);
	m_noise->set_value(2);
	//////////////////////////////////////////////////////////////////////////////////////////////////////
}

void ssao_technique::destoy()
{
	if (m_shader) m_shader = nullptr;
	
	if (m_ssao_texture) glDeleteTextures(1, &m_ssao_texture);
	
	if (m_noise_texture) glDeleteTextures(1, &m_noise_texture);
	
	if (m_fbo) glDeleteFramebuffers(1, &m_fbo);

	m_last_n_text = 0;
	m_ssao_texture = 0;
	m_noise_texture = 0;
	m_ssao_texture = 0;
	m_uniform_kernel = nullptr;
	m_uniform_near_far = nullptr;
	m_uniform_noise_scale = nullptr;
	m_uniform_projection = nullptr;
	m_position = nullptr;
	m_normal = nullptr;
	m_noise = nullptr;
}

void ssao_technique::unbind(entity::ptr e_camera, g_buffer& buffer)
{
	//unbind
	m_shader->unbind();
	//disable g_buffer 
	buffer.disable_texture(g_buffer::G_BUFFER_TEXTURE_TYPE_POSITION);//0
	buffer.disable_texture(g_buffer::G_BUFFER_TEXTURE_TYPE_NORMAL);  //1
	//disable noise												
	glActiveTexture(GL_TEXTURE0 + 2);//2
	glBindTexture(GL_TEXTURE_2D, 0);
	//disable fbo
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ssao_technique::set_texture(GLenum n_tex)
{
	m_last_n_text = n_tex;
	glActiveTexture(GL_TEXTURE0 + n_tex);
	glBindTexture(GL_TEXTURE_2D, m_ssao_texture);
}

void ssao_technique::disable_texture()
{
	glActiveTexture(GL_TEXTURE0 + m_last_n_text);
	glBindTexture(GL_TEXTURE_2D, 0);
	m_last_n_text = 0;
}

