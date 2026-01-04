#ifndef POST_PROCESSING_COMMON_INCLUDE
#define POST_PROCESSING_COMMON_INCLUDE

layout( push_constant ) uniform constants
{
	VertexBuffer vertex_buffer;
	int tone_mapper_mode;
	float l_white_point; // luminance
} pc;

#endif