#include "ObjMesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <format>
#include <iostream>

void ObjMesh::init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const std::string& obj_path, const std::string& mtl_path)
{
	size_t file_ext_idx = obj_path.rfind(".") + 1;
	if (obj_path.substr(file_ext_idx, obj_path.size()) != "obj") {
		throw IOException(
			std::format("Tried to open file {} which is not an obj using ObjMesh", obj_path),
			__FILE__, __LINE__
		);
	}

	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = mtl_path; // Path to material files

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(obj_path, reader_config)) {
		throw IOException(
			std::format("Tried to open file {}, Error: {} ", obj_path, reader.Error()),
			__FILE__, __LINE__
		);
	}

	if (!reader.Warning().empty()) {
		std::cout << "Warning ObjMesh " << reader.Warning() << std::endl;
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	meshes = std::vector<Mesh>(materials.size());

	std::vector<Vertex> vertices;
	std::vector<std::vector<uint32_t>> indices = std::vector<std::vector<uint32_t>>(materials.size());
	//std::unordered_map<Vertex, uint32_t> unique_vertices;

	// TODO: big buffers for position, normals etc shared between all shapes
	// vertices.reserve(attrib.vertices.size() / 3);

	// Loop over shapes
	for (const tinyobj::shape_t& shape : shapes) {
		size_t index_offset = 0;
		// loop over faces
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shape.mesh.num_face_vertices[f]);

			if (size_t(shape.mesh.num_face_vertices[f]) != 3) {
				throw RuntimeException(
					std::format("Mesh {} failed to be triangulated. Contains faces with {} vertices", obj_path, fv),
					__FILE__, __LINE__
				);
			}

			size_t material_idx = shape.mesh.material_ids[f];

			for (size_t v = 0; v < fv; v++) {
				tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

				// asssemble vertex
				glm::vec3 p = {
						attrib.vertices[3 * size_t(idx.vertex_index) + 0],
						attrib.vertices[3 * size_t(idx.vertex_index) + 1],
						attrib.vertices[3 * size_t(idx.vertex_index) + 2]
				};

				glm::vec3 n = (idx.normal_index >= 0) ? glm::vec3{
					attrib.normals[3 * size_t(idx.normal_index) + 0],
					attrib.normals[3 * size_t(idx.normal_index) + 1],
					attrib.normals[3 * size_t(idx.normal_index) + 2],
				} : glm::vec3{0, 0, 0};

				float uv_x = (idx.texcoord_index >= 0) ? attrib.texcoords[2 * size_t(idx.texcoord_index) + 0] : 0;
				float uv_y = (idx.texcoord_index >= 0) ? attrib.texcoords[2 * size_t(idx.texcoord_index) + 1] : 0;

				Vertex vert{
					p,
					uv_x,
					n,
					uv_y,
					{1,0,1,1} // color
				};

				// add to vector for buffer
				uint32_t i = vertices.size();
				vertices.push_back(vert);

				// insert index
				indices[material_idx].push_back(i);
			}

			index_offset += fv;
		}
	}

	// create vertex buffer
	create_vertex_buffer(device, transfer_pool, vertices, vertex_buffer, vertex_buffer_address);

	// create meshes
	for (size_t i = 0; i < meshes.size(); i++) {
		meshes[i].init(device, transfer_pool, vertex_buffer_address, indices[i]);
	}

}

void ObjMesh::del()
{
	for(Mesh & m: meshes) {
		m.del();
	}
	

	vertex_buffer.del();
}

