#pragma once
import math;

namespace console {
	void initialize();
	void log(const char* message...);
	void console_color_msg(color_t color, const char* message...);
}
