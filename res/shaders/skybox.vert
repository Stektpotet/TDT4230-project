#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal;
in layout(location = 2) vec2 uv;
in layout(location = 3) vec3 tangent;
in layout(location = 4) vec3 bitangent;

uniform layout(location = 0) mat4 MVP;
uniform layout(location = 1) mat4 TRS;
uniform layout(location = 2) mat3 NRM;
uniform layout(location = 3) vec3 eye;

out layout(location = 0) vec4 position_out;
out layout(location = 1) vec3 normal_out;
out layout(location = 2) vec2 uv_out;
out layout(location = 3) mat3 TBN;

void main()
{
    uv_out = uv;
    normal_out = normalize(NRM * normalize(normal));
    TBN = mat3( // Tangent space to world space
        normalize(NRM * normalize(tangent)),
        normalize(NRM * normalize(bitangent)),
        normal_out
    );
    gl_Position = MVP * vec4(position, 1.0);
    position_out = TRS * vec4(position, 1.0);
}
