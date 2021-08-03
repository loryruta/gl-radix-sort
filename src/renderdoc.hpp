#pragma once

#include <functional>

namespace renderdoc
{
	void watch(bool capture, std::function<void()> const& f);
}
