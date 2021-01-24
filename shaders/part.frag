#version 330 core

in float lifeTime;
out vec4 fragColor;

void main()
{
    float normalizedLifeTime = lifeTime / 2; 
    fragColor = vec4( 1.f, normalizedLifeTime, 0.f, normalizedLifeTime);
};