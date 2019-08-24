#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Attributes
layout (location = 0) in vec2 in_Position;
layout (location = 1) in vec2 in_TexCoords;
layout (location = 2) in vec3 in_Color;

layout (binding = 0) uniform UBO { 
    vec2 u_screenDimensions;
} ubo;

layout (location = 0) out vec2 v_TexCoords;
layout (location = 1) out vec4 v_Color;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main()
{
    // Map to normalized clip coordinates:
    float x = ((2.0 * (in_Position.x - 0.5)) / ubo.u_screenDimensions.x) - 1.0;
    float y = 1.0 - ((2.0 * (in_Position.y - 0.5)) / ubo.u_screenDimensions.y);

    // float x = ((2.0 * (in_Position.x - 0.5)) / 1280.0) - 1.0;
    // float y = 1.0 - ((2.0 * (in_Position.y - 0.5)) / 720.0);

    gl_Position = vec4(x, y, 0.0, 1.0);
    gl_Position.y = -gl_Position.y;
    v_TexCoords = vec2(in_TexCoords.x, in_TexCoords.y);
    v_Color = vec4(in_Color, 1.0);
}