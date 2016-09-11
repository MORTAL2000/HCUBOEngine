#pragma vertex
//in
layout(location = ATT_POSITIONT) in vec3 position;
//out
out vec4 frag_position;

//uniform model camera position
#pragma include "lib/uniform.glsl"
//include light lib
#pragma include "lib/point_light.glsl"
//uniform light
uniform point_light light;

void main()
{
	vec3 world_position = position * light.m_radius * 1.1 + light.m_position;
	frag_position       = camera.m_projection * camera.m_view * vec4(world_position, 1.0f);
	gl_Position         = frag_position;
}

#pragma fragment
//in
in vec4 frag_position;
//out
out vec4 frag_color;
//uniform
uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedo_spec;
uniform sampler2D g_occlusion;
//uniform model camera position
#pragma include "lib/uniform.glsl"
//include screen utilities
#pragma include "lib/post_processing_utilities.glsl"
//include light lib
#pragma include "lib/point_light.glsl"
//uniform light
uniform point_light light;
//get uv
vec2 get_uv_screen()
{
	return projection_space_to_screen_space(frag_position);
}
//compute shader
void main()
{
    // Retrieve data from gbuffer
    vec4  position  = texture(g_position,    get_uv_screen()).rgba;
    vec3  normal    = texture(g_normal,      get_uv_screen()).rgb;
    vec3  diffuse   = texture(g_albedo_spec, get_uv_screen()).rgb;
    float specular  = texture(g_albedo_spec, get_uv_screen()).a;
    float occlusion = texture(g_occlusion,   get_uv_screen()).r;
    
    //unpack
    normal = normalize(normal * 2.0 - 1.0);
    
    //todo: material
    float shininess = 16.0f;
        
    //view dir
	vec3 view_dir = normalize(camera.m_position - position.xyz);
    
    //result
    point_light_res light_results;
    
    // accumulator
    vec3 diff_col = diffuse * occlusion;

    // then calculate lighting as usual
    compute_point_light(light,
						position,
                        view_dir,
                        normal,
                        shininess,
                        light_results);
    //color
    vec3 lighting = diff_col * (light_results.m_diffuse + light_results.m_specular * specular);    
    
    //output
    frag_color = vec4(lighting,1.0);
}