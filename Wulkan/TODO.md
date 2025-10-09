# Planned features

## General features
* Implement best practises as reported by validation layers [X]
* Shader includes with a shared file for common helper functions [X] 
* Remove Pipeline from draw[X]
* Shader printf suppport [X] 
* Separate sets for things bound at different freq (also allocate only if we need a unique one per object) [X]
* std::path instead of std::strings for path's [X] 
* Descriptor Pool improvements [X] 

* Update glfw due to https://github.com/glfw/glfw/issues/2684 or switch to SDL [ ]
* m_ convention[ ] 
* static_cast and not c style casts [ ] 
* First frame analysis [ ] 

* Maybe Make warnings only be compiled in debug mode [ ]
* Maybe combine buffer + layout for uniforms [ ] 
* Maybe cut down on parameters in favor of accessing via engine [ ] 
* Maybe Begin / End etc replacing with std::function to be called in that "context" [ ] 

* Anti Aliasing [ ] 
* Maybe experiment with Slang [ ] 
* Check best practises and synchronization regularly [ ]

# Models and Materials 
* Basic model loading [X]
* Refactor the idea of Pipeline + Shared Data to smt closer to materials [X]
* Shadows [X] 
* PBR Shader [~]
* Move textures (and maybe uniform buffers) into Material class instances [X] 

* Automatically detect if texture requires alpha channel [ ] 
* Completly transparent texture support (discard; also in depth) [~] 
* Emission / Ambient [ ] 
* Support more textures [ ] 
* Normal mapping [ ] 
* Texture mipmaps [ ] 
* Draw Indirect [ ] 
* Support more parameters [ ]
* Fix assumption that per model only one pipeline [ ]
* Mesh shaders [ ]

# Tracy 
* CPU side basics [X]
* GPU side basics [X]
* Synchronization [X]
* CPU Memory usage [ ]
* GPU Memory usage [ ] 

## Bugs
* Flickering at high frame rates [ ]
* Looking up / down changes shadows [ ]

## Terrain
* Check if normals are scaled correctly [ ] 
* Compute normals on the fly (or into texture) [ ] 
* Check how well multiple terrain's are handled [ ] 

## Shadows
### Hard shadows [~]
* Better layout transitions for shadow pass [X] 
* Maybe add screen space shadows to cover for peter panning [ ]
### Cascaded shadow maps [X]
* Lock orthographic shadow views to pixel movement [X]
### Soft shadows [X]

## Foliage
* Instanced rendering [ ]
* Bill boards [ ]
* Procedural placement[ ] 
* Wind [ ] 
* Culling/LOD [ ]

## Clouds