#version 450

#extension GL_GOOGLE_include_directive : enable

#include "../../shader/common.glsl"

layout(set = 1, binding = 0) uniform PipelineParam
{
    Handle original_image;
}
pipelineParam;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 color = texelFetch(texture2Ds[pipelineParam.original_image], ivec2(gl_FragCoord.xy), 0).rgb;
    outColor = vec4(color, 1.0f);
}
