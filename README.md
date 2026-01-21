# Wulkan
**Wulkan** is a personal Vulkan-based real-time renderer focused on learning modern graphics
techniques and reproducing the visual quality of a path-traced reference image created during
an ETH Computer Graphics course final project.

<div align="center">

![Showcase](https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/main_image.png)

</div>

## Feature Overview
| Feature | Description |
|-------|------------|
| Terrain Tessellation | Curvature- and distance-based dynamic tessellation with frustum culling |
| Shadows | Cascaded Shadow Maps with Contact-Hardening Soft Shadows |
| OBJ Loading | PBR-capable OBJ loading with PBR material support |
| CPU Instancing | Supports CPU instancing with multiple different level of detail models |
| Tone mapping | Supports multiple tone mappers such as Rheihard, ACES & AgX |


## Installation
**Supported platforms:** Windows

### Prebuild binaries
Add binary into the Wulkan folder and execute from there

### Windows Compilation
**Requirements:**
- Vulkan SDK 1.3+ with TODO
- Visual Studio 2022

Steps:
1. Open `Wulkan.sln` in Visual Studio
2. Set the Vulkan SDK paths:
   - Add `VulkanSDK\<Version>\Lib` to Linker -> General -> Additional Library Directories
   - Add `VulkanSDK\<Version>\Include` to VC++ Directories -> External Include Directories
3. Build in `Release` or `Debug`

## Technical details

### Terrain with Dynamic Tesselation based on curvature and distance
Uses mean curavture (precomputed) and distance to camera to decide how much to tesselate. If a patch is completly outside of the camera frustum, it gets culled.

<div align="center">

<em>
Left we show a height map and on the right how it looks in the engine with mean curavture being visualized (from least to most: green, blue, red)
</em>
<br>
 
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

### Obj Support
Enables loading Obj files using [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader). Supports PBR Materials supporting diffuse, metallic, roughness parameters and diffuse textures.

<div align="center">

<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/obj_loading/basic_obj_loading.png" alt="Basic OBJ" width="100%">

*Shows a simple OBJ file being loaded into the engine*


<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/obj_loading/textured_material.png" alt="Basic OBJ" width="100%">

*Test scene with a more complex, textured material*

</div>

TODO Mipmapped Textures


Mipmapped Textures
CPU Instancing and LOD
### Tonemapping

Implements different tonemappers as a post processing step (full screen quad) before ImGUI draws it's UI ontop.

<div align="center">


<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/tonemapper/None1.png" alt="None 1" width="45%">
<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/tonemapper/None2.png" alt="None 2" width="45%">

*No tonemapper*

<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/tonemapper/Rheinhard1.png" alt="Rh 1" width="45%">
<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/tonemapper/Rheinhard2.png" alt="Rh 2" width="45%">

*Rheinhard on Luminance*

<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/tonemapper/ExtRheinhard1.png" alt="Ext Rh 1" width="45%">
<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/tonemapper/ExtRheinhard2.png" alt="Ext Rh 2" width="45%">

*Extended Rheinhard on Luminance*

<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/tonemapper/Uncharted1.png" alt="Uch 1" width="45%">
<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/tonemapper/Uncharted2.png" alt="Uch 2" width="45%">

*Uncharted / Hable Filmic with default parameters*

<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/tonemapper/ACES1.png" alt="ACES 1" width="45%">
<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/tonemapper/ACES2.png" alt="ACES 2" width="45%">

*ACES tone mapper*

<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/tonemapper/AgxPunchy1.png" alt="AGX 1" width="45%">
<img src="https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/tonemapper/AgxPunchy2.png" alt="AGX 2" width="45%">

*AgX with Punchy preset*

</div>

The white point for extended rheinhard is set using the GUI. Uncharted 2's tonemapper is shown with the default values from the following [blog](https://graphics-programming.org/blog/tone-mapping) and the plan is to replace it with a tonemapper with more understandable parameters ([New Hable filmic](http://filmicworlds.com/blog/filmic-tonemapping-with-piecewise-power-curves/)).

## Pathtraced Inspiration
This image is meant as a visual target to inspire which feature to add to the project and the goal is not prefect parity. Trade off's will need to be made due to real time constraints.
<div align="center">

![Render](https://github.com/Klark007/Wulkan/blob/master/Wulkan/screenshots/PathtracedInspiration.png)
*Pathtraced image for my final project in ETH Computer graphics course (two person project).<br> Lower half of image is used as inspiration/goal for this real time project*

</div>

<!-- CONTROLS -->

## Dependencies
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

## Models
* [Air baloon](https://www.turbosquid.com/3d-models/hot-air-baloon-3d-2092268)
* [Tree](https://sketchfab.com/3d-models/maple-trees-pack-lowpoly-game-ready-lods-b5d2833c258f4054a01ee2b4ef85adf0#download)
* [Sponza](https://github.com/jimmiebergmann/Sponza/tree/master)
* [Mitsuba](https://casual-effects.com/data/)

## Roadmap
For a roadmap see [Wulkan/TODO.md](https://github.com/Klark007/Wulkan/blob/master/Wulkan/TODO.md)

