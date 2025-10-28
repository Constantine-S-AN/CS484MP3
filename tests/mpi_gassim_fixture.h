#include <random>

#include <memory>

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

#include <gtest/gtest.h>
#include "gtest-mpi-listener.hpp"
#include <mpi.h>


#include <string>
#include <sstream>

#define DEFAULT_GENERATOR_SEED 1337

#include "gas_constants.h"

#define TESTING_TOLERANCE_GHOSTS 1E-12
#define TESTING_TOLERANCE 1E-8

const gas_simulation testing_simulation_for_unbounded_fixture = {	.mass = MASS_AR_AG,
							.epsilon = EPSILON_AR_AGNM2PERNS2,
							.sigma = SIGMA_AR_NM,
							.cutoff = 3.0,
							.v_max = 1000.0,
							.bounds = { .min = {.x = 0.0, .y = 0.0},
										.max = {.x = 20.0, .y = 20.0 }
									},
							.enforce_bounds = { .x = false, .y = false }
						};
/*
 * https://github.com/google/googletest/blob/master/googletest/docs/primer.md
 */


#define UNBOUNDED_TESTING_MINSIZE 4

class MpiGasSim_BaseFixture : public ::testing::Test {
protected:
	//int world_rank, world_size; //Not members.

	const int testing_group_size = UNBOUNDED_TESTING_MINSIZE;
	int in_testing_group = 0;
	MPI_Comm testing_comm;
	int testing_rank, testing_size;//testing_size should always equal testing_group_size, but use this.

	void SetUp() override {
		MPI_Barrier(MPI_COMM_WORLD); //Keeps all ranks synchronized for testing.
		//mysim = testing_simulation_for_unbounded_fixture;

		int world_rank, world_size;
		MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
		MPI_Comm_size(MPI_COMM_WORLD, &world_size);

		//TODO:: Use MPI process creation in the future.
		ASSERT_GE(world_size,testing_group_size) << "Can't run this test with fewer than " << testing_group_size << " ranks.";//Can't test

		in_testing_group = (world_rank < testing_group_size) ? 1 : 0;
		MPI_Comm_split(MPI_COMM_WORLD, in_testing_group, world_rank, &testing_comm);

		MPI_Comm_rank(testing_comm, &testing_rank);
		MPI_Comm_size(testing_comm, &testing_size);

		/* DONE WITH COMM SETUP */
		//if(world_rank == 0){ std::cerr << "SETUP CALLED" << std::endl;}
		//if(!in_testing_group){ GTEST_SUCCESS_("skip on inactive ranks.");}
	}

	void TearDown() override {
		MPI_Comm_free(&testing_comm);
		MPI_Barrier(MPI_COMM_WORLD);//Keeps all ranks synchronized for testing.
	}

	virtual std::shared_ptr<SimulationBlock> create_sim_block() = 0;

	bool exchange_test_helper(int particle_origin, int particle_destination, phys_vector_t start, phys_vector_t end){
		if(!in_testing_group){ return true; }

		//CREATE THE WORLD.
		std::shared_ptr<SimulationBlock> my_sim_block = create_sim_block();
		my_sim_block->init_communication();


		MPI_Barrier(testing_comm);
		//clear all particles
		while(my_sim_block->N_particles > 0){ my_sim_block->remove_particle(0); }

		if(testing_rank == particle_origin){
			phys_particle_t foo;
			foo.v.x = 0.0;
			foo.v.y = 0.0;
			foo.p = start;
			my_sim_block->add_particle(foo);
		}
		my_sim_block->exchange_particles();

		EXPECT_EQ(
				(testing_rank == particle_destination) ? 1 : 0,
				my_sim_block->N_particles
				);

		if(testing_rank == particle_destination ){
			phys_vector_t expected_particle = end;
			phys_particle_t communicated_particle = my_sim_block->all_particles[0];
			EXPECT_NEAR(expected_particle.x, communicated_particle.p.x,TESTING_TOLERANCE_GHOSTS);
			EXPECT_NEAR(expected_particle.y, communicated_particle.p.y,TESTING_TOLERANCE_GHOSTS);
		}


		MPI_Barrier(testing_comm);
		my_sim_block->finalize_communication();
		my_sim_block.reset();

		return true;
	}
};

class MpiGasSimPeriodic : public MpiGasSim_BaseFixture {
protected:

	virtual std::shared_ptr<SimulationBlock> create_sim_block() override {
		if(!in_testing_group){ return NULL; }
		//CREATE THE WORLD.
		gas_simulation my_sim = testing_simulation_for_unbounded_fixture;

		SimulationGrid sim_world_grid(my_sim.bounds,testing_size);
		std::shared_ptr<SimulationBlock> my_sim_block = std::make_shared<MPISimulationBlock>(my_sim, sim_world_grid, testing_comm, testing_rank);
		return my_sim_block;
	}

};

class MpiGasSimBounded : public MpiGasSim_BaseFixture {
protected:
	virtual std::shared_ptr<SimulationBlock> create_sim_block() override{
		if(!in_testing_group){ return NULL; }
		//CREATE THE WORLD.
		gas_simulation bounded_sim = testing_simulation_for_unbounded_fixture;
		bounded_sim.enforce_bounds.x = bounded_sim.enforce_bounds.y = true;

		SimulationGrid sim_world_grid(bounded_sim.bounds,testing_size);
		std::shared_ptr<SimulationBlock> my_sim_block = std::make_shared<MPISimulationBlock>(bounded_sim, sim_world_grid, testing_comm, testing_rank);
		return my_sim_block;
	}

	bool boundary_test_helper(int particle_origin, const phys_vector_t start){
		return exchange_test_helper(particle_origin, particle_origin, start, start);
	}
};
