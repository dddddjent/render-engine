#extension GL_EXT_nonuniform_qualifier : enable

#define BindlessDescriptorSet 0

#define BindlessUniformBinding 0
#define BindlessStorageBinding 1
#define BindlessSamplerBinding 2

#define GetLayoutVariableName(Name) u##Name##Register

// Access a specific resource
#define GetResource(Name, Index) \
    GetLayoutVariableName(Name)[Index]

layout(set = BindlessDescriptorSet, binding = BindlessSamplerBinding)
    uniform sampler2D texture2Ds[];

layout(set = BindlessDescriptorSet, binding = BindlessSamplerBinding)
    uniform sampler3D texture3Ds[];

#define Handle uint

const float gamma = 2.2;
const float exposure = 1.0;

vec3 linearToSrgb(vec3 v)
{
    return pow(v, vec3(1.0 / gamma));
}

vec3 srgbToLinear(vec3 v)
{
    return pow(v, vec3(gamma));
}

bool selectPixel(int i, int j, vec4 GL_FragCoord)
{
    return GL_FragCoord.x > i && GL_FragCoord.y > j && GL_FragCoord.x <= i + 1 && GL_FragCoord.y <= j + 1;
}

vec3 rgb_to_hsv(vec3 rgb) {
    float maxC = max(rgb.r, max(rgb.g, rgb.b));
    float minC = min(rgb.r, min(rgb.g, rgb.b));
    float delta = maxC - minC;

    float hue;
    if (delta == 0.0) {
        hue = 0.0;
    } else if (maxC == rgb.r) {
        hue = mod((rgb.g - rgb.b) / delta, 6.0);
    } else if (maxC == rgb.g) {
        hue = (rgb.b - rgb.r) / delta + 2.0;
    } else {
        hue = (rgb.r - rgb.g) / delta + 4.0;
    }
    hue *= 60.0;
    if (hue < 0.0) hue += 360.0;

    float saturation = (maxC == 0.0) ? 0.0 : delta / maxC;

    float value = maxC;

    return vec3(hue, saturation, value);
}

vec3 hsv_to_rgb(vec3 hsv) {
    float c = hsv.z * hsv.y;
    float x = c * (1.0 - abs(mod(hsv.x / 60.0, 2.0) - 1.0));
    float m = hsv.z - c;

    vec3 rgb;
    if (0.0 <= hsv.x && hsv.x < 60.0) {
        rgb = vec3(c, x, 0.0);
    } else if (60.0 <= hsv.x && hsv.x < 120.0) {
        rgb = vec3(x, c, 0.0);
    } else if (120.0 <= hsv.x && hsv.x < 180.0) {
        rgb = vec3(0.0, c, x);
    } else if (180.0 <= hsv.x && hsv.x < 240.0) {
        rgb = vec3(0.0, x, c);
    } else if (240.0 <= hsv.x && hsv.x < 300.0) {
        rgb = vec3(x, 0.0, c);
    } else {
        rgb = vec3(c, 0.0, x);
    }

    return rgb + vec3(m, m, m);
}
