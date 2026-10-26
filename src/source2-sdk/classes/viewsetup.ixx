module;

#include <cstddef>
#include <cstdint>

export module viewsetup_class;


export {
class c_view_setup {
public:
	char pad1[0x498];
	float fov;
};
static_assert(offsetof(c_view_setup, fov) == 0x498, "c_view_setup::fov has wrong offset");
}
