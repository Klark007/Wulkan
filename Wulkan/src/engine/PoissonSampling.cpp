#include "common.h"
#define TPH_POISSON_IMPLEMENTATION
#include "PoissonSampling.h"

void PoissonSampling::init(float bound, int min_nr_samples, float starting_radius, uint64_t seed)
{
	tph_poisson_sampling sampling{};
	std::array<tph_poisson_real, 2> bounds_min{ -bound, -bound };
	std::array<tph_poisson_real, 2> bounds_max{ bound, bound };
	float radius = starting_radius;

	while (true) {
		tph_poisson_args args = {};
		args.radius = radius;
		args.ndims = 2;
		args.bounds_min = bounds_min.data();
		args.bounds_max = bounds_max.data();
		args.max_sample_attempts = 30;
		args.seed = seed;

		if (const int ret = tph_poisson_create(&args, nullptr, &sampling);
			ret != TPH_POISSON_SUCCESS) {
			throw RuntimeException(fmt::format("Failed to create poisson sampling ({})", ret), __FILE__, __LINE__);
		};

		// retry until we reach min nr samples
		if (sampling.nsamples >= min_nr_samples) {
			break;
		}
		else {
			tph_poisson_destroy(&sampling);
			radius /= 2;
		}		
	}

	const tph_poisson_real* data = tph_poisson_get_samples(&sampling);

	samples.reserve(sampling.nsamples);

	for (int i = 0; i < sampling.nsamples; i++) {
		samples.push_back({ data[2 * i], data[2 * i + 1] });
	}

	tph_poisson_destroy(&sampling);
}
