#version 460

in vec3 Position;
in vec2 TexCoord;
in mat3 TBN;

layout (binding = 0) uniform sampler2D baseTexColor1;
layout (binding = 1) uniform sampler2D NormalMapTex;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform int UseTexture;
uniform vec3 SolidColor;
uniform float EmissiveStrength;
uniform float Time;
uniform int IsBeam;

uniform struct LightInfo {
    vec4 Position;
    vec3 La;
    vec3 Ld;
    vec3 Ls;
} Light;

uniform struct MaterialInfo {
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    float Shininess;
} Material;

float hash(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float beamPattern(vec2 uv, float t)
{
    float wave1 = sin((uv.y * 18.0) - t * 6.0) * 0.5 + 0.5;
    float wave2 = sin((uv.y * 31.0) + t * 9.0) * 0.5 + 0.5;
    float wave3 = sin((uv.x * 10.0) + (uv.y * 24.0) - t * 4.0) * 0.5 + 0.5;

    float n = hash(vec2(floor(uv.y * 20.0), floor(t * 8.0)));
    float flicker = mix(0.85, 1.15, n);

    return (0.45 * wave1 + 0.35 * wave2 + 0.20 * wave3) * flicker;
}

void main()
{
    vec3 albedo = (UseTexture == 1)
    ? texture(baseTexColor1, TexCoord).rgb
    : SolidColor;

    if (IsBeam == 1)
    {
        float energy = beamPattern(TexCoord, Time);

        // brighter core with animated variation
        albedo *= mix(0.7, 1.8, energy);

        // slight color shift 
        albedo += vec3(0.05, 0.15, 0.25) * energy;
    }

    vec3 N;
    if (UseTexture == 1) {
        vec3 nTan = texture(NormalMapTex, TexCoord).xyz * 2.0 - 1.0;
        N = normalize(TBN * nTan);
    } else {
        N = normalize(TBN[2]);
    }

    vec3 s = normalize(Light.Position.xyz - Position);
    vec3 v = normalize(-Position);
    vec3 h = normalize(v + s);

    vec3 ambient = Light.La * albedo * Material.Ka;

    float sDotN = max(dot(s, N), 0.0);
    vec3 diffuse = Light.Ld * albedo * Material.Kd * sDotN;

    vec3 spec = vec3(0.0);
    if (sDotN > 0.0) {
        spec = Light.Ls * Material.Ks * pow(max(dot(h, N), 0.0), Material.Shininess);
    }

    vec3 litColor = ambient + diffuse + spec;

    // emissive glow
    litColor += albedo * EmissiveStrength;

    FragColor = vec4(litColor, 1.0);

    // brightness extraction
    float brightness = dot(litColor, vec3(0.2126, 0.7152, 0.0722));

    if (brightness > 1.0)
        BrightColor = vec4(litColor, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}