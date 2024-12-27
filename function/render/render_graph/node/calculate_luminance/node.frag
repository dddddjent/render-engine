#version 450

#extension GL_GOOGLE_include_directive : enable

#include "../../shader/common.glsl"

layout(set = 1, binding = 0) uniform PipelineParam
{
    Handle sdr_image;
}
pipelineParam;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 color = texelFetch(texture2Ds[pipelineParam.sdr_image], ivec2(gl_FragCoord.xy), 0).rgb;
    color = srgbToLinear(color);
    float luminance = dot(color, vec3(0.2126729f, 0.7151522f, 0.0721750f));
    outColor = vec4(color, luminance);
}
