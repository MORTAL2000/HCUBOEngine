//
//  transform.h
//  HCubo
//
//  Created by Gabriele on 23/07/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#pragma once
#include <hcube/config.h>
#include <hcube/math/vector_math.h>
#include <hcube/core/component.h>
#include <hcube/render/shader.h>
#include <hcube/data/property.h>
#include <hcube/core/component_register.h>

namespace hcube
{
	class HCUBE_API transform : public component
	{

		HCUBE_COMPONENT_DEC(transform)

	public:

		enum message_ids
		{
			MSG_DIRTY = 1,
		};

		void on_attach(entity& ent);
		void on_detach();
		void on_message(const message& message);


		void look_at(const vec3& eye, const vec3& center, const vec3& up);
		void translation(const vec3& vector);
		void move(const vec3& vector);
		void turn(const quat& rot);

		void position(const vec3& pos);
		void rotation(const quat& rot);
		void scale(const vec3& pos);

		vec3 get_position() const;
		quat get_rotation() const;
		vec3 get_scale() const;

		vec3 get_global_position();
		quat get_global_rotation();
		vec3 get_global_scale();

		mat4 const& get_local_matrix();
		mat4 const& get_matrix();

		inline operator const mat4& ()
		{
			return get_matrix();
		}

		inline bool is_dirty() const
		{
			return m_tranform.m_dirty;
		}

		virtual component_ptr copy() const;

		HCUBE_DEFINE_PROPERTIES(
			make_property_const_get_set(&transform::get_position, &transform::rotation, "position"),
			make_property_const_get_set(&transform::get_rotation, &transform::rotation, "rotation"),
			make_property_const_get_set(&transform::get_scale,    &transform::scale,    "scale")
		)

	private:
		//info
		struct local_tranform
		{
			bool      m_dirty;
			vec3 m_position;
			quat m_rotation;
			vec3 m_scale{ 1.0f, 1.0f, 1.0f };
		}
		m_tranform;
		//send dirty
		void    set_dirty();
		void    send_dirty();
		//matrix
		mat4  m_model_local;
		mat4  m_model_global;
		//compute
		void compute_matrix();

	};

	using transform_ptr = std::shared_ptr< transform >;
	using transform_uptr = std::unique_ptr< transform >;
	using transform_wptr = std::weak_ptr< transform >;

	template < class... Args >
	static inline transform_ptr transform_snew(Args&&... args)
	{
		return std::make_shared< transform >(args...);
	}

	struct uniform_transform
	{
		context_uniform* m_uniform_model   { nullptr };
		context_uniform* m_uniform_position{ nullptr };
		context_uniform* m_uniform_rotation{ nullptr };
		context_uniform* m_uniform_scale   { nullptr };

		void get_uniform(shader::ptr shader);

		void uniform(transform_wptr transform) const;

		bool is_valid() const;

	protected:

		bool m_valid{ false };

	};
}