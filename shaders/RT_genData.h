
struct RayGenData {
    vec3 color;
    float depth;
	vec3 hitPosition ;
};

float tMin = 0.001f;
float tMax = 10000.0f;