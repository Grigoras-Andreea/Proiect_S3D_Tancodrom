#version 330 core

in vec2 MidTexCoords;

out vec4 OutColor;

uniform sampler2D texture_diffuse1;

void main()
{
    //OutColor = vec4(1);

    OutColor = texture(texture_diffuse1, MidTexCoords);
}