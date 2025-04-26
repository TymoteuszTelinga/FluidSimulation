#version 460 core

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 UVcord;

layout(location = 0) out vec4 outColor;

vec3 gradient(float value)
{
    vec3 color1 = vec3(0.030f, 0.363f, 0.940f);// blue
    vec3 color2 = vec3(0.030f, 0.940f, 0.421f);// green
    vec3 color3 = vec3(0.940f, 0.664f, 0.030f);// yellow
    vec3 color4 = vec3(0.940f, 0.150f, 0.030f);// red

    if (value < 0.062)
    {
        return color1;
    }
    if (value < 0.5)
    {
        float range = 0.5 - 0.062;
        float pos = value - 0.062;
        float v = pos / range;
        return mix(color1,color2,v);
    } 
    if (value < 0.7)
    {
        float range = 0.7 - 0.5;
        float pos = value - 0.5;
        float v = pos / range;
        return mix(color2,color3,v);
    }
    if (value < 1.0)
    {
        float range = 1.0 - 0.7;
        float pos = value - 0.7;
        float v = pos / range;
        return mix(color3,color4,v);
    }
    
    return color4;
}

void main()
{
    vec2 pos = (UVcord * 2.f) - 1.f;
    //float fade = 100 / 4800;
    float distance = 1.0f - length(pos);

    float alpha = smoothstep(0.f, 0.1f, distance);

    float speed = length(fragColor);
    const float MaxSpeed = 3.0;
    speed = clamp(speed / MaxSpeed, 0.0, 1.0);

    alpha = 1.0f;
    outColor = vec4(gradient(speed), alpha);
}