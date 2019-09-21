#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Attributes
layout (location = 0) in vec3 in_Position;

layout (binding = 0) uniform UBO { 
    mat4 view;
    mat4 proj;
} ubo;

layout (location = 0) out vec3 v_TexCoords;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main()
{
    mat4 model = mat4(999999.0f);
    model[3][3] = 1.0f;

    v_TexCoords = in_Position;
    vec4 pos = ubo.proj * ubo.view * model * vec4(in_Position, 1.0);
    gl_Position = pos.xyww;
    gl_Position.y = -gl_Position.y;
}