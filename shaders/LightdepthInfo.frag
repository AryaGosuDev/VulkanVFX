#version 450

layout (location = 0 ) in vec4 lightPosTransformed ;

layout (location = 0) out float fragDepth;

void main() {
	fragDepth = lightPosTransformed.z;
}
