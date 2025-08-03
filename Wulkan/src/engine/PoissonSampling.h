#pragma once

#include "common.h"
#include "vk_wrap/VKW_Object.h"

#include "tph_poisson.h"
#include "glm/glm.hpp"

class PoissonSampling
{
public:
	PoissonSampling() = default;
	void init(float bound, int min_nr_samples, float starting_radius, uint64_t seed);
private:
	std::vector<glm::vec2> samples;
public:
	const std::vector<glm::vec2>& get_sampler() const { return samples; };
};

