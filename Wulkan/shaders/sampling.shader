#ifndef SAMPLING_INCLUDE
#define SAMPLING_INCLUDE

// samples sample_idx out of sample_count many samples and rotates by phi
vec2 sample_vogel_disk(int sample_idx, int sample_count, float phi)
{
    float golden_angle = 2.4f;

    float r = sqrt(sample_idx + 0.5f) / sqrt(sample_count);
    float theta = sample_idx * golden_angle + phi;

    float sine= sin(theta);
    float cosine = cos(theta);

    return vec2(r*cosine, r*sine);
}

// given screen_pos return a random value in [0,1]
float interleaved_gradient_noise(vec2 screen_pos)
{
    vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
    return fract(magic.z * fract(dot(screen_pos, magic.xy)));
}

#endif 