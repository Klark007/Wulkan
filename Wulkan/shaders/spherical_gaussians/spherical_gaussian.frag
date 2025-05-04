#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec3 pos;
layout (location = 2) in vec3 inQ;
layout (location = 3) in float inSigma;

layout (location = 0) out vec4 outFragColor;

#define PI 3.1415926538

void main() 
{
	vec3 q = inQ;
	float sigma = inSigma;

	float C = 1 / (sqrt(2) * pow(PI, 2.0/3.0) * sigma);
	float G = C * exp(-0.5 * pow(acos(dot(pos,q)) / sigma, 2)); 

	outFragColor = vec4(inColor * G,1.0f);
}
