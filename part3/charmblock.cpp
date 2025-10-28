#include "charmblock.decl.h"
#include "charmblock.h"
#include "simmain.decl.h"

#include "simblock.h"
#include "datagen.h"
#include "xyz_writer.h"
#include "gas_constants.h"

#include "gassim2d_puppers.hpp"

#include <cassert>

inline void operator|(PUP::er &p, SimulationGrid &a_grid){
	p|a_grid.bounds; //Could differ from that of the simulation, for example, when wrapping.
	p|a_grid.wrap_x;
	p|a_grid.wrap_y;
	p|a_grid.sizes;
	p|a_grid.n_cpus;
	p|a_grid.n_cols;//row size
	p|a_grid.n_rows;//col size
}

inline void operator|(PUP::er &p, XYZWriter &w){
	if(p.isUnpacking()){
		std::string foo;
		p|foo;
		w.open(foo,"a");
	}else{
		std::string foo;
		foo = w.get_filename();
		p|foo;
	}
}

CharmSimBlock::CharmSimBlock(gas_simulation _in_sim, SimulationGrid _grid, int which_block) : SimulationBlock(_in_sim, _grid, which_block, 10000) {

}

void CharmSimBlock::pup(PUP::er &p){
	p|my_sim;
	p|the_grid;
	p|block_rank;
	if(p.isUnpacking()){ initialize_bounds_and_edges(); }
	p|bounds;

	if(p.isUnpacking()){
		int read_max_size;
		p|read_max_size;
		this->set_max_particles(read_max_size);
	}else{
		int write_max_size = this->get_max_particles();
		p|write_max_size;
	}

	p|N_particles;
	p|N_ghosts;

	//TODO:: Handle the particles, forces, and ghosts.
	//throw std::runtime_error("Pupper not fully implemented");
	PUParray(p,all_particles, N_particles);
	PUParray(p,forces, N_particles);
	PUParray(p,all_ghosts, N_ghosts);

	p|elapsed_ticks;
	p|T_elapsed_ns;
	p|tick_delta_t_ns;
	p|exchange_frequency;

}


extern /* readonly */ CProxy_SimMain mainProxy;

CharmBlock::CharmBlock(
	int xsize, int ysize,
	int N_steps,
	std::string file_output_basename, int _output_at_iteration,
	int particle_migration_frequency,
	double spatial_extent,
	double temp_kelvin,
	double lattice_pitch, double high_density, double low_density,
	int generator_seed,
	int _process_migration_frequency
) : n_received(0), n_neighbors(0), current_iteration(0),
N_steps(N_steps), output_at_iteration(_output_at_iteration),
process_migration_frequency(_process_migration_frequency) {


	int world_size = xsize*ysize;
	int world_rank = thisIndex.y*ysize + thisIndex.x;

	if(! file_output_basename.empty()){
		#ifdef DEBUG
		CkPrintf("Creating file output thingy\n");
		#endif
		file_output = new CFILEXYZWriter();
		file_output->open(file_output_basename + "_" + std::to_string(world_rank) + ".xyz");
	}

	//CREATE THE WORLD.
	gas_simulation the_sim = {	.mass = MASS_AR_AG,
								.epsilon = EPSILON_AR_AGNM2PERNS2,
								.sigma = SIGMA_AR_NM,
								.cutoff = 3.0,
								.v_max = 1000.0,
								.bounds = { .min = {.x = 0.0, .y = 0.0},
											.max = {.x = spatial_extent, .y = spatial_extent }
										},
								.enforce_bounds = { .x = true, .y = true }
							};
	SimulationGrid sim_world_grid(the_sim.bounds,world_size);
	my_sim_block = new CharmSimBlock(the_sim, sim_world_grid, world_rank);

	//defines the bounding box for what will be handled by my_sim_block
	my_sim_block->bounds = sim_world_grid.bb_for_cpu(world_rank);//You should probably also handle this in your constructor.

	my_sim_block->exchange_frequency = particle_migration_frequency;

	{//Figure out the number of neighbors
		int gotten_edge_mask = my_sim_block->get_edge_mask();
		int calc_n_neighbors = 0;
		if(SimulationBlock::DIR_N ==(SimulationBlock::DIR_N  & gotten_edge_mask)){ ++calc_n_neighbors; }
		if(SimulationBlock::DIR_S ==(SimulationBlock::DIR_S  & gotten_edge_mask)){ ++calc_n_neighbors; }
		if(SimulationBlock::DIR_E ==(SimulationBlock::DIR_E  & gotten_edge_mask)){ ++calc_n_neighbors; }
		if(SimulationBlock::DIR_W ==(SimulationBlock::DIR_W  & gotten_edge_mask)){ ++calc_n_neighbors; }
		if(SimulationBlock::DIR_NE==(SimulationBlock::DIR_NE & gotten_edge_mask)){ ++calc_n_neighbors; }
		if(SimulationBlock::DIR_NW==(SimulationBlock::DIR_NW & gotten_edge_mask)){ ++calc_n_neighbors; }
		if(SimulationBlock::DIR_SE==(SimulationBlock::DIR_SE & gotten_edge_mask)){ ++calc_n_neighbors; }
		if(SimulationBlock::DIR_SW==(SimulationBlock::DIR_SW & gotten_edge_mask)){ ++calc_n_neighbors; }
		n_neighbors = calc_n_neighbors;
		#ifdef DEBUG
		CkPrintf("(%i,%i) init edgemask %i , neigh %i\n",thisIndex.x, thisIndex.y, gotten_edge_mask , n_neighbors);
		#endif
	}


	GlobalPopulator my_populator(&sim_world_grid, my_sim_block);
	my_populator.set_seed(generator_seed);

	{//Populate unevenly.
	bounding_box_t datagen_bounds = sim_world_grid.bounds;
	datagen_bounds.min.x += lattice_pitch/2.0;
	datagen_bounds.min.y += lattice_pitch/2.0;
	datagen_bounds.max.x -= lattice_pitch/2.0;
	datagen_bounds.max.y -= lattice_pitch/2.0;

	bounding_box_t dense_region_bounds = datagen_bounds;
	dense_region_bounds.max.x = (dense_region_bounds.max.x - lattice_pitch) / 2.0;
	dense_region_bounds.max.y = (dense_region_bounds.max.y - lattice_pitch) / 2.0;

	bounding_box_t ld_region_1_bounds = datagen_bounds;
	ld_region_1_bounds.min.x = datagen_bounds.max.x / 2.0;
	ld_region_1_bounds.max.y = datagen_bounds.max.y / 2.0;

	bounding_box_t ld_region_2_bounds = datagen_bounds;
	ld_region_2_bounds.min.y = datagen_bounds.max.y / 2.0;

	my_populator.populate_region_random_number_density(dense_region_bounds, lattice_pitch, high_density, temp_kelvin);
	my_populator.populate_region_random_number_density(ld_region_1_bounds, lattice_pitch, low_density, temp_kelvin);
	my_populator.populate_region_random_number_density(ld_region_2_bounds, lattice_pitch, low_density, temp_kelvin);
	}


	my_sim_block->init_communication();

	if(nullptr != file_output){ file_output->write_xyz(my_sim_block->all_particles, my_sim_block->N_particles); }

	#ifdef DEBUG
	CkPrintf("(%i, %i) Init finished. n_neighbors(%i).\n",thisIndex.x, thisIndex.y,n_neighbors );
	#endif

	usesAtSync = true;
}

CharmBlock::CharmBlock(CkMigrateMessage *msg){
	file_output = nullptr;
	my_sim_block = new CharmSimBlock();
}

void CharmBlock::pup(PUP::er &p){
	p|output_at_iteration;
	p|current_iteration;
	p|N_steps;
	p|n_neighbors;
	p|n_received;

	p|process_migration_frequency;

	p|(*my_sim_block);

	//file writer
	if(p.isUnpacking()){
		bool has_file_writer;
		p|has_file_writer;
		if(has_file_writer){
			file_output = new CFILEXYZWriter();
			p|(*file_output);
		}
	}else{
		bool has_file_writer = (nullptr != file_output);
		p|has_file_writer;
		if(has_file_writer){
			p|(*file_output);
		}

	}
}

void CharmBlock::hello(){
	#ifdef DEBUG
	CkPrintf("Hello from %i %i \n",thisIndex.x, thisIndex.y);
	#endif
	//file_output->write_xyz(my_sim_block->all_particles,my_sim_block->N_particles);
	mainProxy.done();
}

void CharmBlock::handle_phase(){
	#ifdef DEBUG
	CkPrintf("(%i,%i) Handling tick %i. I have %i particles, and %i ghosts \n",
				thisIndex.x, thisIndex.y,
				current_iteration,my_sim_block->N_particles,my_sim_block->N_ghosts
			);
	#endif
	my_sim_block->simulate_tick();

	/* Periodic file output. */
	if( (0 == current_iteration % output_at_iteration) || (current_iteration == (N_steps-1) )){
		if(0 == my_sim_block->block_rank){
			CkError("i=%d\n",current_iteration);/* Terminal output of iteration.*/
		}
		if(nullptr != file_output){ file_output->write_xyz(my_sim_block->all_particles, my_sim_block->N_particles); }
	}
};

void CharmBlock::send_migrants(){
	//Remember, as in part1, to process particles from last to first when you may delete them.

}

void CharmBlock::send_ghosts(){
}

void CharmBlock::receive_migrants_impl(int phase, int from, int direction, std::vector<phys_particle_t> migrants){
	#ifdef DEBUG
	CkPrintf("(%i, %i) Receive_migrants bottom end.\n",thisIndex.x,thisIndex.y);
	#endif

	for(size_t i = 0;i<migrants.size();++i){
		my_sim_block->add_particle(migrants.at(i));//TODO: use iterator.
	}
}

void CharmBlock::receive_ghosts_impl(int phase, int from, int direction, std::vector<phys_particle_t> ghosts){

	/*
	This needs to fill my_sim_block->all_ghosts array with received ghosts.
	It should start at my_sim_block->N_ghosts.
	And needs to increment my_sim_block->N_ghosts by ghosts.size()

	*/
	#ifdef DEBUG
	CkPrintf("(%i, %i) Receive bottom end.\n",thisIndex.x,thisIndex.y);
	#endif
	
	size_t number_of_ghosts = my_sim_block->N_ghosts;//puts in a stack variable, but needs to be written back.
	phys_particle_t *ghosts_array = my_sim_block->all_ghosts;

	for(size_t i = 0;i<ghosts.size();++i){
		ghosts_array[number_of_ghosts++] = ghosts.at(i);//TODO: use iterator.
	}
	my_sim_block->N_ghosts = number_of_ghosts;
}

#include "charmblock.def.h"
