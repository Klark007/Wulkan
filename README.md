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
### Terrain with Dynamic Tesselation based on curvature and distance
Uses mean curavture (precomputed) and distance to camera to decide how much to tesselate. If a patch is completly outside of the camera frustum, it gets culled.

<div align="center">

<em>
Left we show a height map and on the right how it looks in the engine with mean curavture being visualized (from least to most: green, blue, red)
</em>
 
<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/textures/terrain/test/height_test1.png" alt="Heightmap 1" width="36%">
<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/terrain/terrain_test_1.png" alt="Tesselation 1" width="45%">

*The non symmetry is due to how we use finite difference to estimate the curvature*

<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/textures/terrain/test/height_test2.png" alt="Heightmap 2" width="36%">
<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/terrain/terrain_test_2.png" alt="Tesselation 2" width="45%">

<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/terrain/terrain_tess.png" alt="Terrain Tesselation" width="90%">

*Visualize terrain with tesselation strength*

<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/terrain/terrain.png" alt="Terrain" width="45%">
<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/terrain/terrain_wireframe.png" alt="Wireframe" width="45%">

*Shows tesselated terrain with diffuse lighting and its corresponding wireframe*
</div>

### Shadows
Implements [cascaded shadow maps](https://developer.download.nvidia.com/SDK/10.5/opengl/src/cascaded_shadow_maps/doc/cascaded_shadow_maps.pdf) with [contact hardening soft shadows](https://wojtsterna.com/wp-content/uploads/2023/02/contact_hardening_soft_shadows.pdf)

<div align="center">

<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/shadows/NoShadows.png" alt="No Shadow" width="100%">
<em>No shadows</em>
<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/shadows/HardShadows.png" alt="Hard Shadow" width="100%">
<em>Hard shadows</em>
<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/shadows/SoftShadows.png" alt="Soft Shadow" width="100%">
<em>Soft shadows</em>
</div>

Shows the difference between no shadows, hard shadows and soft shadows. Most notable on the shadow of the baloon in the left half of the images

<div align="center">

<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/shadows/cascade_visualization.png" alt="Cascades" width="100%">

*Visualize the different cascades given the shown camera frustum*
</div>

PBR Materials supporting diffuse, metallic, roughness parameters and diffuse textures
Mipmapped Textures
CPU Instancing and LOD
Multiple Tonemappers

## Pathtraced Inspiration
<div align="center">

![Render](https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/PathtracedInspiration.png)
*Pathtraced image for my final project in ETH Computer graphics course (two person project).<br> Lower half of image is used as inspiration/goal for this real time project*

</div>

