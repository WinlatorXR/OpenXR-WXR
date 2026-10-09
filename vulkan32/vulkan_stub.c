// Stand-ins used only to generate an x86 vulkan-1.lib; the real vulkan-1.dll is loaded at run time.
// Argument counts give the stdcall @N byte sizes the runtime imports (64-bit handles take two).
void __stdcall vkAcquireNextImageKHR(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10) {}
void __stdcall vkAllocateCommandBuffers(int a1, int a2, int a3) {}
void __stdcall vkAllocateMemory(int a1, int a2, int a3, int a4) {}
void __stdcall vkBeginCommandBuffer(int a1, int a2) {}
void __stdcall vkBindBufferMemory(int a1, int a2, int a3, int a4, int a5, int a6, int a7) {}
void __stdcall vkBindImageMemory(int a1, int a2, int a3, int a4, int a5, int a6, int a7) {}
void __stdcall vkCmdBlitImage(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10) {}
void __stdcall vkCmdCopyBufferToImage(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) {}
void __stdcall vkCmdPipelineBarrier(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10) {}
void __stdcall vkCreateBuffer(int a1, int a2, int a3, int a4) {}
void __stdcall vkCreateCommandPool(int a1, int a2, int a3, int a4) {}
void __stdcall vkCreateFence(int a1, int a2, int a3, int a4) {}
void __stdcall vkCreateImage(int a1, int a2, int a3, int a4) {}
void __stdcall vkCreateSemaphore(int a1, int a2, int a3, int a4) {}
void __stdcall vkCreateSwapchainKHR(int a1, int a2, int a3, int a4) {}
void __stdcall vkCreateWin32SurfaceKHR(int a1, int a2, int a3, int a4) {}
void __stdcall vkDestroyBuffer(int a1, int a2, int a3, int a4) {}
void __stdcall vkDestroyCommandPool(int a1, int a2, int a3, int a4) {}
void __stdcall vkDestroyFence(int a1, int a2, int a3, int a4) {}
void __stdcall vkDestroyImage(int a1, int a2, int a3, int a4) {}
void __stdcall vkDestroySemaphore(int a1, int a2, int a3, int a4) {}
void __stdcall vkDestroySwapchainKHR(int a1, int a2, int a3, int a4) {}
void __stdcall vkEndCommandBuffer(int a1) {}
void __stdcall vkEnumeratePhysicalDevices(int a1, int a2, int a3) {}
void __stdcall vkFreeCommandBuffers(int a1, int a2, int a3, int a4, int a5) {}
void __stdcall vkFreeMemory(int a1, int a2, int a3, int a4) {}
void __stdcall vkGetBufferMemoryRequirements(int a1, int a2, int a3, int a4) {}
void __stdcall vkGetDeviceQueue(int a1, int a2, int a3, int a4) {}
void __stdcall vkGetImageMemoryRequirements(int a1, int a2, int a3, int a4) {}
void __stdcall vkGetPhysicalDeviceMemoryProperties(int a1, int a2) {}
void __stdcall vkGetPhysicalDeviceProperties(int a1, int a2) {}
void __stdcall vkGetSwapchainImagesKHR(int a1, int a2, int a3, int a4, int a5) {}
void __stdcall vkMapMemory(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9) {}
void __stdcall vkQueuePresentKHR(int a1, int a2) {}
void __stdcall vkQueueSubmit(int a1, int a2, int a3, int a4, int a5) {}
void __stdcall vkQueueWaitIdle(int a1) {}
void __stdcall vkResetCommandBuffer(int a1, int a2) {}
void __stdcall vkResetFences(int a1, int a2, int a3) {}
void __stdcall vkUnmapMemory(int a1, int a2, int a3) {}
void __stdcall vkWaitForFences(int a1, int a2, int a3, int a4, int a5, int a6) {}
