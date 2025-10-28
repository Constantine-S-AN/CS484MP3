#include <boost/program_options.hpp>

#include <random>
#include <chrono>

#include <mpi.h>

#include "gassim2d.h"
#include "gassim2d_util.h"

#include <string>
#include <sstream>

#include <iostream>

#include <stdio.h>

#include "simblock.h"
#include "datagen.h"
#include "solution.h"

#include "xyz_writer.h"

#define DEFAULT_GENERATOR_SEED 1337

#include "gas_constants.h"

int main(int argc, char **argv){
	//FILE *output = nullptr;
	XYZWriter *file_output = nullptr;

	int output_at_iteration = 1000;
	int N_steps = 10000;

	int particle_migration_frequency = 7;

	double spatial_extent = 48.0;

	double temp_kelvin = 295.0;//Roughly room temperature
	double lattice_pitch = 0.36; //selected for Argon specifically. Not currently a command-line option.

	double low_density = 0.3;
	double high_density = 0.85;

	int generator_seed = DEFAULT_GENERATOR_SEED;

	std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
	std::chrono::time_point<std::chrono::high_resolution_clock> end_time;

	#ifdef AMPI
	int ampi_migrate_frequency = 20;
	#endif

	int world_rank;
	int world_size;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	{//ENFORCE SQUARE WORLD
		int ranks_sqrt = std::sqrt(world_size);
		if(world_size != ranks_sqrt*ranks_sqrt){
			std::cerr << "Ranks must be a square number." << std::endl;
			assert(world_size == ranks_sqrt*ranks_sqrt);
		}
	}

	#ifdef AMPI
	AMPI_Set_migratable(0 != world_rank);
	#endif

	srand(time(0)+world_rank);

	//Boost command line parsing adapted from example at :
	// http://www.radmangames.com/programming/how-to-use-boost-program_options
	// Which is now unavailable. A copy is available at:
	// https://www.programmersought.com/article/71112056035/

	namespace po = boost::program_options;
	po::options_description desc("Options");
	desc.add_options()
	("help", "Print help messages")
	#ifdef AMPI
	("process-migrate", po::value<int>(), "How often to migrate AMPI virtual ranks to a different cpu/machine.")
	#endif
	("outfile,o",  po::value<std::string>(), "Save output to .xyz files with this prefix.")
	("outinterval,i", po::value<int>(), "Output every i iterations.")
	("exchange,e", po::value<int>(), "Exchange/migrate particles at this interval.")
	("steps,N", po::value<int>(), "Simulate a total of N steps.")
	("seed", po::value<int>(), "Seed for random data generator.")
	("low-density,l", po::value<double>(), "Low density for generation. Must be between 0.0 and 1.0.")
	("high-density,h", po::value<double>(), "High density for generation. Must be between 0.0 and 1.0.")
	("size,s", po::value<double>(), "Size of the square (or cube) to simulate on.")
	("temp,T", po::value<double>(), "Temperature, in Kelvin. (Ar melts at 83.81K and boils at 87.302K .)" );


	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);

	if ( vm.count("help")  ){
		std::cout << "Parallel, torroidal, 2D Lennard-Jones potential Argon simulator." << std::endl
				<< desc << std::endl; //Prints out the Boost-commandline generated description.
		MPI_Finalize();
		return 0;
	}

	if(vm.count("outfile")){
		std::string filename = std::string("") + vm["outfile"].as<std::string>()  + "_" + std::to_string(world_rank) + ".xyz";
		std::cerr << "Outputting to " << filename << std::endl;
		//output = fopen(filename.c_str(), "w");
		file_output = new CFILEXYZWriter();
		file_output->open(filename);
	}else{ file_output = nullptr; }

	if(vm.count("outinterval")){
		output_at_iteration = vm["outinterval"].as<int>();
		if(output_at_iteration < 1){
			std::cerr << "Invalid output frequency" << std::endl;
			exit(1);
		}
	}

	if(vm.count("exchange")){
		particle_migration_frequency = vm["exchange"].as<int>();
		if(particle_migration_frequency < 1){
			std::cerr << "Invalid exchange frequency." << std::endl;
			exit(1);
		}
	}

	if(vm.count("steps")){
		N_steps = vm["steps"].as<int>();
		if(N_steps == 0){
			std::cerr << "You requested 0 steps. Are you sure?" << std::endl;
		}
		if(N_steps < 1){
			std::cerr << "Invalid number of steps" << std::endl;
			exit(1);
		}
	}

	#ifdef AMPI
	if(vm.count("process-migrate")){
		ampi_migrate_frequency = vm["process-migrate"].as<int>();
	}
	#endif

	if(vm.count("seed")){
		generator_seed = vm["seed"].as<int>();
	}


	if(vm.count("low-density")){
		low_density = vm["low-density"].as<double>();
	}

	if(vm.count("high-density")){
		high_density = vm["high-density"].as<double>();
	}

	if(vm.count("size")){
		spatial_extent = vm["size"].as<double>();
	}

	if(vm.count("temp")){
		temp_kelvin = vm["temp"].as<double>();
		if(temp_kelvin < 0.0){
			assert(false);
		}
	}

	//Done with command-line arguments.

	if(0 == world_rank)
		std::cerr << "World size is : " << world_size << std::endl;

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
	SimulationBlock *my_sim_block = new MPISimulationBlock(the_sim, sim_world_grid, MPI_COMM_WORLD, world_rank);

	//defines the bounding box for what will be handled by my_sim_block
	my_sim_block->bounds = sim_world_grid.bb_for_cpu(world_rank);//You should probably also handle this in your constructor.

	my_sim_block->exchange_frequency = particle_migration_frequency;

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

	if(nullptr != file_output) file_output->write_xyz(my_sim_block->all_particles, my_sim_block->N_particles);

	MPI_Barrier(MPI_COMM_WORLD);//For timing
	start_time = std::chrono::high_resolution_clock::now();
	int i;
	for(i=0;i<N_steps;i++){
		//every k steps: migrate OOB particles to neighbors, receive OOB particles from neighbors
		//every step: send/receive ghosts to/from all neighbors

		my_sim_block->simulate_tick();

		#ifdef AMPI
		if(0 != i && 0 == i%ampi_migrate_frequency) {//we don't want to migrate on the first iteration.
			my_sim_block->finalize_communication();
			my_sim_block->N_ghosts = 0;
			my_sim_block->set_max_particles(my_sim_block->N_particles);//shrink storage
			my_sim_block->dealloc_ephemeral_storage();//deallocate temporary storage
			AMPI_Migrate(AMPI_INFO_LB_SYNC);
			my_sim_block->set_max_particles(10000);//reallocate storage
			my_sim_block->init_communication();
		}
		#endif

		if(0 == i % output_at_iteration){
			if(0 == world_rank){
				fprintf(stderr, "i=%d\n",i);
			}
			if(nullptr != file_output) file_output->write_xyz(my_sim_block->all_particles, my_sim_block->N_particles);
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);//for timing
	end_time = std::chrono::high_resolution_clock::now();

	if(0 != (N_steps-1) % output_at_iteration){
		//Output last frame, but only if it was not already output.
		if(0 == world_rank){
			fprintf(stderr, "i=%d\n",N_steps-1);
			fflush(stderr);
		}
		if(nullptr != file_output){ file_output->write_xyz(my_sim_block->all_particles, my_sim_block->N_particles); file_output->close(); }
	}


	if(0 == world_rank){
		std::chrono::duration<double> diff = end_time-start_time;
		std::cout << "Simulation_Duration " << diff.count() << " s" << std::endl << std::flush;
	}

	MPI_Barrier(MPI_COMM_WORLD);//for funzies (for the communication, just in case.)
	my_sim_block->finalize_communication();

	delete my_sim_block;

	MPI_Finalize();

	return 0;
}
