#include "schema_system.hpp"

#include "../interfaces/interfaces.hpp"
#include "../../utilities/signatures.hpp"
#include "../../utilities/utilities.hpp"

#include <unordered_map>
#include <vector>

namespace {
    struct schema_hash {
        using is_transparent = void;
        std::size_t operator()(std::string_view value) const noexcept {
            return std::hash<std::string_view>{}(value);
        }
    };
    template <typename T>
    using schema_map = std::unordered_map<std::string, T, schema_hash, std::equal_to<>>;
    schema_map<schema_map<schema_system::schema_offset_t>> schema_data;
}

bool schema_system::initialize()
{
	if (!interfaces::schema_system)
	{
		console::log("[-] failed to initialize schema system (no interface)\n");
		return false;
	}

	std::vector<schema_system_type_scope*> scopes;

	scopes.push_back(interfaces::schema_system->global_type_scope());

	std::uint8_t* all_type_scope_address = utilities::pattern_scan(L"schemasystem.dll", ALL_TYPE_SCOPE);

	if (all_type_scope_address)
	{
		std::uint8_t* all_type_scope = utilities::resolve_rip(all_type_scope_address, 3, 7);

		auto scopes_array = *reinterpret_cast<schema_system_type_scope***>(all_type_scope);
		auto scopes_count = *reinterpret_cast<std::uint16_t*>(all_type_scope - 0x8);

		if (scopes_array)
		{
			for (std::uint16_t i = 0; i < scopes_count; i++)
			{
				if (scopes_array[i])
				{
					scopes.push_back(scopes_array[i]);
				}
			}
		}
	}

	for (schema_system_type_scope* scope : scopes)
	{
		if (!scope)
		{
			continue;
		}

		schema_class_list_t* class_list = scope->get_class_container();

		if (!class_list)
		{
			continue;
		}

		std::int32_t block_index = 0;

		for (schema_block_container_t& block_container : class_list->block_containers)
		{
			for (schema_block_t* block = block_container.first_block; block && block_index < class_list->get_num_schema(); block = block->next, block_index++)
			{
				schema_class_binding_t* binding = block->binding;

				if (!binding || !binding->binding_name)
				{
					continue;
				}

				if (!binding->data_array)
				{
					continue;
				}

				for (std::int32_t i = 0; i < static_cast<std::int32_t>(binding->data_array_size); i++)
				{
					schema_class_field_data_t* field = &binding->data_array[i];

					if (!field->field_name)
					{
						continue;
					}

					schema_data[binding->binding_name][field->field_name].class_name = binding->binding_name;
					schema_data[binding->binding_name][field->field_name].property_name = field->field_name;
					schema_data[binding->binding_name][field->field_name].offset = field->field_offset;
				}
			}
		}
	}

	if (schema_data.empty())
	{
		console::log("[-] failed to initialize schema system\n");
		return false;
	}
	return true;
}

bool schema_system::has_schema(std::string_view class_name, std::string_view property_name)
{
    return get_schema(class_name, property_name) != 0;
}

std::uint32_t schema_system::get_schema(std::string_view class_name, std::string_view property_name)
{
    const auto class_it = schema_data.find(class_name);
    if (class_it == schema_data.end()) return 0;
    const auto property_it = class_it->second.find(property_name);
    return property_it == class_it->second.end() ? 0 : property_it->second.offset;
}
