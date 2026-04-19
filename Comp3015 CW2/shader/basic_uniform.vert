#version 460

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;
layout (location = 3) in vec3 VertexTangent;

out vec3 Position;   // view-space position
out vec3 WorldPos;   // world-space position
out vec2 TexCoord;
out mat3 TBN;

uniform mat4 ModelMatrix;
uniform mat4 ModelViewMatrix;
uniform mat3 NormalMatrix;
uniform mat4 MVP;

void main()
{
    TexCoord = VertexTexCoord;

    // View-space position (used for lighting)
    Position = (ModelViewMatrix * vec4(VertexPosition, 1.0)).xyz;

    // World-space position (used for point-shadow lookup)
    WorldPos = vec3(ModelMatrix * vec4(VertexPosition, 1.0));

    vec3 N = normalize(NormalMatrix * VertexNormal);
    vec3 T = normalize(NormalMatrix * VertexTangent);
    vec3 B = normalize(cross(N, T));
    TBN = mat3(T, B, N);

    gl_Position = MVP * vec4(VertexPosition, 1.0);
}