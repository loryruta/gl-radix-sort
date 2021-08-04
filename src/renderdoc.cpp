
#include "renderdoc.hpp"

#include <renderdoc_app.h>
#define NOMINMAX
#include <windows.h>

RENDERDOC_API_1_1_2* g_handle = nullptr;

void renderdoc_init()
{
	if (g_handle == nullptr)
	{
		if (HMODULE module = GetModuleHandleA("renderdoc.dll"))
		{
			auto RENDERDOC_GetAPI = (pRENDERDOC_GetAPI) GetProcAddress(module, "RENDERDOC_GetAPI");
			int res = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**) &g_handle);
			if (res != 1)
			{
				fprintf(stderr, "Failed to initialize RenderDoc in-application API. Result: `%d`.\n", res);
				fflush(stderr);

				g_handle = nullptr;
				return;
			}
		}
	}
}

bool rgc::renderdoc::is_profiling()
{
	if (g_handle == nullptr) {
		renderdoc_init();
	}
	return g_handle != nullptr;
}

void rgc::renderdoc::watch(bool capture, const std::function<void()>& f)
{
	if (g_handle == nullptr) {
		renderdoc_init();
	}

	if (g_handle && capture) {
		g_handle->StartFrameCapture(NULL, NULL);
	}

	f();

	if (g_handle && capture) {
		g_handle->EndFrameCapture(NULL, NULL);
	}
}


