#pragma once
import math;
import econ_item_class;

#include <cstdint>
#include "../../utilities/utilities.hpp"

class i_client
{
public:
	c_econ_item_system* get_econ_item_system()
	{
		using function_t = c_econ_item_system*(__fastcall*)(i_client*);
		return (*reinterpret_cast<function_t**>(this))[129](this);
	}

	void set_view_angles(vec3_t angles) {}
};
