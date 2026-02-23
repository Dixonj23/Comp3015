#version 460

in vec3 Position;
in mat3 TBN;
in vec2 TexCoord;

uniform sampler2D RenderTex;
layout (binding = 0) uniform sampler2D baseTexColor1;
layout (binding = 1) uniform sampler2D baseTexColor2;
layout (binding = 2) uniform sampler2D NormalMapTex;

layout (location = 0) out vec4 FragColor;


#define NUM_LIGHTS 3
uniform struct LightInfo {
    vec4 Position;
    vec3 La;
    vec3 L;
    vec3 Direction;
    float Cutoff;
    float Exponent;
} Lights[NUM_LIGHTS];

uniform struct MaterialInfo {
    vec3 Kd;
    vec3 Ka;
    vec3 Ks;
    float Shininess;
} Material;



vec3 blinnPhongLight(int i, vec3 n, vec3 texColor)
{
     vec3 ambient = Lights[i].La * texColor;

    vec3 s = normalize(Lights[i].Position.xyz - Position);   // frag -> light
    float sDotN = max(dot(s, n), 0.0);

    vec3 diffuse = texColor * sDotN;

    vec3 spec = vec3(0.0);
    if (sDotN > 0.0) {
        vec3 v = normalize(-Position);       // frag -> camera (eye)
        vec3 h = normalize(v + s);
        spec = Material.Ks * pow(max(dot(h, n), 0.0), Material.Shininess);
    }

    // spotlight factor
    float cosAng = dot(normalize(-s), normalize(Lights[i].Direction));
    float spot = 0.0;
    if (cosAng > Lights[i].Cutoff) spot = pow(cosAng, Lights[i].Exponent);

    return ambient + spot * (diffuse + spec) * Lights[i].L;
}


void main()
{
    // texture mixing
    vec4 c1 = texture(baseTexColor1, TexCoord);
    vec4 c2 = texture(baseTexColor2, TexCoord);
    vec3 texColor = mix(c1.rgb, c2.rgb, c2.a);

    // normal map (tangent -> eye using TBN)
    vec3 nTan = texture(NormalMapTex, TexCoord).xyz * 2.0 - 1.0;
    vec3 n = normalize(TBN * nTan);

    vec3 color = vec3(0.0);
    for (int i = 0; i < NUM_LIGHTS; i++)
        color += blinnPhongLight(i, n, texColor);

    FragColor = vec4(color, 1.0);
}
