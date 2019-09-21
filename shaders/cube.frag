#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Varying
layout (location = 0) in vec2 v_TexCoords;

layout (binding = 1) uniform sampler2D tex2D;

// Return Output
layout (location = 0) out vec4 out_FragColor;

void main()
{
    out_FragColor = texture(tex2D, v_TexCoords);
}