#version 450

#extension GL_GOOGLE_include_directive : enable

#include "../../shader/common.glsl"

layout(set = BindlessDescriptorSet, binding = BindlessUniformBinding) uniform Camera
{
    mat4 view;
    mat4 proj;
    vec3 eye_w;
    float fov;
    vec3 view_dir;
    float aspect_ratio;
    vec3 up;
    float focal_distance;
    int width;
    int height;
}
GetLayoutVariableName(camera)[];

layout(set = 1, binding = 0) uniform PipelineParam
{
    Handle original_image;
    Handle camera;
}
pipelineParam;

layout(location = 0) out vec4 outColor;

#define camera GetResource(camera, pipelineParam.camera)
#define buf0 texture2Ds[pipelineParam.original_image]

vec4 fastFXAA()
{
    float FXAA_SPAN_MAX = 8.0;
    float FXAA_REDUCE_MUL = 1.0 / 8.0;
    float FXAA_REDUCE_MIN = 1.0 / 128.0;

    vec2 frameBufSize = vec2(camera.width, camera.height);
    float lumaNW = texture(buf0, (gl_FragCoord.xy + vec2(-1.0, -1.0)) / frameBufSize).a;
    float lumaNE = texture(buf0, (gl_FragCoord.xy + vec2(1.0, -1.0)) / frameBufSize).a;
    float lumaSW = texture(buf0, (gl_FragCoord.xy + vec2(-1.0, 1.0)) / frameBufSize).a;
    float lumaSE = texture(buf0, (gl_FragCoord.xy + vec2(1.0, 1.0)) / frameBufSize).a;
    float lumaM = texture(buf0, gl_FragCoord.xy).a;

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
              max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
                  dir * rcpDirMin))
        / frameBufSize;

    vec4 rgbA = (1.0 / 2.0) * (texture(buf0, gl_FragCoord.xy / frameBufSize + dir * (1.0 / 3.0 - 0.5)) + texture(buf0, gl_FragCoord.xy / frameBufSize + dir * (2.0 / 3.0 - 0.5)));
    vec4 rgbB = rgbA * (1.0 / 2.0) + (1.0 / 4.0) * (texture(buf0, gl_FragCoord.xy / frameBufSize + dir * (0.0 / 3.0 - 0.5)) + texture(buf0, gl_FragCoord.xy / frameBufSize + dir * (3.0 / 3.0 - 0.5)));
    float lumaB = rgbB.a;

    if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
        return vec4(rgbA.rgb, 1.0f);
    } else {
        return vec4(rgbB.rgb, 1.0f);
    }
}

void main()
{
    ivec2 fragCoord = ivec2(gl_FragCoord.xy);
    if (fragCoord.x >= camera.width - 1 || fragCoord.y >= camera.height - 1
        || fragCoord.x <= 0 || fragCoord.y <= 0) {
        outColor = vec4(vec3(0.0f), 1.0f);
        return;
    }

    outColor = fastFXAA();
}
