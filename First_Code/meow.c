#include "vulkan/vulkan.h"
#include <stdio.h>

/* I tend to call this function log.
 * But that generate issues when calling math functions... */
#define log_entry(fmt, ...) \
	fprintf(stderr, "[%s:%s():%d] " fmt "\n", \
		__FILE__, __func__, __LINE__, ##__VA_ARGS__ )

/* I'm writing this code while reading :
 * https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-1
 *
 * So yeah, the code will be extremely similar, if not identical.
 */
int myy_vulkan_create_instance(
	VkInstance * __restrict instance_address)
{
	VkApplicationInfo application_info = {
		/* ??? I can understand provide structures types when the
		 * receiving function doesn't know what coming in but here,
		 * that doesn't make a lot of sense ...
		 */
  		VK_STRUCTURE_TYPE_APPLICATION_INFO,             // VkStructureType            sType
		/* TODO You can set multiple applications at once ? */
		NULL,                                        // const void                *pNext
		/* The name of our wonderful application */
		"Myy Vulkan : First level !",                   // const char                *pApplicationName
		/* Its version */
		VK_MAKE_VERSION( 0, 1, 0 ),                     // uint32_t                   applicationVersion
		/* The name of our awesome engine ! */
		"Myy Vulkan Inexistent engine !",               // const char                *pEngineName
		/* Its version */
		VK_MAKE_VERSION( 0, 0, 1 ),                     // uint32_t                   engineVersion
		/* VK_API_VERSION exists no more */
		VK_API_VERSION_1_0                                  // uint32_t                   apiVersion
	};

	/* The documentation states that the structure type defines the
	 * structure size... However, does the documentation state the
	 * pointer size ?
	 * 32 bits pointer being 4 bytes long and 64 bytes being 8 bytes
	 * long, this is quite different...
	 */
	VkInstanceCreateInfo instance_create_info = {
		/* Ok, I guess that all structures are typed now. */
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // VkStructureType            sType
		/* And are all linked lists ... */
		NULL,                                   // const void*                pNext
		/* Reserved for future use. 0 currently */
		0,                                      // VkInstanceCreateFlags      flags
		/* Our application information, set up just before */
		&application_info,                      // const VkApplicationInfo   *pApplicationInfo
		/* Enable 0 layer... Because we don't care about them here. */
		0,                                      // uint32_t                   enabledLayerCount
		/* No layers enabled, so no layers provided */
		NULL,                                   // const char * const        *ppEnabledLayerNames
		/* Enable 0 extension... Because we don't care about them here. */
		0,                                      // uint32_t                   enabledExtensionCount
		/* No extensions enableld, so no extensions provided */
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
		VkPhysicalDevice current_device =
			*devices++;

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
	}
}


int myy_vulkan_devices_enumerate_from(
	VkInstance const myy_vulkan_instance,
	VkPhysicalDevice * __restrict myy_chosen_physical_device)
{

	/* Calling vkEnumeratePhysicalDevices with :
	 *   VulkanInstance, 0 and NULL
	 * will force vkEnumeratePhysicalDevices to provide us the actual
	 * number of devices through &n_devices.
	 * 
	 * Then we call prepare enough memory to store n_devices handles
	 * and call the same function again with :
	 *   VulkanInstance, n_devices, physical_devices
	 * 
	 * physical_devices being some memory space pre-allocated to
	 * store, at least, N VkPhysicalDevice handles.
	 */

	uint32_t n_devices = 0;
	VkResult last_vk_func_ret;

	last_vk_func_ret = vkEnumeratePhysicalDevices(
		myy_vulkan_instance,
		&n_devices, NULL);
	int success = 0;
	VkPhysicalDevice physical_devices[n_devices];

	if (last_vk_func_ret != VK_SUCCESS) {
		log_entry("vkEnumeratePhysicalDevices failed :C");
		goto func_end;
	}

	if (n_devices == 0) {
		log_entry("No vulkan devices available... We won't go very far...");
		goto func_end;
	}

	last_vk_func_ret = vkEnumeratePhysicalDevices(
		myy_vulkan_instance,
		&n_devices, physical_devices);

	if (last_vk_func_ret != VK_SUCCESS) {
		log_entry("vkEnumeratePhysicalDevices failed the second time... ??");
		goto func_end;
	}
	
	myy_vulkan_devices_print_basic_properties(n_devices, physical_devices);

	/* We take the first device, because :
	* if it's first, it's better, right ?
	* 
	* Just kidding, I'm just lazy.
	*/

	*myy_chosen_physical_device = physical_devices[0];

	success = 1;


func_end:
	return success;
}

static inline char const * c_boolean_to_string(
	uint_fast32_t const value)
{
	return (value != 0 ? "true" : "false");
}
void myy_vulkan_device_queues_print_basic_properties(
	uint_fast32_t n_queues,
	VkQueueFamilyProperties const * __restrict queues)
{
	while(n_queues--) {
		VkQueueFlags flags = queues->queueFlags;
		log_entry("Queue Flags :\n"
			"\tGraphics        : %s\n"
			"\tCompute         : %s\n"
			"\tTransfer        : %s\n"
			"\tSparse bindings : %s\n"
			"queueCount         : %d\n"
			"timestampValidBits : %d\n",
			c_boolean_to_string(flags & VK_QUEUE_GRAPHICS_BIT),
			c_boolean_to_string(flags & VK_QUEUE_COMPUTE_BIT),
			c_boolean_to_string(flags & VK_QUEUE_TRANSFER_BIT),
			c_boolean_to_string(flags & VK_QUEUE_SPARSE_BINDING_BIT),
			queues->queueCount,
			queues->timestampValidBits);
		queues++;
	}
}

int myy_vulkan_device_queues_enumerate(
	VkInstance const myy_app_vulkan_instance,
	VkPhysicalDevice const myy_chosen_device)
{
	uint32_t n_queues = 0;
	int success = 0;
	
	vkGetPhysicalDeviceQueueFamilyProperties(
		myy_chosen_device, &n_queues, NULL);

	/* Let's blow the stack */
	VkQueueFamilyProperties device_vulkan_queues[n_queues];

	if (n_queues == 0)
	{
		log_entry("Your Vulkan device supports no queues !\n"
			"This must be the most useless Vulkan device available to you !");
		goto func_end;
	}

	vkGetPhysicalDeviceQueueFamilyProperties(
		myy_chosen_device, &n_queues, device_vulkan_queues);

	myy_vulkan_device_queues_print_basic_properties(
		n_queues, device_vulkan_queues);

	success = 1;
func_end:
	return success;
}

enum program_status {
	no_problems_so_far,
	status_could_not_create_a_vulkan_instance,
	status_could_not_enumerate_physical_devices,
	status_could_not_enumerate_queues
};

int main() {
	VkInstance myy_app_vulkan_instance;
	VkPhysicalDevice myy_chosen_physical_device;

	enum program_status status = no_problems_so_far;
	int ret;
	ret = myy_vulkan_create_instance(&myy_app_vulkan_instance);

	if (!ret)
	{
		log_entry("Could not create a Vulkan instance");
		status = status_could_not_create_a_vulkan_instance;
		goto no_vulkan_instance;
	}

	ret = myy_vulkan_devices_enumerate_from(
		myy_app_vulkan_instance, &myy_chosen_physical_device);

	if (!ret)
	{
		log_entry("Could not enumerate the available Vulkan devices");
		status = status_could_not_enumerate_physical_devices;
		goto no_devices;
	}

	myy_vulkan_device_queues_enumerate(
		myy_app_vulkan_instance, myy_chosen_physical_device);

cleanup:
no_devices:
	vkDestroyInstance(myy_app_vulkan_instance, NULL);
no_vulkan_instance:
func_end:
	return status;
}
