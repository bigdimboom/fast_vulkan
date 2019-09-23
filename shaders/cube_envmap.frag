#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Return Output
layout (location = 0) out vec4 out_FragColor;

// textured map
layout (location = 0) in vec2 v_TexCoords;
layout (binding = 1) uniform sampler2D tex2D;

layout (push_constant) uniform Tweek
{
    float base_rate;
}tweek;


// envriment map
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec3 Position;
layout (location = 3) in vec3 CameraPos;
layout (binding = 2) uniform samplerCube envrmap;

void main()
{
    //float ratio = 1.00 / 1.52;
    vec3 I = normalize(Position - CameraPos);
    vec3 R = reflect(I, normalize(Normal));
    //vec3 R = refract(I, normalize(Normal), ratio);
    out_FragColor = vec4(texture(envrmap, R).rgb, 1.0f) * tweek.base_rate + 
                    vec4(texture(tex2D, v_TexCoords).rgb, 1.0f) * (1.0 - tweek.base_rate);
}