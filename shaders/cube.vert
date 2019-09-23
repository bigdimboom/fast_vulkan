#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Attributes
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec2 in_uv;

layout (binding = 0) uniform UBO { 
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 campos;
} ubo;

layout (location = 0) out vec2 v_TexCoords;

// environment map
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec3 Position;
layout (location = 3) out vec3 CameraPos;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main()
{
    Normal = mat3(transpose(inverse(ubo.model))) * in_Normal;
    Position = vec3(ubo.model * vec4(in_Position, 1.0));
    CameraPos = ubo.campos;

    v_TexCoords = in_uv;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_Position, 1.0);
    gl_Position.y = -gl_Position.y;
}