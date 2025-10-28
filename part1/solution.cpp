#include "solution.h"

#include <cassert>
#include <stdexcept>
#include <cstdlib>
#include <iostream>

#include <mpi.h>


inline void MPISimulationBlock::outgoing_wrap(){
	//If I am on one of the edges, I need to post-process my outgoing buffers.

}


//Do not alter the initializer line, nor the signature of the constructor.
MPISimulationBlock::MPISimulationBlock(gas_simulation _in_sim, SimulationGrid _grid, MPI_Comm _comm, int _rank) : SimulationBlock(_in_sim, _grid, _rank, 10000), my_comm(_comm), my_rank(_rank){
	/* You may add code at the bottom of this constructor */
}

MPISimulationBlock::~MPISimulationBlock(){
}

/* Students overwrite and implement these. */
int MPISimulationBlock::init_communication(){
	return 0;
}
int MPISimulationBlock::finalize_communication(){
	return 0;
}
int MPISimulationBlock::exchange_particles(){

	//NOTE: because "remove_particle" creates a hole and
	//fills it with the last particle in the array, you MUST iterate backwards.
	//(In fact, you will find in general in programming that deletion should
	// usually be done with backwards iterators.
	// I have found bugs in widely-distributed, production NASA software
	// that arise from deleting on a forward iterator.)
	//     Loop signature should be similar to:
	//
	//    for(int i=N_particles-1;i>=0;--i){
	//		FIGURE OUT PARTICLE MIGRATION DIRECTION
	//	}
	//
	//	MIGRATE ALL PARTICLES


	return 0;
}
int MPISimulationBlock::communicate_ghosts(){
	/*
	* Our superclass (defined in "simblock.h")
	* has (and therefore we have) the member variables:
	*
	* `unsigned int N_ghosts`
	* and
	* `phys_particle_t *all_ghosts`
	*
	* In this function, you should find all the particles that need to be
	* communicated as ghosts, and communicate them (or at least their position)
	* to adjacent blocks.
	*
	* The particles that you recieve from adjacent blocks need to be added
	* to `this->all_ghosts` and `this->N_ghosts` needs to be changed to equal
	* the total number of ghosts received in this call.
	*
	* Ghosts are reset every time this is called, so you should start populating
	* `this->all_ghosts` from the 0th position.
	*
	* If this block is on a Northern, Southern, Eastern, or Western edge,
	* make sure to send to the appropriate wrapped block. In this case,
	* You will also need to adjust the X or Y coordinate
	* (`p.x`, or `p.y` members of the `phys_particle_t` type.)
	* accordingly for wraparound.
	*
	*/

	this->N_ghosts = 0;//Clear all the ghosts we have, we get entirely new ghosts.
	//TODO: Add your solution after this line.

	return 0;
}


void MPISimulationBlock::create_mpi_datatypes(){
	/* TODO: Refactor to work with MPI 2.0 .
			This code will stop working with MPI 3.0 .
			https://www.open-mpi.org/faq/?category=mpi-removed#mpi-1-mpi-lb-ub
	*/
	
	/*
		We are creating existing members:
			phys_vector_mpidt
			exchanged_particle_mpidt
			ghost_particle_mpidt
			
	*/
	
	{/* Creation of phys_vector_mpidt ; Type for physics vector. Automatically works with 2d or 3d code. */
		MPI_Type_contiguous(VECTOR_DIMENSIONALITY,MPI_DOUBLE,&phys_vector_mpidt);
		MPI_Type_commit(&phys_vector_mpidt);
	}
	
	
	phys_particle_t tmp_part; //can't be const while also automatically having VECTOR_DIMENSIONALITY
	const int num_members = 2; //Position and velocity
	MPI_Datatype types[num_members] = {phys_vector_mpidt,phys_vector_mpidt};
	MPI_Aint displacements[num_members];
	
	MPI_Aint base_address;
	MPI_Get_address(&tmp_part, &base_address);
	MPI_Get_address(&tmp_part.p, &displacements[0]);
	MPI_Get_address(&tmp_part.v, &displacements[1]);
	displacements[0] = MPI_Aint_diff(displacements[0], base_address);
	displacements[1] = MPI_Aint_diff(displacements[1], base_address);
	
	{//Creation of exchanged_particle_mpidt
		int blocklens[num_members] = {1,1};
		MPI_Type_create_struct(num_members,blocklens,displacements,types,&exchanged_particle_mpidt);
		MPI_Type_commit(&exchanged_particle_mpidt);
	}
	
	{//Creation of ghost_particle_mpidt
		MPI_Datatype tmp_ghost_particle_mpidt;
		int blocklens[num_members] = {1,0};
		MPI_Type_create_struct(num_members,blocklens,displacements,types,&tmp_ghost_particle_mpidt);
		
		MPI_Type_create_resized(tmp_ghost_particle_mpidt,
					static_cast<MPI_Aint>(0),
					static_cast<MPI_Aint>(sizeof(tmp_part)),
					&ghost_particle_mpidt
				);
		
		MPI_Type_commit(&ghost_particle_mpidt);
	}

}
void MPISimulationBlock::free_mpi_datatypes(){
	MPI_Type_free( &exchanged_particle_mpidt );
	MPI_Type_free( &ghost_particle_mpidt );
	MPI_Type_free( &phys_vector_mpidt );
}
