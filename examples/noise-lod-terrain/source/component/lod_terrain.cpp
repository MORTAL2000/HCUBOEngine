#include <iostream>
#include <component/lod_terrain.h>
#include <hcube/geometries/intersection.h>
#include <hcube/math/tangent_space_calculation.h>
#include <hcube/render/rendering_system.h>
#include <hcube/render/OpenGL4.h>
#define HCUBE_USE_GPU_TERRAIN

namespace hcube
{
	//vertex data description CPU
	struct alignas(16) terrain_vertex
	{
		vec3 m_position;
		vec2 m_uvmap;
		
		terrain_vertex(){}
		terrain_vertex(const vec3& in_vertex,
			           const vec2& in_uvmap)
		{
			m_position = in_vertex;
			m_uvmap = in_uvmap;
		}

	};

	//node methos
	#pragma region node
	void lod_terrain::node::set(const lod_terrain::node::build_info& info)
	{
#if 0
		std::cout << "--------------------------------------------------------------\n";
		std::cout << "start: " << info.m_start.x << ", " << info.m_start.y << "\n";
		std::cout << "end: " << info.m_end.x << ", " << info.m_end.y << "\n";
#endif
		//save
		m_info = info;
		//ref
		const vec2& _start  = info.m_start;
		const vec2& _end    = info.m_end;
		//compute box size
		vec3
		box_size(_end.x - _start.x, 1.0, _end.y - _start.y);
		box_size /= 2.0f;
		//compute box position
		vec3
		box_pos((_start.x + _end.x)*0.5,
				0.0,
				(_start.y + _end.y)*0.5);
		box_pos -= vec3(0.5, 0.0, 0.5);
		//set OBB
		m_box.set
		(
			mat3(1), //rotation
			box_pos, //position
			box_size //extesion
		);
		//////////////////////////////////////////////////////////////////////////////////////////////////
	}
	#pragma endregion

	//build nodes
	#pragma region node_build

	void lod_terrain::build_index_buffer()
	{
		//mid size
		ivec2 mid_detail = m_detail - ivec2(2, 2);
		//buffer
		std::vector< unsigned int >   indices(mid_detail.x * mid_detail.y * 6);
		//init index
		int i = 0;
		//#pragma omp parallel for num_threads(4)
		for (int y = 1; y < m_detail.y-1; ++y)
		for (int x = 1; x < m_detail.x-1; ++x)
		{
			// top
			const unsigned int point = y * m_detail_vertexs.x + x;
			const unsigned int point1 = point + 1;
			// bottom
			const unsigned int point2 = (y + 1) * m_detail_vertexs.x + x;
			const unsigned int point3 = point2 + 1;
			//add tri 1
			indices[i++] = point;
			indices[i++] = point3;
			indices[i++] = point1;
			//add tri 2
			indices[i++] = point;
			indices[i++] = point2;
			indices[i++] = point3;
		}
		m_ib_middle_size = indices.size();		
		//alloc index buffer
		m_ibuffer_middle = render::create_stream_IBO(indices.data(), indices.size());
		//top / bottom
		for (int edge = 0; edge < 2; ++edge)
		{
			/*
			          point 2
			point 0 *....*....* point 4
			        |\   |   /|
		            | \2 | 3/ |
			        |  \ | /  |
			        | 1 \|/ 4 |
			point 1 *....*....* point 5
			          point 3
			*/
			int left_order_0[] 
			{ 
				0,1,3,
				0,3,2,
				2,3,4,
				4,3,5 
			};
			int right_order_0[]
			{
				0,3,1,
				0,2,3,
				2,4,3,
				4,5,3
			};
			/*
				point 0 *....x....* point 3
						|\       /|
						| \  2  / |
						|  \   /  |
					    | 1 \ / 3 |
			   point 1 *.....*....* point 4
				          point 2
			*/
			int left_order_1[]
			{
				0,1,2,
				0,2,3,
				3,2,4
			};
			int right_order_1[]
			{
				0,2,1,
				0,3,2,
				3,4,2
			};
			//index
			int* index_0 = (edge == NODE_EDGE_TOP) ? left_order_0 : right_order_0;
			int* index_1 = (edge == NODE_EDGE_TOP) ? left_order_1 : right_order_1;
			//y
			int y  = (edge == NODE_EDGE_TOP) ? 0 : m_detail.y;
			int y1 = (edge == NODE_EDGE_TOP) ? 1 : m_detail.y - 1;
			//////////////////////////////////////////////////////////////////////////////////////////
			//alloc
			indices = std::vector< unsigned int >(m_detail.x * 6 - 6, 0);
			//compute
			for (int i = 0, x = 0; x < m_detail.x; x += 2)
			{
				// top 
				const unsigned int points[]
				{
					y  * m_detail_vertexs.x + x,
					y1 * m_detail_vertexs.x + x,

					points[0] + 1,
					points[1] + 1,

					points[2] + 1,
					points[3] + 1,
				};
				//jump first
				if (x > 0)
				{
					//add tri 1
					indices[i++] = points[index_0[0]];
					indices[i++] = points[index_0[1]];
					indices[i++] = points[index_0[2]];
				}
				//add tri 2
				indices[i++] = points[index_0[3]];
				indices[i++] = points[index_0[4]];
				indices[i++] = points[index_0[5]];
				//add tri 3
				indices[i++] = points[index_0[6]];
				indices[i++] = points[index_0[7]];
				indices[i++] = points[index_0[8]];
				//jump last
				if (x < m_detail.x - 2)
				{
					//add tri 3
					indices[i++] = points[index_0[9 ]];
					indices[i++] = points[index_0[10]];
					indices[i++] = points[index_0[11]];
				}
			}
			//width
			m_ib_edge_size[edge][0] = indices.size();
			//alloc index buffer
			m_ibuffer_edges[edge][0] = render::create_stream_IBO(indices.data(), indices.size());
			//////////////////////////////////////////////////////////////////////////////////////////
			//alloc
			indices = std::vector< unsigned int >(m_detail.x / 2 * 9 - 6, 0);
			//compute
			for (int i = 0, x = 0; x < m_detail.x; x += 2)
			{
				// 0
				const unsigned int points[] =
				{
					y  * m_detail_vertexs.x + x,
					y1 * m_detail_vertexs.x + x,
					points[1] + 1,
					points[0] + 2,
					points[1] + 2,
				};
				//jump first
				if (x > 0)
				{
					//add tri 1
					indices[i++] = points[index_1[0]];
					indices[i++] = points[index_1[1]];
					indices[i++] = points[index_1[2]];
				}
				//add tri 2
				indices[i++] = points[index_1[3]];
				indices[i++] = points[index_1[4]];
				indices[i++] = points[index_1[5]];
				//jump last
				if (x < m_detail.x-2)
				{
					//add tri 3
					indices[i++] = points[index_1[6]];
					indices[i++] = points[index_1[7]];
					indices[i++] = points[index_1[8]];
				}
			}
			//width
			m_ib_edge_size[edge][1] = indices.size();
			//alloc index buffer
			m_ibuffer_edges[edge][1] = render::create_stream_IBO(indices.data(), indices.size());
		}			
		//left / right
		for (int edge = 2; edge < 4; ++edge)
		{
			//init index
			int i = 0;
			////////////////
			/*
			 p0 ____ p1
			   |\   |
			   | \ 0|
			   | 1\ |
			p2 |___\| p3
			   |   /|
			   | 2/ |
			   | / 3|
			   |/___|
			p4       p5
			*/
			int right_order_0[]
			{
				0,1,3,
				0,3,2,
				2,3,4,
				4,3,5
			};
			int left_order_0[]
			{
				0,3,1,
				0,2,3,
				2,4,3,
				4,5,3
			};
			/*
			p0 ____ p1
			  |\   |
			  | \ 0|
			  |  \ |
			x | 2 \| p2
			  |   /|
			  |  / |
			  | / 3|
			  |/___|
			p3       p4
			*/
			int right_order_1[]
			{
				0,1,2,
				0,2,3,
				3,2,4
			};
			int left_order_1[]
			{
				0,2,1,
				0,3,2,
				3,4,2
			};
			//index
			int* index_0 = (edge == NODE_EDGE_LEFT) ? left_order_0 : right_order_0;
			int* index_1 = (edge == NODE_EDGE_LEFT) ? left_order_1 : right_order_1;
			//x
			int x  = (edge == NODE_EDGE_LEFT) ? 0 : m_detail.x;
			int x1 = (edge == NODE_EDGE_LEFT) ? 1 : m_detail.x - 1;
			//alloc
			indices = std::vector< unsigned int >(m_detail.y * 6 - 6, 0);
			//compute
			for (int i = 0, y = 0; y < m_detail.y; y += 2)
			{
				const unsigned int points[] =
				{
					y * m_detail_vertexs.x + x,
					y * m_detail_vertexs.x + x1,

					(y + 1) * m_detail_vertexs.x + x,
					(y + 1) * m_detail_vertexs.x + x1,

					(y + 2) * m_detail_vertexs.x + x,
					(y + 2) * m_detail_vertexs.x + x1
				};
				//jump first
				if (y > 0)
				{
					//add tri 1
					indices[i++] = points[index_0[0]];
					indices[i++] = points[index_0[1]];
					indices[i++] = points[index_0[2]];
				}
				//add tri 2
				indices[i++] = points[index_0[3]];
				indices[i++] = points[index_0[4]];
				indices[i++] = points[index_0[5]];
				//add tri 3
				indices[i++] = points[index_0[6]];
				indices[i++] = points[index_0[7]];
				indices[i++] = points[index_0[8]];
				//jump last
				if (y < m_detail.y - 2)
				{
					//add tri 3
					indices[i++] = points[index_0[9]];
					indices[i++] = points[index_0[10]];
					indices[i++] = points[index_0[11]];
				}
			}
			//width
			m_ib_edge_size[edge][0] = indices.size();
			//alloc index buffer
			m_ibuffer_edges[edge][0] = render::create_stream_IBO(indices.data(), indices.size());
			//////////////////////////////////////////////////////////////////////////////////////////
			//alloc
			indices = std::vector< unsigned int >(m_detail.y / 2 * 9 - 6, 0);
			//compute
			for (int i = 0, y = 0; y < m_detail.y; y += 2)
			{
				const unsigned int points[] =
				{
					y * m_detail_vertexs.x + x,
					y * m_detail_vertexs.x + x1,

					(y + 1) * m_detail_vertexs.x + x1,

					(y + 2) * m_detail_vertexs.x + x,
					(y + 2) * m_detail_vertexs.x + x1
				};
				//jump first
				if (y > 0)
				{
					//add tri 1
					indices[i++] = points[index_1[0]];
					indices[i++] = points[index_1[1]];
					indices[i++] = points[index_1[2]];
				}
				//add tri 2
				indices[i++] = points[index_1[3]];
				indices[i++] = points[index_1[4]];
				indices[i++] = points[index_1[5]];
				//jump last
				if (y < m_detail.y - 2)
				{
					//add tri 3
					indices[i++] = points[index_1[6]];
					indices[i++] = points[index_1[7]];
					indices[i++] = points[index_1[8]];
				}
			}
			//width
			m_ib_edge_size[edge][1] = indices.size();
			//alloc index buffer
			m_ibuffer_edges[edge][1] = render::create_stream_IBO(indices.data(), indices.size());
		}
	}

	void lod_terrain::build()
	{
		//vertex data description GPU
		m_layout =
		render::create_IL
		({
			sizeof(terrain_vertex),
			{
				attribute{ ATT_POSITIONT, AST_FLOAT3, offsetof(terrain_vertex, m_position) },
				attribute{ ATT_TEXCOORD0, AST_FLOAT2, offsetof(terrain_vertex, m_uvmap) }
			}
		});
		//buffer
		std::vector< terrain_vertex > vertices(m_detail_vertexs.x*m_detail_vertexs.y);
		//gpu will compute the height 
		for (unsigned int y = 0; y != m_detail_vertexs.y; ++y)
		for (unsigned int x = 0; x != m_detail_vertexs.x; ++x)
		{
			vertices[y* m_detail_vertexs.x + x].m_position.x = float(x);
			vertices[y* m_detail_vertexs.x + x].m_position.y = float(y);
		}
		//set obb
		set_bounding_box(obb(mat3(1), vec3(0, 0, 0), vec3(0.5, 0.5, 0.5)));
		//save sizes
		m_vb_size = vertices.size();
		//alloc vertex buffer
		m_vbuffer = render::create_stream_VBO((unsigned char*)vertices.data(), sizeof(terrain_vertex), vertices.size());
		//index buffer
		build_index_buffer();
		//errors
		HCUBE_RENDER_PRINT_ERRORS
		//build tree
		build_tree();
		//errors
		HCUBE_RENDER_PRINT_ERRORS
	}

	void lod_terrain::build_tree()
	{
		//alloc all and link
		alloc_all();
		link_all_childs();
		link_all_nears();
		level_to_all_nodes();
		//compute root 
		get_node(0, 0, 0)->set({ vec2(0.0,0.0), vec2(1.0,1.0), m_detail });
		//build childs
		build_tree(get_node(0, 0, 0), 1);
	}

	void lod_terrain::build_tree
	(
		link<node>& parent,
		unsigned int  level
	)
	{
		//safe: 
		if (level == m_levels) return;
		//compute local size
		auto& p_info  = parent->m_info;
		vec2 p_g_size = p_info.m_end - p_info.m_start;
		vec2 l_size   = p_g_size / 2.0f;
		//coord
		{
			vec2 start = p_info.m_start;
			vec2 end = start + l_size;
			(*parent->m_chils[NODE_CHILD_TOP_LEFT])->set({ start, end, m_detail });
		}
		{
			vec2 start = p_info.m_start + vec2(p_g_size.x / 2.0f, 0);
			vec2 end = start + l_size;
			(*parent->m_chils[NODE_CHILD_TOP_RIGHT])->set({ start, end, m_detail });
		}
		{
			vec2 start = p_info.m_start + vec2(0, p_g_size.y / 2.0f);
			vec2 end = start + l_size;
			(*parent->m_chils[NODE_CHILD_BOTTOM_LEFT])->set({ start, end, m_detail });
		}
		{
			vec2 start = p_info.m_start + p_g_size / 2.0f;
			vec2 end = start + l_size;
			(*parent->m_chils[NODE_CHILD_BOTTOM_RIGHT])->set({ start, end, m_detail });
		}
		//next levels
		++level;
		//call for all childs
		if (level < m_levels)
		for (size_t i = 0; i != NODE_CHILD_MAX; ++i)
			build_tree((*parent->m_chils[i]), level);
	}

	//alloc nodes
	void lod_terrain::alloc_all()
	{
		//alloc all levels
		m_nodes.resize(m_levels);
		//alloc all nodes
		for (unsigned int l = 0; l != m_levels; ++l)
		{
			m_nodes[l].resize(std::pow(4, l));
		}
	}
	
	void lod_terrain::level_to_all_nodes()
	{
		for (unsigned int l = 0; l != m_levels; ++l)
		for (unsigned int n = 0; n != m_nodes[l].size(); ++n)
		{
			m_nodes[l][n]->m_level = l;
		}
	}

	link<lod_terrain::node>& lod_terrain::get_node(unsigned int level, unsigned int x, unsigned int y)
	{
		unsigned int level_w_size = std::pow(2, level);
		return m_nodes[level][y * level_w_size + x];
	}

	void lod_terrain::link_childs(unsigned int level, unsigned int x, unsigned int y)
	{
		link<node>& thiz = get_node(level, x, y);
		thiz->m_chils[NODE_CHILD_TOP_LEFT]	   = &get_node(level + 1, x * 2 + 0, y * 2 + 0);
		thiz->m_chils[NODE_CHILD_TOP_RIGHT]    = &get_node(level + 1, x * 2 + 1, y * 2 + 0);
		thiz->m_chils[NODE_CHILD_BOTTOM_LEFT]  = &get_node(level + 1, x * 2 + 0, y * 2 + 1);
		thiz->m_chils[NODE_CHILD_BOTTOM_RIGHT] = &get_node(level + 1, x * 2 + 1, y * 2 + 1);
		//link to parent
		//for (node_child i : std::vector<node_child>{ NODE_CHILD_TOP_LEFT , NODE_CHILD_TOP_RIGHT, NODE_CHILD_BOTTOM_LEFT, NODE_CHILD_BOTTOM_RIGHT })
		for(unsigned int i = 0; i!=4; ++i)
		{ 
			(*thiz->m_chils[i])->m_parent	     = &thiz; 
			(*thiz->m_chils[i])->m_in_parent_is = (node_child)i;
		};
	}

	void lod_terrain::link_all_childs()
	{
		//root
		link_childs(0, 0, 0);
		//all
		for (unsigned int level = 1; level < m_nodes.size() - 1; ++level)
		{
			//len
			unsigned int level_w_size = std::pow(2, level);
			//for all
			for (unsigned int y = 0; y != level_w_size; ++y)
			for (unsigned int x = 0; x != level_w_size; ++x)
			{
				link_childs(level, x, y);
			}
		}
	}

	void lod_terrain::link_all_nears()
	{
		//all
		for (unsigned int level = 1; level < m_nodes.size(); ++level)
		{
			//len
			unsigned int level_w_size = std::pow(2, level);
			//for all
			for (unsigned int y = 0; y != level_w_size; ++y)
			for (unsigned int x = 0; x != level_w_size; ++x)
			{
				link<node>& thiz = get_node(level, x, y);
				if (y != (level_w_size - 1)) thiz->m_neighbors[NODE_EDGE_BOTTOM] = &get_node(level, x, y + 1);
				if (y != 0)                  thiz->m_neighbors[NODE_EDGE_TOP]    = &get_node(level, x, y - 1);
				if (x != 0)                  thiz->m_neighbors[NODE_EDGE_LEFT]   = &get_node(level, x - 1, y);
				if (x != (level_w_size - 1)) thiz->m_neighbors[NODE_EDGE_RIGHT]  = &get_node(level, x + 1, y);
			}
		}
	}


	#pragma endregion

	//terrain
	#pragma region terrain
	lod_terrain::lod_terrain(vec2 detail, unsigned int levels)
	{
		m_detail		 = detail;
		m_detail_vertexs = detail + vec2(1,1);
		m_levels		 = levels;
		build();
		set_support_culling(true);
	}
	
	lod_terrain::~lod_terrain()
	{
		if (m_layout)
		{
			render::delete_IL(m_layout);
		}
		if (m_ibuffer_middle)
		{
			render::delete_IBO(m_ibuffer_middle);
		}
		for (int i = 0; i != 2; ++i)
		for (int e = 0; e != NODE_EDGES; ++e)
		{
			if (m_ibuffer_edges[e][i]) render::delete_IBO(m_ibuffer_edges[e][i]);
		}
		if (m_vbuffer)
		{
			render::delete_VBO(m_vbuffer);
		}
	}
	#pragma endregion
	
	//draw
	#pragma region draw
	//overload renderable::draw
	void lod_terrain::draw(rendering_system& rsystem, entity::ptr view) 
	{
		//get camera
		entity::ptr e_camera = rsystem.get_current_draw_camera();
		//get components
		auto c_camera = e_camera->get_component<camera>();
		auto t_camera = e_camera->get_component<transform>();
		auto t_this   = get_entity()->get_component<transform>();
		//this model
		mat4 m4_model = get_entity()->get_component<transform>()->get_matrix();
		//fov
		/*
		a = std::tan(fov / h);

		Here, fov and  h represent  the  camera�s  field  of  view  and  viewport�s  width  in  pixels,  respectively. 
		This makes a approximately the size of one world unit perpendicular to the camera  per  screen  pixel  per  world  unit  in  the  
		direction  of  the  camera 
		*/
		float fov = std::atan(1.0 / c_camera->get_projection()[1][1]) * 2.0f;
		float h   = c_camera->get_viewport_size().x;
		float camera_a = std::tan(fov / h);
		float camera_e = c_camera->get_viewport_size().x;
		//draw
		compute_objects_to_draw_in_all_levels
		(
			camera_a,
			camera_e,
			t_camera->get_global_position(), 
			c_camera->get_frustum(),
			m4_model
		);
		//draw
		for (const node& thiz_node : m_draw_queue)
		{
			if (thiz_node.m_state == NODE_DRAW)
			{
				//uv range in terrain
				vec2  uv_area = thiz_node.m_info.m_end - thiz_node.m_info.m_start;
				vec2  uv_step = uv_area / (vec2(m_detail_vertexs) - vec2(1, 1));
				if (material_ptr mat = get_material())
				{
					if (context_shader* curr_shader = render::get_bind_shader())
					{
						if (auto* u_offset = render::get_uniform(curr_shader, std::string("uv_height_offset")))
							u_offset->set_value(thiz_node.m_info.m_start);
						if (auto* u_step = render::get_uniform(curr_shader, std::string("uv_height_step")))
							u_step->set_value(uv_step);
						
					}
				}				 
				//////////////////////////////////////////////////////////////////////////////////////////////////////////////
				render::bind_VBO(m_vbuffer);
				render::bind_IL(m_layout);
				//draw center
				render::bind_IBO(m_ibuffer_middle);
				render::draw_elements(DRAW_TRIANGLES, m_ib_middle_size);
				render::unbind_IBO(m_ibuffer_middle);
				//edges
				for(int edge = 0; edge < NODE_EDGES; ++edge)
				{
					int i = !thiz_node.is_draw_node_of_edge((node_edges)edge);
					render::bind_IBO(m_ibuffer_edges[edge][i]);
					render::draw_elements(DRAW_TRIANGLES, m_ib_edge_size[edge][i]);
					render::unbind_IBO(m_ibuffer_edges[edge][i]);
				}				
				//unbind
				render::unbind_IL(m_layout);
				render::unbind_VBO(m_vbuffer);
				HCUBE_RENDER_PRINT_ERRORS
				//////////////////////////////////////////////////////////////////////////////////////////////////////////////
			}
		}
	}


	void lod_terrain::compute_objects_to_draw_in_all_levels
	(
		const float camera_a,
		const float camera_e,
		const vec3& camera_position,
		const frustum& frustum,
		const mat4& model_view
	) 
	{
		//all no draw
		for (auto& leve_nodes : m_nodes)
		for (auto& thiz_node : leve_nodes)
		{
			thiz_node.next_to_null();
			thiz_node->m_state = NODE_NOT_DRAW;
		}
		//clear queue
		m_draw_queue.clear();
		m_child_to_draw_queue.clear();
		//root
		compute_object_to_draw_of_a_node
		(
			get_node(0, 0, 0),
			camera_a, 
			camera_e, 
			camera_position,
			frustum, 
			model_view
		);
		//
		int n = 0; 
		link<node>* ln_node;
		link<node>* ln_next;
		//queue
		for(ln_node = m_child_to_draw_queue.top();
			ln_node;
			ln_node = ln_next)
		{
			//get next (safe)
			ln_next = ln_node->get_next();
			//test a node (n.b. this method can break the queue)
			compute_object_to_draw_of_a_node
			(
				*ln_node,
				camera_a,
				camera_e,
				camera_position,
				frustum,
				model_view
			);
		}
		//clean the broken queue
		m_child_to_draw_queue.clear();
	}

	bool lod_terrain::compute_object_to_draw_of_a_node
	(
		link<node>& ln_node,
		const float camera_a,
		const float camera_e,
		const vec3& camera_position,
		const frustum& frustum,
		const mat4& model
	)
	{
		//copy box
		obb this_box = ln_node->m_box;
		//compute box
		this_box.applay(model);
		//event, intersection
		auto intersection_res = intersection::check(frustum, this_box);
		//is inside?
		if (intersection_res != intersection::OUTSIDE)
		{
			//level
			if (m_levels == (ln_node->m_level+1))
			{
				//draw this
				ln_node->m_state = NODE_DRAW;
				m_draw_queue.push(ln_node);
				return true;
			}
			//if near node draw parent i must to be draw
			for (unsigned char n = 0; n != NODE_EDGES; ++n)
			{
				if (ln_node->m_neighbors[n])
				if ((*ln_node->m_neighbors[n])->m_parent)
				if ((*(*ln_node->m_neighbors[n])->m_parent)->m_state == NODE_DRAW)
				{
					ln_node->m_state = NODE_DRAW;
					m_draw_queue.push(ln_node);
					return true;
				}
			}
			/*
				L = (a*d / s) * e

				s is  the  fixed  world  distance  between  neighboring  height  samples  in  the
				tile, or the grid cell size.

				d represents the distance between the camera position and the closest  point  on  the  axis-aligned
				box  formed  by  the  tile�s  position,
				size  and  full  height range.

				Assuming the heightfield is viewed from above, ad/s is the (maximum) amount of grid cells per pixel.
				e is the maximum allowed screen �error� in pixels
			*/
			//distance
			const float d = distance(camera_position, this_box.closest_point(camera_position));
			const float s = length(vec2(this_box.get_extension().x, this_box.get_extension().z));
			const float L = ((camera_a*d) / (s*2.0)) * camera_e;
			const const float max_l = 1.5;
			const const float inv_max_l = 1.0 / max_l;
			//levels
			if (L > inv_max_l)
			{
				//draw this
				ln_node->m_state = NODE_DRAW;
				m_draw_queue.push(ln_node);
				return true;
			}
			else
			{
				//draw childs
				ln_node->m_state = NODE_DRAW_CHILD;
				//edges
				for (int child = 0; child < NODE_CHILD_MAX; ++child)
				{
					if (ln_node->m_chils[child])
						m_child_to_draw_queue.push(ln_node->m_chils[child]);
				}
				//return true
				return true;
			}
		}
		return false;
	}
	#pragma endregion

	#pragma region debug
	void lod_terrain::compute_object_to_draw_debug(unsigned int level_to_draw)
	{	
		//all no draw
		for (auto& leve_nodes : m_nodes)
		for (auto& thiz_node : leve_nodes)
		{
			thiz_node.next_to_null();
			thiz_node->m_state = NODE_NOT_DRAW;
		}
		//clear queue
		m_draw_queue.clear();
		//level size
		unsigned int level_w_size = std::pow(2, level_to_draw);
		//for all
		for (unsigned int y = 0; y != level_w_size; ++y)
		for (unsigned int x = 0; x != level_w_size; ++x)
		{
			//get node
			link<node>& current = get_node(level_to_draw, x, y);
			current->m_state = NODE_DRAW;
			m_draw_queue.push(current);
		}
	}
	#pragma endregion
}
