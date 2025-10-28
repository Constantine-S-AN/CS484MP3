#include <boost/program_options.hpp>

#include <random>
#include <cmath>

#include "gassim2d.h"
#include "gassim2d_util.h"

#include <string>
#include <sstream>
#include <iostream>
#include <cstdio>


#include "simblock.h"
#include "datagen.h"

//#include "xyz_writer.h"

#define DEFAULT_GENERATOR_SEED 1337

#include "gas_constants.h"

#include "simmain.decl.h"
#include "simmain.h"

/* readonly */ CProxy_SimMain mainProxy;


// Entry point of Charm++ application
SimMain::SimMain(CkArgMsg* msg) {
	//FILE *output = nullptr;
	//XYZWriter *file_output = nullptr;
	std::string file_output_basename;

	int output_at_iteration = 1000;
	int N_steps = 10000;

	int particle_migration_frequency = 7;

	double spatial_extent = 48.0;

	double temp_kelvin = 295.0;//Roughly room temperature
	double lattice_pitch = 0.36; //selected for Argon specifically. Not currently a command-line option.

	double low_density = 0.3;
	double high_density = 0.85;

	int generator_seed = DEFAULT_GENERATOR_SEED;

	int ampi_migrate_frequency = 20;

	int number_of_ranks = 256;

	//Boost command line parsing adapted from example at :
	// http://www.radmangames.com/programming/how-to-use-boost-program_options

	namespace po = boost::program_options;
	po::options_description desc("Options");
	desc.add_options()
	("help", "Print help messages")
	("process-migrate", po::value<int>(), "How often to migrate AMPI virtual ranks to a different cpu/machine.")
	("ranks,R", po::value<int>(), "Number of chares to start.")
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
	po::store(po::parse_command_line(msg->argc, msg->argv, desc), vm);

	if ( vm.count("help")  ){
		std::cout << "Parallel, torroidal, 2D Lennard-Jones potential Argon simulator." << std::endl
				<< desc << std::endl;
		CkExit(0);
	}

	if(vm.count("outfile")){
		file_output_basename = vm["outfile"].as<std::string>();
		//std::cerr << "Outputting to " << file_output_basename << std::endl;
	}else{ file_output_basename = std::string(); }

	if(vm.count("outinterval")){
		output_at_iteration = vm["outinterval"].as<int>();
		if(output_at_iteration < 1){
			std::cerr << "Invalid output frequency" << std::endl;
			CkExit(1);
		}
	}

	if(vm.count("exchange")){
		particle_migration_frequency = vm["exchange"].as<int>();
		if(particle_migration_frequency < 1){
			std::cerr << "Invalid exchange frequency." << std::endl;
			CkExit(1);
		}
	}

	if(vm.count("steps")){
		N_steps = vm["steps"].as<int>();
		if(N_steps == 0){
			std::cerr << "You requested 0 steps. Are you sure?" << std::endl;
		}
		if(N_steps < 1){
			std::cerr << "Invalid number of steps" << std::endl;
			CkExit(1);
		}
	}

	if(vm.count("process-migrate")){
		ampi_migrate_frequency = vm["process-migrate"].as<int>();
		if(0 >= ampi_migrate_frequency){
			CkError("Process migration must have a period greater than 0!\n");
			CkAssert(false);
		}
	}

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
			CkError("A temperature of less than 0.0 Kelvin is ridiculous.\n");
			CkAssert(false);
		}
	}

	if(vm.count("ranks")){
		number_of_ranks = vm["ranks"].as<int>();
		if(number_of_ranks <= 0){
			CkError("Number of ranks below zero.\n");
			CkAssert(false);
		}

		int ranks_sqrt = std::sqrt(number_of_ranks);
		if(number_of_ranks != ranks_sqrt*ranks_sqrt){
			CkError("Ranks should be a square number.\n");
			CkAssert(false);
		}
	}

	//Done with command-line arguments.

	//if(0 == world_rank)
	//	std::cerr << "World size is : " << world_size << std::endl;



  // We are done with msg so delete it.
  delete msg;

  // Set the mainProxy readonly to point to a
  //   proxy for the Main chare object (this
  //   chare object).
  mainProxy = thisProxy;

	numElements = number_of_ranks;

	int x_number = std::sqrt(number_of_ranks);
	int y_number = std::sqrt(number_of_ranks);

	CProxy_CharmBlock blockArray = CProxy_CharmBlock::ckNew( x_number, y_number,
	N_steps,
	file_output_basename, output_at_iteration,
	particle_migration_frequency,
	spatial_extent,
	temp_kelvin,
	lattice_pitch,
	high_density, low_density, generator_seed,
	ampi_migrate_frequency,
	x_number, y_number);


	start_time = std::chrono::high_resolution_clock::now();
	blockArray.run_until_done();

}


// Constructor needed for chare object migration (ignore for now)
// NOTE: This constructor does not need to appear in the ".ci" file
SimMain::SimMain(CkMigrateMessage* msg) { }


// When called, the "done()" entry method will cause the program
//   to exit.
void SimMain::done() {

  // Increment the doneCount.  If all of the Hello chare
  //   objects have indicated that they are done, then exit.
  //   Otherwise, continue waiting for the Hello chare objects.
  doneCount++;
  if (doneCount >= numElements){
	end_time = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff = end_time-start_time;
	std::cout << "Simulation_Duration " << diff.count() << " s" << std::endl << std::flush;
    CkExit();
	}
}


#include "simmain.def.h"
