#include "datagen.h"

GlobalPopulator::GlobalPopulator(SimulationGrid *g, SimulationBlock *b) : my_grid(g), my_block(b) {
	my_engine = std::mt19937_64();
}
GlobalPopulator::~GlobalPopulator(){}

void GlobalPopulator::populate_region_random_number_density(bounding_box_t region, double pitch, double number_density, double temp_Kelvin){

	uint_fast64_t local_seed = my_engine();

	//find grid size and offset for the entire grid
	int full_N_x = (int)floor((region.max.x - region.min.x) / pitch);
	int full_N_y = (int)floor((region.max.y - region.min.y) / pitch);

	int my_begin_i = (int)ceil( (my_block->bounds.min.y - region.min.y) / pitch); if( my_begin_i < 0){ my_begin_i = 0;}
	int my_begin_j = (int)ceil( (my_block->bounds.min.x - region.min.x) / pitch); if( my_begin_j < 0){ my_begin_j = 0;}
	int my_end_i   = (int)floor( (my_block->bounds.max.y - region.min.y) / pitch); if( my_end_i > full_N_y){my_end_i = full_N_y;}
	int my_end_j   = (int)floor( (my_block->bounds.max.x - region.min.x) / pitch); if( my_end_j > full_N_x){my_end_j = full_N_x;}

	//We will reseed this engine for each point with a seed based on the grid position and the global RNG.
	std::mt19937_64 tmp_engine;

	const double tmp_engine_max = (double)tmp_engine.max();
	const double maxwell_boltzmann_stddev = pow(temp_Kelvin*KB_AGPERKELVIN_NM2PERS2*pow(my_block->my_sim.mass,-1.0),0.5);//TODO
	const uint_fast64_t rand_cutoff = (uint_fast64_t)(number_density*tmp_engine_max);

	for(int i = my_begin_i;i<my_end_i;i++ ){
		for(int j = my_begin_j;j<my_end_j;j++){
			tmp_engine.seed((local_seed << 30) ^ (i << 15) ^ j );

			if( tmp_engine() <= rand_cutoff ){
				//generate a particle at i,j

				//Single marsaglia_alg calculation, from Rosetta Code
				double x,y,rsq,f;
	            do {
	                x = 2.0 * tmp_engine() / tmp_engine_max - 1.0;
	                y = 2.0 * tmp_engine() / tmp_engine_max - 1.0;
	                rsq = x * x + y * y;
	            }while( rsq >= 1. || rsq == 0. );
	            f = sqrt( -2.0 * log(rsq) / rsq );
				x *= f;
				y *= f;
				//End of marsaglia algorithm


				phys_particle_t p;
				p.p = (phys_vector_t){ .x = region.min.x + pitch*j, .y = region.min.y + pitch*i };
				p.v = (phys_vector_t){ .x = x*maxwell_boltzmann_stddev, .y = y*maxwell_boltzmann_stddev };


				my_block->add_particle(p);
			}

		}
	}

}


//void populate_random_number_density(double spacing, double number_density, double temp_Kelvin);
