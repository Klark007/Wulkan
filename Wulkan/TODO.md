# Planned features

## General features
* Implement best practises as reported by validation layers [X]
* Shader includes with a shared file for common helper functions [X] 
* Separate sets for things bound at different freq [ ]
* Shader printf suppport [ ] 
* Make warnings only be compiled in debug mode[ ]
* Anti Aliasing [ ] 
* Maybe experiment with Slang [ ] 
* Check best practises and synchronization regularly [ ]

# Models and Materials 
* Basic model loading [ ]
* Refactor the idea of Pipeline + Shared Data to smt closer to materials [ ]
* Draw Indirect [ ] 
* Fix assumption that per model only one pipeline [ ]
* Mesh shaders [ ] 
* PBRT Shader [ ] 

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