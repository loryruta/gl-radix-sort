#pragma once

#include <functional>

namespace rgc::renderdoc
{
	bool is_profiling();

	void watch(bool capture, std::function<void()> const& f);
}
