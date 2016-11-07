#version 450 core

layout(origin_upper_left) in vec4 gl_FragCoord;

layout(binding = 0) uniform fractal_params {
    float   fp_width;
    float   fp_height;
    vec2    fp_c;
    uint    fp_max_iterations;
};

in VS_OUT_PS_IN
{
    layout(location = 0) vec2 position;
} ps_in;

out vec4 frag_color;

float bernstein_red(const float t) {
    return 9.0f * (1.0f - t) * t * t * t;
}

float bernstein_green(const float t) {
    return 15.0f * (1.0f - t) * (1.0f - t) * t * t;
}

float bernstein_blue(const float t) {
    return 8.5f * (1.0f - t) * (1.0f - t) * (1.0f - t) * t;
}

vec4 bernstein_coloring(const uint iter_cnt, const uint max_iterations) {
    const float factor = float(iter_cnt) / float(max_iterations);
    vec4 res = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    res.r = bernstein_red(factor);
    res.g = bernstein_green(factor);
    res.b = bernstein_blue(factor);

    return res;
}

vec4 smooth_iteration_count_coloring(const uint iter_cnt, const float z_abs) {
    const float n = float(iter_cnt) + 3.0 - log(log(z_abs)) / log(2.0);

    vec4 final_color;

    final_color.r = (-cos(0.025 * n) + 1.0) * 0.5;
    final_color.g = (-cos(0.080 * n) + 1.0) * 0.5;
    final_color.b = (-cos(0.120 * n) + 1.0) * 0.5;
    final_color.a = 1.0;

    return final_color;
}

struct complex_t {
    float re;
    float im;
};

complex_t complex_add(const complex_t c0, const complex_t c1) {
    return complex_t(c0.re + c1.re, c0.im + c1.im);
}

complex_t complex_mul(const complex_t c0, const complex_t c1) {
    return complex_t(
        c0.re * c1.re - c0.im * c1.im,
        c0.re * c1.im + c0.im * c1.re);
}

float complex_abs_squared(const complex_t c) {
    return c.re * c.re + c.im * c.im;
}

float complex_abs(const complex_t c) {
    return sqrt(complex_abs_squared(c));
}

bool is_equal(const float a, const float b) {
    return abs(a - b) < 1.0e-4;
}

bool complex_eq(const complex_t c0, const complex_t c1) {
    return is_equal(c0.re, c1.re) && is_equal(c0.im, c1.im);
}

complex_t clip_to_complex(const vec2 scr_pos) {
    //
    //  Fragment is in clip coordinates [-1, 1]x[-1, 1]
    //  Julia domain needs [-2.5, 1]x[-1, 1]
    complex_t c;
    c.re = 3.5 * scr_pos.x * 0.5f;
    c.im = scr_pos.y;

    return c;
}

complex_t screen_to_complex(const vec2 screen_coords) {
    //
    //  Fragment is in window coords [0, fp_width]x[0, fp_height]
    //  Julia domain is [-2.5, 1]x[-1, 1]
    complex_t c;
    c.re = 3.5 * (screen_coords.x - fp_width * 0.5) / fp_width;
    c.im = 2.0 * (screen_coords.y - fp_height * 0.5) / fp_height;

    return c;
}

void main() {
    complex_t z = screen_to_complex(gl_FragCoord.xy);
    // clip_to_complex(ps_in.position);
    complex_t z_prev = z;
    const complex_t C = complex_t(fp_c.x, fp_c.y);

    uint iter_cnt = 0;

    while ((complex_abs_squared(z) <= 4.0f) && (iter_cnt < fp_max_iterations)) {
        z = complex_add(complex_mul(z, z), C);

        if (complex_eq(z, z_prev)) {
            break;
        }

        z_prev = z;
        ++iter_cnt;
    }

    frag_color = bernstein_coloring(iter_cnt, fp_max_iterations);
    //frag_color = smooth_iteration_count_coloring(iter_cnt, complex_abs(z));
}
