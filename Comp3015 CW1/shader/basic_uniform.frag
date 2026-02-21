#version 460

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D RenderTex;
layout (binding = 0) uniform sampler2D baseTexColor1;
layout (binding = 1) uniform sampler2D baseTexColor2;

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



vec3 blinnPhongLight(int i, vec3 position, vec3 n)
{
    vec3 diffuse = vec3(0), spec = vec3(0);

    vec4 baseTexColor1 = texture(baseTexColor1, TexCoord);
    vec4 baseTexColor2 = texture(baseTexColor2, TexCoord);
    vec3 texColor = mix(baseTexColor1.rgb, baseTexColor2.rgb, baseTexColor2.a);

    vec3 ambient = Lights[i].La * texColor;

    vec3 s = normalize(Lights[i].Position.xyz - position);
    float sDotN = max(dot(s,n), 0.0);

    vec3 spotDir = normalize(Lights[i].Direction);
    float cosAng = dot(-s, spotDir);

    float spotFactor = 0.0;
    if(cosAng > Lights[i].Cutoff)
        spotFactor = pow(cosAng, Lights[i].Exponent);

    diffuse = texColor * sDotN;
    if (sDotN > 0.0){
        vec3 v = normalize(-position.xyz);
        vec3 h = normalize(v + s);
        spec = Material.Ks * pow(max(dot(h,n), 0.0), Material.Shininess);
    }

    return ambient + spotFactor * (diffuse + spec) * Lights[i].L;
}

void main()
{
    vec3 n = normalize(Normal);
    vec3 color = vec3(0.0);
    for(int i = 0; i < NUM_LIGHTS; i++){
        color += blinnPhongLight(i, Position, n);
    }

    FragColor = vec4(color, 1.0);
}
