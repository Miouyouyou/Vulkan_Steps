#include "vulkan/vulkan.h"
#include <stdio.h>

#include <myy/log_helpers.h>

/* I'm writing this code while reading :
 * https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-1
 *
 * So yeah, the code will be extremely similar, if not identical.
 */
int myy_vulkan_create_instance(
	VkInstance * __restrict instance_address)
{
	VkApplicationInfo application_info = {
		VK_STRUCTURE_TYPE_APPLICATION_INFO,             // VkStructureType            sType
		NULL,                                           // const void                *pNext
		"Myy Vulkan : First level !",                   // const char                *pApplicationName
		VK_MAKE_VERSION( 0, 1, 0 ),                     // uint32_t                   applicationVersion
		"Myy Vulkan Inexistent engine !",               // const char                *pEngineName
		VK_MAKE_VERSION( 0, 0, 1 ),                     // uint32_t                   engineVersion
		VK_API_VERSION_1_0                              // uint32_t                   apiVersion
	};

	/* The documentation states that the structure type defines the
	 * structure size... However, does the documentation state the
	 * pointer size ?
	 * 32 bits pointer being 4 bytes long and 64 bytes being 8 bytes
	 * long, this is quite different...
	 */
	VkInstanceCreateInfo instance_create_info = {
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // VkStructureType            sType
		NULL,                                   // const void*                pNext
		0,                                      // VkInstanceCreateFlags      flags
		&application_info,                      // const VkApplicationInfo   *pApplicationInfo
		0,                                      // uint32_t                   enabledLayerCount
		NULL,                                   // const char * const        *ppEnabledLayerNames
		0,                                      // uint32_t                   enabledExtensionCount
		NULL                                    // const char * const        *ppEnabledExtensionNames
	};

	/* VkResult vkCreateInstance(
	 *     const VkInstanceCreateInfo*                 pCreateInfo,
	 *     const VkAllocationCallbacks*                pAllocator,
	 *     VkInstance*                                 pInstance);
	 */
	int const success =	(
		vkCreateInstance(&instance_create_info, NULL, instance_address)
		== VK_SUCCESS);

	return success;
}

void myy_vulkan_devices_print_basic_properties(
	uint_fast32_t n_devices,
	VkPhysicalDevice const * __restrict devices)
{
	VkPhysicalDeviceProperties current_device_props;

	while(n_devices--) {
		VkPhysicalDevice current_device = *devices++;

		vkGetPhysicalDeviceProperties(
			current_device, &current_device_props);
		uint32_t const
			api_version = current_device_props.apiVersion,
			driver_version = current_device_props.driverVersion;
		log_entry(
			"Name : %s\n"
			"\tapiVersion    : %d,%d,%d\n"
			"\tdriverVersion : %d,%d,%d\n"
			"\tvendorID      : 0x%X\n"
			"\tdeviceID      : 0x%X\n"
			"\tdeviceType    : %u",
			current_device_props.deviceName,
			VK_VERSION_MAJOR(api_version),
			VK_VERSION_MINOR(api_version),
			VK_VERSION_PATCH(api_version),
			VK_VERSION_MAJOR(driver_version),
			VK_VERSION_MINOR(driver_version),
			VK_VERSION_PATCH(driver_version),
			current_device_props.vendorID,
			current_device_props.deviceID,
			current_device_props.deviceType);
		myy_vulkan_device_queues_enumerate(current_device);
	}
}

struct queue_scan_result {
	uint32_t queue_index;
	uint8_t found;
}

struct queue_scan_result myy_vulkan_device_has_a_matchin_queue(
	VkPhysicalDevice const valid_physical_device,
	VkQueueFlags const mandatory_queue_flags,
	uint32_t mandatory_queue_count)
{
	struct queue_scan_result scan_result = {
		.queue_index = 0,
		.found = 0
	};

	uint32_t n_queues = 0;
	
	vkGetPhysicalDeviceQueueFamilyProperties(
		myy_chosen_device, &n_queues, NULL);

	/* Let's blow the stack */
	VkQueueFamilyProperties device_vulkan_queues[n_queues];

	if (n_queues == 0)
	{
		log_entry("Your Vulkan device supports no queues !\n"
			"This must be the most useless Vulkan device available to you !");
		goto out;
	}

	vkGetPhysicalDeviceQueueFamilyProperties(
		myy_chosen_device, &n_queues, device_vulkan_queues);

	for (uint_fast32_t i = 0; i < n_queues; i++)
	{
		VkQueueFamilyProperties current_queue = device_vulkan_queues[i];
		if ((current_queue.queueFlags & mandatory_queue_flags)
		    && current_queue.queueCount <= mandatory_queue_count)
		{
			scan_result.queue_index = i;
			scan_result.found = 1;
			break;
		}
	}

out:
	return scan_result;
}

struct myy_best_device_search {
	VkDevice const found_device;
	uint32_t const good_queue_index;
	uint8_t success;
};

struct myy_best_device_search myy_vulkan_get_first_best_device_with(
	VkInstance const myy_vulkan_instance,
	VkPhysicalDeviceType const accepted_types,
	VkQueueFlags const mandatory_queue_flags,
	uint32_t mandatory_queue_count)
{
	uint32_t n_devices = 0;
	VkResult last_vk_func_ret;
	VkPhysicalDeviceProperties current_device_props;
	struct queue_scan_result;

	struct myy_best_device_search device_search_result = {
		.found_device = NULL,
		.good_queue_index = 0,
		.success = 0
	};

	last_vk_func_ret = vkEnumeratePhysicalDevices(
		myy_vulkan_instance,
		&n_devices, NULL);

	VkPhysicalDevice physical_devices[n_devices];

	if (last_vk_func_ret != VK_SUCCESS)
	{
		log_entry("vkEnumeratePhysicalDevices failed :C");
		goto out;
	}

	if (n_devices == 0)
	{
		log_entry("No vulkan devices available... We won't go very far...");
		goto out;
	}

	last_vk_func_ret = vkEnumeratePhysicalDevices(
		myy_vulkan_instance,
		&n_devices, physical_devices);

	if (last_vk_func_ret != VK_SUCCESS)
	{
		log_entry("vkEnumeratePhysicalDevices failed the second time... ??");
		goto out;
	}

	while(n_devices--)
	{
		VkPhysicalDevice current_device = *physical_devices++;
		vkGetPhysicalDeviceProperties(current_device,
			&current_device_props);
		if (current_device_props.deviceType & accepted_types)
		{
			queue_scan_result = myy_vulkan_device_has_a_matchin_queue(
				current_device,
				mandatory_queue_flags,
				mandatory_queue_count);
			if (queue_scan_result.found) {
				device_search_result.found_device = current_device;
				device_search_result.good_queue_index =
					queue_scan_result.index;
				device_search_result.success = 1;
				break;
			}
		}
	}

out:
	return device_search_result;
}

enum program_status {
	no_problems_so_far,
	status_could_not_create_a_vulkan_instance,
	status_could_not_get_a_good_vulkan_device
};

int main() {
	VkInstance myy_app_vulkan_instance;
	VkDevice myy_vulkan_device; /* The chosen logical device */

	enum program_status status = no_problems_so_far;
	int ret;
	ret = myy_vulkan_create_instance(&myy_app_vulkan_instance);

	if (!ret)
	{
		log_entry("Could not create a Vulkan instance");
		status = status_could_not_create_a_vulkan_instance;
		goto no_vulkan_instance;
	}

	{
		struct myy_best_device_search search_result =
			myy_vulkan_get_first_best_device_with(
				myy_app_vulkan_instance,
				VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
				| VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

		if (!search_result.success) {
			log_entry("Could not an appropriate vulkan device");
			status = status_could_not_get_a_good_vulkan_device;
			goto no_device;
		}
		
	}

out:
cleanup:
no_device:
	vkDestroyInstance(myy_app_vulkan_instance, NULL);
no_vulkan_instance:
func_end:
	return status;
}
