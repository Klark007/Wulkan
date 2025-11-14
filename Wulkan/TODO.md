# Planned features

## General features
* 1Compute shader for sampling arbitrary textures from cpu side (https://docs.vulkan.org/guide/latest/storage_image_and_texel_buffers.html#_storage_images) [ ]
* 2MSAA [ ]
* Better init with faster start up time [ ]

## Maybe and Convetions
* void VKW_Buffer::copy(const void* data, size_t data_size, size_t offset) change order of size/offset [ ] 
* Update glfw due to https://github.com/glfw/glfw/issues/2684 or switch to SDL [ ]
* m_ convention[ ] 
* static_cast and not c style casts [ ] 
* First frame analysis [ ] 

* Maybe Make warnings only be compiled in debug mode [ ]
* Maybe combine buffer + layout for uniforms [ ] 
* Maybe cut down on parameters in favor of accessing via engine [ ] 
* Maybe Begin / End etc replacing with std::function to be called in that "context" [ ] 
* Maybe experiment with Slang [ ] 

* Check best practises and synchronization regularly [ ]

## Models and Materials 
* Store Texture mipmaps [ ] 
* Draw Indirect [ ] 
* Fix assumption that per model only one pipeline [ ]
* Mesh shaders [ ]

## PBR
* 3Normal mapping [ ] 
* Automatically detect if texture requires alpha channel [ ] 
* Emission / Ambient [ ] 
* Support more textures [ ] 
* Support more parameters [ ]

## Bugs
* Blender UV imporing weird [ ] 
* Flickering at high frame rates [ ]
* Looking up / down changes shadows [ ]

## Terrain
* Check if normals are scaled correctly [ ] 
* Compute normals on the fly (or into texture) [ ] 
* Check how well multiple terrain's are handled [ ] 

## Shadows
* Maybe add screen space shadows to cover for peter panning [ ]

## Foliage
### CPU Side
* Orienting Bill boards [ ]

### GPU Side
* Procedural placement[ ] 
* Wind [ ] 
* Culling/LOD [ ]

## Clouds


# Finished features
## General features
* Implement best practises as reported by validation layers [X]
* Shader includes with a shared file for common helper functions [X] 
* Remove Pipeline from draw[X]
* Shader printf suppport [X] 
* Separate sets for things bound at different freq (also allocate only if we need a unique one per object) [X]
* std::path instead of std::strings for path's [X] 
* Descriptor Pool improvements [X] 
* loadEXR memory leak due to no free? [X] 
* Better mapping interface with VMA (https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html) [X] 

## Models and Materials 
* Basic model loading [X]
* Refactor the idea of Pipeline + Shared Data to smt closer to materials [X]
* Shadows [X] 
* Move textures (and maybe uniform buffers) into Material class instances [X] 
* Completly transparent texture support (discard; also in depth) [X] 
* Texture mipmaps [X]
* Deduplicate texture code [X]

## Tracy 
* CPU side basics [X]
* GPU side basics [X]
* Synchronization [X]
* CPU Memory usage [X]
* GPU Memory usage [~] 

## Shadows
### Hard shadows [~]
* Better layout transitions for shadow pass [X] 
### Cascaded shadow maps [X]
* Lock orthographic shadow views to pixel movement [X]
### Soft shadows [X]

## Foliage
### CPU Side
* CPU Instances [X]
* CPU LOD [X]
* Backface culling disable for certain models [X] 
* Synchronization of mapped memory (see https://docs.vulkan.org/guide/latest/synchronization_examples.html#_transfer_dependencies) [X]
* CPU Instanced LOD [X] 
* Change ranges GUI [X] 
* Change near, far plane [X] 
* Visualize LOD variant [X]