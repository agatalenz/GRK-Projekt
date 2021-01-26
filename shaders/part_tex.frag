#version 330 core

in float lifeTime;
out vec4 fragColor;

in vec2 interpTexCoord;
uniform sampler2D textureSampler;

void main()
{
    vec4 color = texture2D(textureSampler, interpTexCoord);

    float normalizedLifeTime = lifeTime / 2; 
    fragColor = vec4( color);
};