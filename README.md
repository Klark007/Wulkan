# Wulkan
A personal project exploring and learning Vulkan while trying to recreate the visuals of a pathtraced image created during a university course as a final project

## Installation
### Prebuild binaries
Add binary into the Wulkan folder and execute from there

### Windows Compilation
Needs VulkanSDK with TODO installed. Use Visual Studio to open Solution, add VulkanSDK\$Version\Lib Additional Library Directories (Linker->General) and add VulkanSDK\$Version\Include to External Include Directories (VC+ Directories).

### Dependencies
* VulkanSDK 1.3 or newer
* GLFW
* Volk
* vk-bootstrap
* vma
* imgui
* glm
* rapidcsv
* stb_image
* tinyexr
* miniz (used by tinyexr)
* tph_poisson
* tracy
* tinyobjloader


## Features
Terrain with Dynamic Tesselation based on curvature and distance
Cascaded Shadow Maps and Contact-hardening Soft Shadows
PBR Materials supporting diffuse, metallic, roughness parameters and diffuse textures
Mipmapped Textures
CPU Instancing and LOD
Multiple Tonemappers