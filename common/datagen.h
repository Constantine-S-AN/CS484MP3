#ifndef DATAGEN_H
#define DATAGEN_H

#include <random>

#include "gassim2d.h"
#include "gassim2d_util.h"

#include <cstdint>

#include "simblock.h"


class GlobalPopulator {
	public:
		GlobalPopulator(SimulationGrid *g, SimulationBlock *b);
		~GlobalPopulator();

	void set_seed(uint32_t new_seed){
		global_seed = new_seed;
		my_engine.seed(new_seed);
	}

	void populate_region_random_number_density(bounding_box_t region, double pitch, double number_density, double temp_Kelvin);
	//void populate_random_number_density(double spacing, double number_density, double temp_Kelvin);

	private:
		uint32_t global_seed;
		SimulationGrid *my_grid;
		SimulationBlock *my_block;

		std::mt19937_64 my_engine;
};





#endif /* DATAGEN_H */
