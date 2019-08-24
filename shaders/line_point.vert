#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Attributes
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec4 in_ColorPointSize;

layout (location = 0) out vec4 v_Color;

layout(binding = 0) uniform UBO {
    mat4 u_MvpMatrix;
} uboVS;

out gl_PerVertex 
{
    vec4 gl_Position;
    float gl_PointSize;
};

void main()
{
    gl_Position  = uboVS.u_MvpMatrix * vec4(in_Position, 1.0);
    gl_Position.y = -gl_Position.y; // quick hack, vulkan NDS is flipped
    gl_PointSize = in_ColorPointSize.w;
    v_Color = vec4(in_ColorPointSize.xyz, 1.0);
}