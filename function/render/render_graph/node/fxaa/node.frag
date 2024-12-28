#version 450

#extension GL_GOOGLE_include_directive : enable

#include "../../shader/common.glsl"

// 0.0312 - 0.0833
const float CONTRAST_ABS_THRESHOLD = 0.0625;
// 0.063 - 0.333
const float CONTRAST_REL_THRESHOLD = 0.125;
// 0.0 - 1.0
const float PIXEL_BLENDING_FACTOR  = 0.75;

#if defined(FXAA_QUALITY_LOW)
#define EXTRA_EDGE_STEPS 3
#define EDGE_STEP_SIZES 1.5, 2.0, 2.0
#define LAST_EDGE_STEP_GUESS 8.0
#elif defined(FXAA_QUALITY_MEDIUM)
#define EXTRA_EDGE_STEPS 8
#define EDGE_STEP_SIZES 1.5, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 4.0
#define LAST_EDGE_STEP_GUESS 8.0
#else
#define EXTRA_EDGE_STEPS 10
#define EDGE_STEP_SIZES 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0
#define LAST_EDGE_STEP_GUESS 8.0
#endif

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

vec2 frameBufSize = vec2(camera.width, camera.height);
vec2 pixelSize    = 1.0 / frameBufSize;

vec4 fastFXAA()
{
    float FXAA_SPAN_MAX   = 8.0;
    float FXAA_REDUCE_MUL = 1.0 / 8.0;
    float FXAA_REDUCE_MIN = 1.0 / 128.0;

    float lumaNW = texture(buf0, (gl_FragCoord.xy + vec2(-1.0, -1.0)) / frameBufSize).a;
    float lumaNE = texture(buf0, (gl_FragCoord.xy + vec2(1.0, -1.0)) / frameBufSize).a;
    float lumaSW = texture(buf0, (gl_FragCoord.xy + vec2(-1.0, 1.0)) / frameBufSize).a;
    float lumaSE = texture(buf0, (gl_FragCoord.xy + vec2(1.0, 1.0)) / frameBufSize).a;
    float lumaM  = texture(buf0, gl_FragCoord.xy).a;

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce
        = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
              max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin))
        / frameBufSize;

    vec4 rgbA = (1.0 / 2.0)
        * (texture(buf0, gl_FragCoord.xy / frameBufSize + dir * (1.0 / 3.0 - 0.5))
           + texture(buf0, gl_FragCoord.xy / frameBufSize + dir * (2.0 / 3.0 - 0.5)));
    vec4 rgbB = rgbA * (1.0 / 2.0)
        + (1.0 / 4.0)
            * (texture(buf0, gl_FragCoord.xy / frameBufSize + dir * (0.0 / 3.0 - 0.5))
               + texture(buf0, gl_FragCoord.xy / frameBufSize + dir * (3.0 / 3.0 - 0.5)));
    float lumaB = rgbB.a;

    if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
        return vec4(rgbA.rgb, 1.0f);
    } else {
        return vec4(rgbB.rgb, 1.0f);
    }
}

struct LumaNeighborhood {
    float m, n, e, s, w, ne, se, sw, nw;
    float highest, lowest, range;
};

struct FXAAEdge {
    bool isHorizontal;
    float pixelStep;
    float lumaGradient, otherLuma;
};

// coord: 0.0 - 1.0
float getLuma(vec2 coord) { return texture(buf0, coord).a; }

float getLuma(vec2 coord, float uOffset, float vOffset)
{
    coord += vec2(uOffset, vOffset) * pixelSize;
    return texture(buf0, coord).a;
}

LumaNeighborhood GetLumaNeighborhood(vec2 uv)
{
    LumaNeighborhood luma;
    luma.m  = getLuma(uv);
    luma.n  = getLuma(uv, 0.0, 1.0);
    luma.e  = getLuma(uv, 1.0, 0.0);
    luma.s  = getLuma(uv, 0.0, -1.0);
    luma.w  = getLuma(uv, -1.0, 0.0);
    luma.ne = getLuma(uv, 1.0, 1.0);
    luma.se = getLuma(uv, 1.0, -1.0);
    luma.sw = getLuma(uv, -1.0, -1.0);
    luma.nw = getLuma(uv, -1.0, 1.0);

    luma.highest = max(max(max(max(luma.m, luma.n), luma.e), luma.s), luma.w);
    luma.lowest  = min(min(min(min(luma.m, luma.n), luma.e), luma.s), luma.w);
    luma.range   = luma.highest - luma.lowest;
    return luma;
}

bool CanSkipFXAA(LumaNeighborhood luma)
{
    return luma.range < max(CONTRAST_ABS_THRESHOLD, CONTRAST_REL_THRESHOLD * luma.highest);
}

bool IsHorizontalEdge(LumaNeighborhood luma)
{
    float horizontal = 2.0 * abs(luma.n + luma.s - 2.0 * luma.m)
        + abs(luma.ne + luma.se - 2.0 * luma.e)
        + abs(luma.nw + luma.sw - 2.0 * luma.w);
    float vertical = 2.0 * abs(luma.e + luma.w - 2.0 * luma.m)
        + abs(luma.ne + luma.nw - 2.0 * luma.n)
        + abs(luma.se + luma.sw - 2.0 * luma.s);
    return horizontal >= vertical;
}

FXAAEdge GetFXAAEdge(LumaNeighborhood luma)
{
    FXAAEdge edge;
    edge.isHorizontal = IsHorizontalEdge(luma);
    float lumaP, lumaN;
    if (edge.isHorizontal) {
        edge.pixelStep = pixelSize.y;
        lumaP          = luma.n;
        lumaN          = luma.s;
    } else {
        edge.pixelStep = pixelSize.x;
        lumaP          = luma.e;
        lumaN          = luma.w;
    }
    float gradientP = abs(lumaP - luma.m);
    float gradientN = abs(lumaN - luma.m);

    if (gradientP < gradientN) {
        edge.pixelStep    = -edge.pixelStep;
        edge.lumaGradient = gradientN;
        edge.otherLuma    = lumaN;
    } else {
        edge.lumaGradient = gradientP;
        edge.otherLuma    = lumaP;
    }

    return edge;
}

float GetSubpixelBlendFactor(LumaNeighborhood luma)
{
    float _filter = 2.0 * (luma.n + luma.e + luma.s + luma.w);
    _filter += luma.ne + luma.nw + luma.se + luma.sw;
    _filter *= 1.0 / 12.0;
    _filter = abs(_filter - luma.m);
    _filter = clamp(_filter / luma.range, 0.0, 1.0);
    _filter = smoothstep(0, 1, _filter);
    return _filter * _filter * PIXEL_BLENDING_FACTOR;
}

const float edgeStepSizes[EXTRA_EDGE_STEPS] = { EDGE_STEP_SIZES };
float GetEdgeBlendFactor(LumaNeighborhood luma, FXAAEdge edge, vec2 uv)
{
    vec2 edgeUV = uv;
    vec2 uvStep = vec2(0.0);
    if (edge.isHorizontal) {
        edgeUV.y += 0.5 * edge.pixelStep;
        uvStep.x = pixelSize.x;
    } else {
        edgeUV.x += 0.5 * edge.pixelStep;
        uvStep.y = pixelSize.y;
    }

    float edgeLuma          = 0.5 * (luma.m + edge.otherLuma);
    float gradientThreshold = 0.25 * edge.lumaGradient;

    vec2 uvP         = edgeUV + uvStep;
    float lumaDeltaP = getLuma(uvP) - edgeLuma;
    bool atEndP      = abs(lumaDeltaP) >= gradientThreshold;

    int i;
#pragma unroll
    for (i = 0; i < EXTRA_EDGE_STEPS; i++) {
        if (atEndP)
            break;
        uvP += uvStep * edgeStepSizes[i];
        lumaDeltaP = getLuma(uvP) - edgeLuma;
        atEndP     = abs(lumaDeltaP) >= gradientThreshold;
    }
    if (!atEndP) {
        uvP += uvStep * LAST_EDGE_STEP_GUESS;
    }

    vec2 uvN         = edgeUV - uvStep;
    float lumaDeltaN = getLuma(uvN) - edgeLuma;
    bool atEndN      = abs(lumaDeltaN) >= gradientThreshold;

#pragma unroll
    for (i = 0; i < EXTRA_EDGE_STEPS; i++) {
        if (atEndN)
            break;
        uvN -= uvStep * edgeStepSizes[i];
        lumaDeltaN = getLuma(uvN) - edgeLuma;
        atEndN     = abs(lumaDeltaN) >= gradientThreshold;
    }
    if (!atEndN) {
        uvN -= uvStep * LAST_EDGE_STEP_GUESS;
    }

    float distanceToEndP, distanceToEndN;
    if (edge.isHorizontal) {
        distanceToEndP = uvP.x - uv.x;
        distanceToEndN = uv.x - uvN.x;
    } else {
        distanceToEndP = uvP.y - uv.y;
        distanceToEndN = uv.y - uvN.y;
    }

    float distanceToNearestEnd;
    bool deltaSign;
    if (distanceToEndP <= distanceToEndN) {
        distanceToNearestEnd = distanceToEndP;
        deltaSign            = lumaDeltaP >= 0;
    } else {
        distanceToNearestEnd = distanceToEndN;
        deltaSign            = lumaDeltaN >= 0;
    }

    if (deltaSign == (luma.m - edgeLuma >= 0)) {
        return 0.0;
    } else {
        return 0.5 - distanceToNearestEnd / (distanceToEndP + distanceToEndN);
    }
}

vec4 FXAA()
{
    ivec2 center      = ivec2(gl_FragCoord.xy);
    vec2 centerUV     = (center + 0.5) * pixelSize;
    vec3 center_color = texelFetch(buf0, center, 0).rgb;

    LumaNeighborhood luma = GetLumaNeighborhood(centerUV);
    if (CanSkipFXAA(luma)) {
        return vec4(center_color, 1);
    }

    FXAAEdge edge = GetFXAAEdge(luma);

    float blendFactor
        = max(GetSubpixelBlendFactor(luma), GetEdgeBlendFactor(luma, edge, centerUV));
    vec2 blendUV = centerUV;
    if (edge.isHorizontal) {
        blendUV.y += blendFactor * edge.pixelStep;
    } else {
        blendUV.x += blendFactor * edge.pixelStep;
    }

    return vec4(texture(buf0, blendUV).rgb, 1.0f);
}

void main()
{
    // outColor = fastFXAA();
    outColor = FXAA();
}
