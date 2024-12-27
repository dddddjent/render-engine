#version 450

#extension GL_GOOGLE_include_directive : enable

#include "../../shader/common.glsl"

layout(set = 1, binding = 0) uniform PipelineParam
{
    Handle hdr_image;
}
pipelineParam;

layout(location = 0) out vec4 outColor;

vec3 toneRemap(vec3 color)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    color = (color * (a * color + b)) / (color * (c * color + d) + e);

    return color;
}

void main()
{
    vec3 color = texelFetch(texture2Ds[pipelineParam.hdr_image], ivec2(gl_FragCoord.xy), 0).rgb;
    color = toneRemap(color);
    color = linearToSrgb(color);
    color = clamp(color, 0.0, 1.0);
    outColor = vec4(color, 1.0);
}
