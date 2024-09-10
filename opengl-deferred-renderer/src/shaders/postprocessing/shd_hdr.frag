#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float exposure;
uniform bool useHdr;


// Constants
const float SRGB_GAMMA = 1.0 / 2.2;
const float SRGB_INVERSE_GAMMA = 2.2;
const float SRGB_ALPHA = 0.055;


// Used to convert from linear RGB to XYZ space
const mat3 RGB_2_XYZ = (mat3(
    0.4124564, 0.2126729, 0.0193339,
    0.3575761, 0.7151522, 0.1191920,
    0.1804375, 0.0721750, 0.9503041
));

// Used to convert from XYZ to linear RGB space
const mat3 XYZ_2_RGB = (mat3(
     3.2404542,-0.9692660, 0.0556434,
    -1.5371385, 1.8760108,-0.2040259,
    -0.4985314, 0.0415560, 1.0572252
));

const vec3 LUMA_COEFFS = vec3(0.2126, 0.7152, 0.0722);

/***** Utility Functions *****/

// Returns the luminance of a !! linear !! rgb color
float get_luminance(vec3 rgb) {
    return dot(LUMA_COEFFS, rgb);
}

// Converts a linear rgb color to a srgb color (approximated, but fast)
vec3 rgb_to_srgb_approx(vec3 rgb) {
    return pow(rgb, vec3(SRGB_GAMMA));
}

// Converts a srgb color to a rgb color (approximated, but fast)
vec3 srgb_to_rgb_approx(vec3 srgb) {
    return pow(srgb, vec3(SRGB_INVERSE_GAMMA));
}

// Converts a single linear channel to srgb
float linear_to_srgb(float channel) {
    if(channel <= 0.0031308)
        return 12.92 * channel;
    else
        return (1.0 + SRGB_ALPHA) * pow(channel, 1.0/2.4) - SRGB_ALPHA;
}

// Converts a single srgb channel to rgb
float srgb_to_linear(float channel) {
    if (channel <= 0.04045)
        return channel / 12.92;
    else
        return pow((channel + SRGB_ALPHA) / (1.0 + SRGB_ALPHA), 2.4);
}

// Converts a linear rgb color to a srgb color (exact, not approximated)
vec3 rgb_to_srgb(vec3 rgb) {
    return vec3(
        linear_to_srgb(rgb.r),
        linear_to_srgb(rgb.g),
        linear_to_srgb(rgb.b)
    );
}

// Converts a srgb color to a linear rgb color (exact, not approximated)
vec3 srgb_to_rgb(vec3 srgb) {
    return vec3(
        srgb_to_linear(srgb.r),
        srgb_to_linear(srgb.g),
        srgb_to_linear(srgb.b)
    );
}

// Converts a color from linear RGB to XYZ space
vec3 rgb_to_xyz(vec3 rgb) {
    return RGB_2_XYZ * rgb;
}

// Converts a color from XYZ to linear RGB space
vec3 xyz_to_rgb(vec3 xyz) {
    return XYZ_2_RGB * xyz;
}

/***** Tone Mapping (Kram's Method) *****/

//1. Calculate brightness
float find_intensity_estimate(vec3 color_xyz) {
    return (dot(color_xyz, color_xyz)/(dot(color_xyz, vec3(1.0, 1.0, 1.0))));
}

//2. Push brightness through a preshaper. Track derivative.
float hdr_shaper(float l) {    
    //split L into absolute and sign to consistently handle negative values.
    //possible to simplify if you can guarantee positive values, but for now...
    float abs_l = abs(l);
    float sign_of_l = sign(l); 

    if (abs_l > 1) {
        return sign_of_l*(SRGB_GAMMA*log(abs_l)+1);
    }
    return sign_of_l*pow(abs_l, SRGB_GAMMA);
}
float get_hdr_shaper_derivative(float l) {
    float abs_l = abs(l);

    if (abs_l > 1) {
        return 1.0/(SRGB_GAMMA*l);
    }
    return sign(l) * SRGB_GAMMA * pow(abs_l, SRGB_GAMMA-1.0);
}

//3. Push brightness through sigmoid compressor. Track derivative.

//quick struct to give some semantic meaning to a few magic numbers we're gonna just invent later on in the shader
struct ToneMapConfig {
    float w; 
    float b; 
    float g_in; 
    float g_out; 
    float s;
};

//Defined to get around duplicate processing in sigmoid and sigmoid derivate functions. Not entirely sure what to call this.
//It's probably faster to cache rather than duplicate calculations, but do note the memory overhead caused by this approach.
struct SigmoidUtility {
    float range;
    float lgi;
    float ghi;
    float L;
    float L_lo;
    float L_lohi;
};

SigmoidUtility preprocess_sigmoid(float l, ToneMapConfig cfg) {
    float lgi = l / cfg.g_in;
    float sgi = cfg.s * cfg.g_in;
    float range = cfg.w - cfg.b;
    float glo = cfg.b - cfg.g_out;
    float ghi = cfg.g_out - cfg.w;

    float sgir = range * sgi;
    float lohi = glo * ghi;

    float L = pow(lgi, sgir/lohi);
    float L_lo = L * glo;
    float L_lohi = L_lo + ghi;

    return SigmoidUtility(range, lgi, ghi, L, L_lo, L_lohi);
}

float sigmoid(float w, float b, SigmoidUtility su) {
    //the actual sigmoid
    float B = b * su.ghi;
    return (w * su.L_lo + B)/su.L_lohi;
}
float sigmoid_derivative(float s, SigmoidUtility su) {
    float dL = su.range * su.range * s / su.lgi;
    return (dL*su.L)/(su.L_lohi * su.L_lohi);
}

float get_min_component(vec3 v) {
    return min(v.x, min(v.y, v.z));
}
float get_max_component(vec3 v) {
    return max(v.x, max(v.y, v.z));
}

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture(scene, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    hdrColor+=bloomColor;
    if(useHdr)
    {
        vec3 xyzColor = rgb_to_xyz(hdrColor);
        float intensityEstimate = find_intensity_estimate(xyzColor);
        float ieShaped = hdr_shaper(intensityEstimate);
        float ieShapedDerivative = hdr_shaper(intensityEstimate);

        ToneMapConfig cfg = ToneMapConfig(1.0, 0.0, 0.5, 0.5, 1.0);
        SigmoidUtility su = preprocess_sigmoid(ieShaped, cfg);

        float ieCompressed = sigmoid(cfg.w, cfg.b, su);
        float ieCompressedDerivative = sigmoid_derivative(cfg.s, su) * ieShapedDerivative; //chain rule
        //at this point, the gray values are completely specified by ieCompressed. The derivative is needed to get the color information back.

        //4. Deal with color.
        vec3 sceneColor = xyzColor - intensityEstimate; //component-wise subtraction
        vec3 xyzCompressed = ieCompressed + ieCompressedDerivative * sceneColor;

        //We now have valid colors, but it's possible to escape the gamut, so we need to regularize.

        //5. Regularize
        float ieNew = find_intensity_estimate(xyzCompressed);
        vec3 rgbCompressed = xyz_to_rgb(xyzCompressed);
        //deal with negative values
        if (get_min_component(rgbCompressed) < 0) {
            rgbCompressed = rgbCompressed-get_min_component(rgbCompressed); 
        }
        vec3 xyzPositive = rgb_to_xyz(rgbCompressed);
        vec3 xyzRestored = xyzPositive * (ieNew / find_intensity_estimate(xyzPositive));
        vec3 rgbRestored = xyz_to_rgb(xyzRestored);
        hdrColor = rgbRestored;
        if (get_max_component(hdrColor) > 1) {
            hdrColor = hdrColor/get_max_component(hdrColor);
        }
        
        //gamma correction has already been applied, so don't do any additional conversion stuff.

        vec3 result = vec3(1.0) - exp(-hdrColor * exposure);

        FragColor = vec4(result, 1.0);
    }
    else
    {
        vec3 result = pow(hdrColor, vec3(1.0 / gamma));
        FragColor = vec4(result, 1.0);
    }
}  