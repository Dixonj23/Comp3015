#version 460

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform int bloom;
uniform float exposure;

void main()
{
    vec3 hdrColor = texture(scene, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;

    if (bloom == 1)
        hdrColor += bloomColor;

    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);

    FragColor = vec4(result, 1.0);
}