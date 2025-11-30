#ifndef P1_SOLLUTION_H
#define P1_SOLLUTION_H

#include "gassim2d.h"
#include "simblock.h"

#include <mpi.h>
#include <cstddef>
#include <vector>
//Include whatever you like.

//You may define your own helper classes if you wish.

class MPISimulationBlock : public SimulationBlock {
public:
	//Don't alter the signature of the constructor.
	MPISimulationBlock(gas_simulation _in_sim, SimulationGrid _grid, MPI_Comm _comm, int _rank);
	~MPISimulationBlock();//Make sure to clean up after yourself.

	/* Students overwrite and implement these. */
	virtual int init_communication();
	virtual int finalize_communication();
	virtual int exchange_particles();
	virtual int communicate_ghosts();

private://but students can access
	//SimulationGrid the_grid;
	MPI_Comm my_comm;
	int my_rank;

	//Vector of particle vectors for sending/receiving particles --> initially empty
	//Students do not need to use this if they wish to implement a different way
	std::vector< std::vector< phys_particle_t > > outgoing_buffers;

	/* Custom-made MPI datatypes for sending/receiving particles. */
	//Students do not need to use these if they wish to implement a different way
	MPI_Datatype phys_vector_mpidt;
	MPI_Datatype exchanged_particle_mpidt;
	MPI_Datatype ghost_particle_mpidt;

	/* Functions to create & destroy custome-made MPI particle datatypes. */
	void create_mpi_datatypes();
	void free_mpi_datatypes();

	/* Students overwrite and implement this. */
	inline void outgoing_wrap();

	//You may need to add your own variables.
	// Precomputed neighbor ranks for each direction we communicate with.
	// Indexed by DirectionIndex below.
	std::vector<int> neighbor_ranks;
	// Order of directions used for communication loops.
	enum DirectionIndex {
		N = 0,
		S,
		E,
		W,
		NE,
		NW,
		SE,
		SW,
		NUM_DIRS
	};
	// Convenience mapping of DirectionIndex -> sim_direction_t bitmask.
	static constexpr sim_direction_t dir_masks[NUM_DIRS] = {
		SimulationBlock::DIR_N,
		SimulationBlock::DIR_S,
		SimulationBlock::DIR_E,
		SimulationBlock::DIR_W,
		SimulationBlock::DIR_NE,
		SimulationBlock::DIR_NW,
		SimulationBlock::DIR_SE,
		SimulationBlock::DIR_SW
	};
	// Symmetric tag ids so opposite directions use the same MPI tag.
	static constexpr int dir_pair_tags[NUM_DIRS] = {
		0, // N
		0, // S
		1, // E
		1, // W
		2, // NE
		3, // NW
		2, // SE
		3  // SW
	};
};





#endif /* P1_SOLLUTION_H */
