//
//  resource.h
//  HCubo
//
//  Created by Gabriele on 12/07/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#pragma once
#include <memory>
#include <string>
#include <hcube/config.h>

namespace hcube
{
	class resources_manager;
	class HCUBE_API resource
	{
	public:

		virtual bool load(resources_manager& resources, const std::string& path) = 0;

	};
	using resource_ptr = std::shared_ptr< resource >;
	using resource_uptr = std::unique_ptr< resource >;
	using resource_wptr = std::weak_ptr< resource >;
}