#include "ObjMesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "DirectionalLight.h"

#include <format>
#include <iostream>

void ObjMesh::init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, OBJ_MESH_DESC_SET_COUNT>& render_pass,const std::string& obj_path, const std::string& mtl_path)
{
	// open file
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
	create_vertex_buffer(device, transfer_pool, vertices, m_vertex_buffer, m_vertex_buffer_address);

	// create meshes
	for (size_t i = 0; i < m_meshes.size(); i++) {
		m_meshes[i].init(device, transfer_pool, m_vertex_buffer_address, indices[i]);
	}
	// create material instances 
	m_materials.reserve(materials.size());
	for (size_t i = 0; i < materials.size(); i++) {
		m_materials.push_back(render_pass.create_material_instance(device, descriptor_pool));
	}

	// create and set uniform buffers
	for (size_t frame_idx = 0; frame_idx < MAX_FRAMES_IN_FLIGHT; frame_idx++) {
		m_uniform_buffers[frame_idx].resize(materials.size());
		for (size_t mat_idx = 0; mat_idx < materials.size(); mat_idx++) {
			VKW_Buffer& uniform_buffer = m_uniform_buffers[frame_idx][mat_idx];

			// TODO: different memory as we don't plan to change those on the fly
			uniform_buffer.init(
				&device,
				sizeof(PBRUniform),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				sharing_exlusive(),
				true,
				std::format("PBR Uniform {} ({})", materials[mat_idx].name, frame_idx)
			);
			uniform_buffer.map();

			// gamma correction for colors
			tinyobj::material_t mat = materials[mat_idx];
			std::cout << "NAME: " << mat.diffuse_texname << std::endl;
			
			assert(mat.diffuse_texname == "", "Cant support diffuse textures yet");
			assert(mat.specular_texname == "", "Cant support specular textures yet");
			assert(mat.metallic_texname == "", "Cant support metallic textures yet");
			assert(mat.ambient_texname == "", "Cant support ambient textures yet");
			assert(mat.emissive_texname == "", "Cant support emission textures yet");

			PBRUniform uniform{
				glm::pow(glm::vec3{ mat.diffuse[0], mat.diffuse[1], mat.diffuse[2] }, glm::vec3{2.2f}), // gamma correction
				mat.metallic,

				glm::vec3{ mat.specular[0], mat.specular[1], mat.specular[2] },
				mat.roughness,

				glm::vec3{ mat.emission[0], mat.emission[1], mat.emission[2]},
				1.000277f / mat.ior, // exterior IOR / interior IOR
				0
			};

			memcpy(uniform_buffer.get_mapped_address(), &uniform, sizeof(PBRUniform));
		}
	}
}

void ObjMesh::set_descriptor_bindings(const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& general_ubo, const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& shadow_map_ubo, Texture& shadow_map, const VKW_Sampler& shadow_map_sampler, const VKW_Sampler& shadow_map_gather_sampler)
{
	for (size_t mat_idx = 0; mat_idx < m_materials.size(); mat_idx++) {
		auto& mat = m_materials[mat_idx];
		for (unsigned int frame_idx = 0; frame_idx < MAX_FRAMES_IN_FLIGHT; frame_idx++) {
			const VKW_DescriptorSet& set0 = mat.get_descriptor_set(frame_idx, 0);
			const VKW_DescriptorSet& set1 = mat.get_descriptor_set(frame_idx, 1);
			const VKW_DescriptorSet& set2 = mat.get_descriptor_set(frame_idx, 2);

			set0.update(0, general_ubo.at(frame_idx));

			set1.update(0, shadow_map_ubo.at(frame_idx));
			set1.update(1, shadow_map.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 0, MAX_CASCADE_COUNT));
			set1.update(2, shadow_map_sampler);
			set1.update(3, shadow_map_gather_sampler);

			set2.update(0, m_uniform_buffers[frame_idx][mat_idx]);
		}
	}
}

RenderPass<PushConstants, OBJ_MESH_DESC_SET_COUNT> ObjMesh::create_render_pass(const VKW_Device* device, const std::array<VKW_DescriptorSetLayout, OBJ_MESH_DESC_SET_COUNT>& layouts, Texture& color_rt, Texture& depth_rt, bool depth_only, bool bias_depth)
{
	RenderPass<PushConstants, OBJ_MESH_DESC_SET_COUNT > render_pass{};

	// Push constants
	VKW_PushConstant<PushConstants> push_constant{};
	push_constant.init(VK_SHADER_STAGE_VERTEX_BIT);

	// Graphics pipeline
	VKW_GraphicsPipeline graphics_pipeline{};

	VKW_Shader vert_shader{};
	if (!depth_only) {
		vert_shader.init(device, "shaders/pbr/pbr_vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "PBR vertex shader");
	}
	else {
		vert_shader.init(device, "shaders/pbr/pbr_depth_vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "PBR vertex shader");
	}

	VKW_Shader frag_shader{};
	frag_shader.init(device, "shaders/pbr/pbr_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "PBR fragment shader");

	graphics_pipeline.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	if (!depth_only) {
		graphics_pipeline.set_culling_mode();
	}
	graphics_pipeline.enable_depth_test();
	graphics_pipeline.enable_depth_write();

	if (depth_only) {
		graphics_pipeline.add_shader_stages({ vert_shader });
	}
	else {
		graphics_pipeline.add_shader_stages({ vert_shader, frag_shader });
	}

	graphics_pipeline.add_descriptor_sets(layouts);
	graphics_pipeline.add_push_constants({ push_constant.get_range() });

	if (!depth_only) {
		graphics_pipeline.set_color_attachment_format(color_rt.get_format());
	}
	graphics_pipeline.set_depth_attachment_format(depth_rt.get_format());

	if (depth_only && bias_depth) {
		graphics_pipeline.enable_dynamic_depth_bias();
	}

	graphics_pipeline.init(device, "PBR graphics pipeline");

	vert_shader.del();
	frag_shader.del();

	// end graphics pipeline

	render_pass.init(
		graphics_pipeline,
		layouts,
		push_constant,
		"Obj Pbr Material"
	);

	return render_pass;
}

VKW_DescriptorSetLayout ObjMesh::create_descriptor_set_layout(const VKW_Device& device)
{
	VKW_DescriptorSetLayout layout{};

	layout.add_binding(
		0,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_FRAGMENT_BIT
	);

	layout.init(&device, "PBR Descriptor Set Layout");

	return layout;
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

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		for (VKW_Buffer& uniform_buffer : m_uniform_buffers[i]) {
			uniform_buffer.del();
		}
	}
}
