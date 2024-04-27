#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D cloudTexture;

void main()
{
    FragColor = texture(cloudTexture, TexCoord);
}
