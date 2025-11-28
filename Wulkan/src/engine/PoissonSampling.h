#pragma once

#include "vk_wrap/VKW_Object.h"

#include "tph_poisson.h"
#include "glm/glm.hpp"

class PoissonSampling
{
public:
	PoissonSampling() = default;
	/*
		Generate poisson samples
			bound: area in which samples lie [-bound, bound]^2
			min_nr_samples: minimum nr of samples guaranteed to be computed
			starting_radius: Guaranteed minimum distance to attempt to get min_nr_samples, will be lowered if unsuccessful at generating samples
			seed: Seed used for computation
	*/
	void init(float bound, int min_nr_samples, float starting_radius, uint64_t seed);
private:
	std::vector<glm::vec2> samples;
public:
	const std::vector<glm::vec2>& get_sampler() const { return samples; };
};

