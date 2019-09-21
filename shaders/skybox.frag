#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Varying
layout (location = 0) in vec3 v_TexCoords;

layout (binding = 1) uniform samplerCube skybox;

// Return Output
layout (location = 0) out vec4 out_FragColor;

void main()
{
    out_FragColor = texture(skybox, v_TexCoords);
}