#pragma vertex
//in
layout(location = ATT_POSITIONT) in vec3 vertex;
layout(location = ATT_NORMAL0)   in vec3 normal;
layout(location = ATT_TEXCOORD0) in vec2 uvcoord;
layout(location = ATT_TANGENT0)  in vec3 tangent;
layout(location = ATT_BINORMAL0) in vec3 bitangents;
//out 
out vec4 frag_vertex_model;
out vec3 frag_vertex;
out vec2 frag_uvcoord;
out mat3 tbn;
//uniform
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
	//vertex
	vec4 model_vertex = model * vec4(vertex, 1.0);
	vec4 view_vertex  = view * model_vertex;
	frag_vertex       = view_vertex.xyz;
	frag_vertex_model = model_vertex.xyzw;
	gl_Position 	  = projection * view_vertex;
    //normal
    mat3 normal_mat = transpose(inverse(mat3(view * model)));
    //pass T/B/N
    vec3 t_pixel = normalize(normal_mat * tangent);
    vec3 n_pixel = normalize(normal_mat * normal);
	vec3 b_pixel = normalize(normal_mat * bitangents);
    //..
    tbn = transpose(mat3(t_pixel.x, b_pixel.x, n_pixel.x,
				         t_pixel.y, b_pixel.y, n_pixel.y,
                         t_pixel.z, b_pixel.z, n_pixel.z));
	//uvmap
	frag_uvcoord = uvcoord;
}

#pragma fragment
//in
in vec4 frag_vertex_model;
in vec3 frag_vertex;
in vec2 frag_uvcoord;
in mat3 tbn;
//out
#pragma include "output_fragment.glsl"
//uniform
uniform vec4      color;
uniform float     mask = 0.0;
uniform sampler2D diffuse_map;
uniform sampler2D normal_map;
//uniform matrix
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

vec3 compute_normal()
{
    //get normal texture
    vec3 text_normal = normalize( texture(normal_map, frag_uvcoord).rgb * 2.0f - 1.0f );
    //return
    return tbn*text_normal;
}

void main()
{
	//color
	vec4 texture_color = texture(diffuse_map, frag_uvcoord);
	if (texture_color.a <= mask) discard;
	//outputs
	output_fragment(frag_vertex_model, frag_vertex, compute_normal(), (texture_color*color), 1.0);
}