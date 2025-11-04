#include "ObjMesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "DirectionalLight.h"

#include "spdlog/spdlog.h"

void ObjMesh::init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_CommandPool& transfer_pool, VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT>& render_pass, const VKW_Path& obj_path, const VKW_Path& mtl_path)
{
	spdlog::info("Loading file {}", obj_path);
	// open file
	if (obj_path.extension() != ".obj") {
		throw IOException(
			fmt::format("Tried to open file {} which is not an obj using ObjMesh", obj_path),
			__FILE__, __LINE__
		);
	}

	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = mtl_path.string(); // Path to material files

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(obj_path.string(), reader_config)) {
		throw IOException(
			fmt::format("Tried to open file {}, Error: {} ", obj_path, reader.Error()),
			__FILE__, __LINE__
		);
	}

	if (!reader.Warning().empty()) {
		// might be too strict in some cases, but for example is ok for current warning encountered (material file missing)
		throw IOException(
			fmt::format("Tried to open file {}, Error: {}", obj_path, reader.Warning()),
			__FILE__, __LINE__
		);
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	// process mesh data
	m_meshes = std::vector<Mesh>(materials.size());

	std::vector<Vertex> vertices;
	std::vector<std::vector<uint32_t>> indices = std::vector<std::vector<uint32_t>>(materials.size());
	//std::unordered_map<Vertex, uint32_t> unique_vertices;

	// TODO: big buffers for position, normals etc shared between all shapes

	// Loop over shapes
	for (const tinyobj::shape_t& shape : shapes) {
		size_t index_offset = 0;
		// loop over faces
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shape.mesh.num_face_vertices[f]);

			if (size_t(shape.mesh.num_face_vertices[f]) != 3) {
				throw RuntimeException(
					fmt::format("Mesh {} failed to be triangulated. Contains faces with {} vertices", obj_path, fv),
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
				uint32_t i = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vert);

				// insert index
				indices[material_idx].push_back(i);
			}

			index_offset += fv;
		}
	}

	// create vertex buffer
	create_vertex_buffer(device, transfer_pool, vertices, m_vertex_buffer, m_vertex_buffer_address);

	// create meshes
	for (size_t i = 0; i < m_meshes.size(); i++) {
		m_meshes[i].init(device, transfer_pool, m_vertex_buffer_address, indices[i]);
	}

	// create material instances (with uniform buffers and textures)
	m_materials.reserve(materials.size());
	for (size_t mat_idx = 0; mat_idx < materials.size(); mat_idx++) {
		tinyobj::material_t mat = materials[mat_idx];

		uint32_t config = 0;
		if (mat.diffuse_texname != "") {
			config |= 1 << 0;
		}

		assert((mat.specular_texname == "") && "Cant support specular textures yet");
		assert((mat.metallic_texname == "") && "Cant support metallic textures yet");
		//assert(mat.ambient_texname == "" && "Cant support ambient textures yet");
		assert(mat.emissive_texname == "" && "Cant support emission textures yet");

		PBRUniform uniform{
			glm::pow(glm::vec3{ mat.diffuse[0], mat.diffuse[1], mat.diffuse[2] }, glm::vec3{2.2f}), // gamma correction
			mat.metallic,

			glm::vec3{ mat.specular[0], mat.specular[1], mat.specular[2] },
			mat.roughness,

			glm::vec3{ mat.emission[0], mat.emission[1], mat.emission[2]},
			1.000277f / mat.ior, // exterior IOR / interior IOR
			config
		};

		m_materials.push_back({});
		m_materials[mat_idx].init(
			device, 
			graphics_pool, 
			descriptor_pool, 
			render_pass, 
			{ ObjMesh::descriptor_set_layout },
			{ 2 },
			uniform,
			obj_path.parent_path(),
			mat.diffuse_texname,
			mat.name
		);
	}
}

void ObjMesh::del()
{
	for(Mesh & mesh: m_meshes) {
		mesh.del();
	}

	for (auto& mat : m_materials) {
		mat.del();
	}
	
	m_vertex_buffer.del();
}
