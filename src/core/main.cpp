

#include "CoreTools.h"
#include "Logger.h"
#include "VulkanApp.h"

#define VMA_IMPLEMENTATION
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

int main (int argc, char* argv[])
{
	std::unique_ptr<VulkanApp> vkApp;
	try
	{
		vkApp = std::make_unique<VulkanApp> ();
	}
	catch (const std::runtime_error& e)
	{
		Log.Debug (fmt::format ("ENGINE FAILED TO INITIALIZE\n{}\n", e.what ()));
		return EXIT_FAILURE;
	}

	try
	{
		vkApp->Run ();
	}
	catch (const std::runtime_error& e)
	{
		Log.Error (fmt::format ("Engine quite in main loop\n{}\n", e.what ()));
		return EXIT_FAILURE;
	}

	try
	{
		vkApp.reset ();
	}
	catch (const std::runtime_error& e)
	{
		Log.Error (fmt::format ("Engine quite in destructor\n{}\n", e.what ()));
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}