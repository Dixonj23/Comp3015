#version 460

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;
layout (location = 3) in vec3 VertexTangent;


#define NUM_LIGHTS 3

uniform struct LightInfo {
    vec4 Position;
    vec3 La;
    vec3 L;
    vec3 Direction;
    float Cutoff;
    float Exponent;
} Lights[NUM_LIGHTS];

out vec3 Position; 
out vec2 TexCoord;
out mat3 TBN;

uniform mat4 ModelViewMatrix;
uniform mat3 NormalMatrix;
uniform mat4 MVP;

void main()
{
	TexCoord = VertexTexCoord;

    Position = (ModelViewMatrix * vec4(VertexPosition,1.0)).xyz;

    vec3 N = normalize(NormalMatrix * VertexNormal);
    vec3 T = normalize(NormalMatrix * VertexTangent);
    vec3 B = normalize(cross(N, T));

    TBN = mat3(T, B, N);

    gl_Position = MVP * vec4(VertexPosition, 1.0);
	
}
