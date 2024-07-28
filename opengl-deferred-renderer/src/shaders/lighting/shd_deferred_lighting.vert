#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

uniform vec2 position;
uniform vec2 scale;

void main()
{
    vec2 pos = (aPos.xy*scale)+position;

    gl_Position = vec4(pos.x, pos.y, 0.0, 1.0); 
    TexCoords = aTexCoords;
}  