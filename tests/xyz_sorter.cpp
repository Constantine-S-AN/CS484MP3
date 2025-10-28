#include <boost/program_options.hpp>

#include <random>

#include <vector>


#include <string>
#include <sstream>

#include <iostream>
#include <fstream>
#include <sstream>

#include <stdio.h>

#include "xyz_writer.h"

using std::vector;

bool particle_sort_cmp(phys_particle_t a, phys_particle_t b){
	if(a.p.x < b.p.x) return true;
	if(a.p.x > b.p.x) return false;
	if(a.p.y < b.p.y) return true;
	if(a.p.y > b.p.y) return false;
	if(a.v.x < b.v.x) return true;
	if(a.v.x > b.v.x) return false;
	//if(a.v.y < b.v.y) return true;
	//if(a.v.y > b.v.y) return false;
	return (a.v.y < b.v.y);
}

int main(int argc, char **argv){
	//FILE *output = nullptr;
	XYZWriter *file_output = nullptr;
	std::string input_filename;
	//vector< vector< phys_particle_t > > frames();

	bool randomize = false;

	//Boost command line parsing adapted from example at :
	// http://www.radmangames.com/programming/how-to-use-boost-program_options

	namespace po = boost::program_options;
	po::options_description desc("Options");
	desc.add_options()
	("help", "Print help messages")
	("randomize,r", "Randomize, don't sort. (For testing.)")
	("infile,i",  po::value<std::string>(), "Read input from .xyz files with this prefix.")
	("outfile,o",  po::value<std::string>(), "Save output to .xyz files with this prefix.")
	;


	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);

	if ( vm.count("help")  ){
		std::cout << "Parallel, torroidal, 2D Lennard-Jones potential Argon simulator." << std::endl
				<< desc << std::endl;
		return 1;
	}
	if (vm.count("randomize")){
		randomize = true;
	}

	if(vm.count("infile")){
		input_filename = vm["infile"].as<std::string>();
	}else{
		std::cerr << "You must specify an input file" << std::endl;
		return 1;
	}

	if(vm.count("outfile")){

		std::string filename = vm["outfile"].as<std::string>();
		if(filename == input_filename){
			std::cerr << "Can't currently output to same filename as input, sorry." << std::endl;
			return 1;
		}
		std::cerr << "Outputting to " << filename << std::endl;
		//output = fopen(filename.c_str(), "w");
		file_output = new CFILEXYZWriter();
		file_output->open(filename);
	}else{
		std::cerr << "Must specify an output file" << std::endl;
		return 1;
	}

	//Done with command-line arguments.
	std::ifstream infile(input_filename,std::ios::in);

	while(infile.good() && !infile.eof()){
		vector< phys_particle_t > one_frame_vector;
		int num_particles = 0;

		std::stringstream ssf;
		std::string num_particle_line;
		std::getline(infile, num_particle_line);
		ssf.str(num_particle_line);
		ssf >> num_particles;



		std::string commentline;
		std::getline(infile, commentline);
		if(!commentline.empty()){
			return 1;
		}

		if(infile.eof()){ break; }//Why is this necessary?

		one_frame_vector.clear();
		one_frame_vector.reserve(num_particles);
		for(int i = 0;i<num_particles;++i){
			std::string particle_type;
			phys_particle_t one_particle;
			one_particle.v.x = 0.0;
			one_particle.v.y = 0.0;

			std::stringstream ss;
			std::string one_line;
			std::getline(infile, one_line);
			ss.str(one_line);

			ss >> particle_type;
			ss >> one_particle.p.x;
			ss >> one_particle.p.y;// >> std::endl;

			one_frame_vector.push_back(one_particle);
		}

		if(randomize){
			std::random_shuffle(one_frame_vector.begin(), one_frame_vector.end());
		}else{
			std::sort(one_frame_vector.begin(), one_frame_vector.end(), particle_sort_cmp);
		}
		file_output->write_xyz(one_frame_vector.data(),one_frame_vector.size());
	}


	return 0;
}
