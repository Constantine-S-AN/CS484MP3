#include "solution.h"

#include <cassert>
#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <array>
#include <algorithm>

#include <mpi.h>


inline void MPISimulationBlock::outgoing_wrap(){
	// If I am on one of the edges and wraparound is permitted, adjust positions
	// of buffered migrants so that they re-enter on the opposite side.
	const double world_w = the_grid.sizes.x;
	const double world_h = the_grid.sizes.y;

	for(size_t idx = 0; idx < outgoing_buffers.size() && idx < DirectionIndex::NUM_DIRS; ++idx){
		sim_direction_t dir = dir_masks[idx];
		for(auto &p : outgoing_buffers[idx]){
			if(DIR_HAS(SimulationBlock::DIR_N, dir)){ p.p.y += world_h; }
			if(DIR_HAS(SimulationBlock::DIR_S, dir)){ p.p.y -= world_h; }
			if(DIR_HAS(SimulationBlock::DIR_W, dir)){ p.p.x += world_w; }
			if(DIR_HAS(SimulationBlock::DIR_E, dir)){ p.p.x -= world_w; }
		}
	}
}


//Do not alter the initializer line, nor the signature of the constructor.
MPISimulationBlock::MPISimulationBlock(gas_simulation _in_sim, SimulationGrid _grid, MPI_Comm _comm, int _rank) : SimulationBlock(_in_sim, _grid, _rank, 10000), my_comm(_comm), my_rank(_rank){
	/* You may add code at the bottom of this constructor */
	neighbor_ranks.resize(DirectionIndex::NUM_DIRS, MPI_PROC_NULL);
}

MPISimulationBlock::~MPISimulationBlock(){
}

/* Students overwrite and implement these. */
int MPISimulationBlock::init_communication(){
	create_mpi_datatypes();

	// Precompute neighbor ranks for all directions using grid position and wrapping.
	SimulationGrid::grid_position pos = the_grid.get_grid_position(my_rank);
	const std::array<int, DirectionIndex::NUM_DIRS> di = { -1, 1, 0, 0, -1, -1, 1, 1 };
	const std::array<int, DirectionIndex::NUM_DIRS> dj = { 0, 0, 1, -1, 1, -1, 1, -1 };

	for(int idx = 0; idx < DirectionIndex::NUM_DIRS; ++idx){
		int ni = pos.i + di[idx];
		int nj = pos.j + dj[idx];
		neighbor_ranks[idx] = the_grid.cpu_for_position(ni, nj);
	}

	return 0;
}
int MPISimulationBlock::finalize_communication(){
	free_mpi_datatypes();
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


	// Clear outgoing buffers for all directions.
	if(outgoing_buffers.size() != DirectionIndex::NUM_DIRS){
		outgoing_buffers.assign(DirectionIndex::NUM_DIRS, std::vector<phys_particle_t>());
	}else{
		for(auto &buf : outgoing_buffers){ buf.clear(); }
	}

	// First pass: classify and remove migrants.
	for(int i = static_cast<int>(N_particles) - 1; i >= 0; --i){
		phys_particle_t p = all_particles[i];
		sim_direction_t dir = check_migrant_direction(p);
		if(DIR_EQ(dir, SimulationBlock::DIR_SELF)){ continue; }

		// Map direction to index.
		int idx = -1;
		for(int k = 0; k < DirectionIndex::NUM_DIRS; ++k){
			if(DIR_EQ(dir, dir_masks[k])){ idx = k; break; }
		}
		if(idx < 0){ throw std::runtime_error("Unknown migration direction."); }
		outgoing_buffers[idx].push_back(p);

		remove_particle(static_cast<unsigned int>(i));
	}

	// Adjust coordinates for wraparound on edge-crossing particles.
	outgoing_wrap();

	// Two-phase exchange: counts then payloads.
	std::array<int, DirectionIndex::NUM_DIRS> send_counts{};
	std::array<int, DirectionIndex::NUM_DIRS> recv_counts{};
	for(int k = 0; k < DirectionIndex::NUM_DIRS; ++k){
		send_counts[k] = static_cast<int>(outgoing_buffers[k].size());
		int neighbor = neighbor_ranks[k];
		MPI_Sendrecv(&send_counts[k], 1, MPI_INT, neighbor, 100 + k,
					 &recv_counts[k], 1, MPI_INT, neighbor, 100 + k,
					 my_comm, MPI_STATUS_IGNORE);
	}

	// Post receives and sends for particle payloads.
	std::vector<MPI_Request> requests;
	requests.reserve(DirectionIndex::NUM_DIRS * 2);

	std::array<std::vector<phys_particle_t>, DirectionIndex::NUM_DIRS> incoming_buffers;
	for(int k = 0; k < DirectionIndex::NUM_DIRS; ++k){
		int neighbor = neighbor_ranks[k];
		int rcount = recv_counts[k];
		if(rcount > 0){
			incoming_buffers[k].resize(rcount);
			MPI_Request req;
			MPI_Irecv(incoming_buffers[k].data(), rcount, exchanged_particle_mpidt,
					  neighbor, 200 + k, my_comm, &req);
			requests.push_back(req);
		}
		int scount = send_counts[k];
		if(scount > 0){
			MPI_Request req;
			MPI_Isend(outgoing_buffers[k].data(), scount, exchanged_particle_mpidt,
					  neighbor, 200 + k, my_comm, &req);
			requests.push_back(req);
		}
	}

	if(!requests.empty()){
		MPI_Waitall(static_cast<int>(requests.size()), requests.data(), MPI_STATUSES_IGNORE);
	}

	// Append received particles to local storage.
	for(int k = 0; k < DirectionIndex::NUM_DIRS; ++k){
		for(const auto &p : incoming_buffers[k]){
			add_particle(p);
		}
	}

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

	// Prepare buffers per direction.
	std::array<std::vector<phys_particle_t>, DirectionIndex::NUM_DIRS> send_bufs;
	for(auto &buf : send_bufs){ buf.clear(); }

	// Helper to push particle into correct ghost buffers.
	for(unsigned int i = 0; i < N_particles; ++i){
		const phys_particle_t &p = all_particles[i];
		sim_direction_t dir = check_ghost_direction(p);
		if(DIR_EQ(dir, SimulationBlock::DIR_SELF)){ continue; }

		bool n = DIR_HAS(SimulationBlock::DIR_N, dir);
		bool s = DIR_HAS(SimulationBlock::DIR_S, dir);
		bool e = DIR_HAS(SimulationBlock::DIR_E, dir);
		bool w = DIR_HAS(SimulationBlock::DIR_W, dir);

		if(n){ send_bufs[DirectionIndex::N].push_back(p); }
		if(s){ send_bufs[DirectionIndex::S].push_back(p); }
		if(e){ send_bufs[DirectionIndex::E].push_back(p); }
		if(w){ send_bufs[DirectionIndex::W].push_back(p); }
		if(n && e){ send_bufs[DirectionIndex::NE].push_back(p); }
		if(n && w){ send_bufs[DirectionIndex::NW].push_back(p); }
		if(s && e){ send_bufs[DirectionIndex::SE].push_back(p); }
		if(s && w){ send_bufs[DirectionIndex::SW].push_back(p); }
	}

	// Exchange counts.
	std::array<int, DirectionIndex::NUM_DIRS> send_counts{};
	std::array<int, DirectionIndex::NUM_DIRS> recv_counts{};
	for(int k = 0; k < DirectionIndex::NUM_DIRS; ++k){
		send_counts[k] = static_cast<int>(send_bufs[k].size());
		int neighbor = neighbor_ranks[k];
		MPI_Sendrecv(&send_counts[k], 1, MPI_INT, neighbor, 300 + k,
					 &recv_counts[k], 1, MPI_INT, neighbor, 300 + k,
					 my_comm, MPI_STATUS_IGNORE);
	}

	// Payload exchange.
	std::vector<MPI_Request> requests;
	requests.reserve(DirectionIndex::NUM_DIRS * 2);
	std::array<std::vector<phys_particle_t>, DirectionIndex::NUM_DIRS> recv_bufs;

	for(int k = 0; k < DirectionIndex::NUM_DIRS; ++k){
		int neighbor = neighbor_ranks[k];
		int rcount = recv_counts[k];
		if(rcount > 0){
			recv_bufs[k].resize(rcount);
			MPI_Request req;
			MPI_Irecv(recv_bufs[k].data(), rcount, ghost_particle_mpidt,
					  neighbor, 400 + k, my_comm, &req);
			requests.push_back(req);
		}
		int scount = send_counts[k];
		if(scount > 0){
			MPI_Request req;
			MPI_Isend(send_bufs[k].data(), scount, ghost_particle_mpidt,
					  neighbor, 400 + k, my_comm, &req);
			requests.push_back(req);
		}
	}

	if(!requests.empty()){
		MPI_Waitall(static_cast<int>(requests.size()), requests.data(), MPI_STATUSES_IGNORE);
	}

	// Fill all_ghosts contiguously.
	unsigned int ghost_idx = 0;
	for(int k = 0; k < DirectionIndex::NUM_DIRS; ++k){
		for(const auto &p : recv_bufs[k]){
			if(ghost_idx >= get_max_particles()){
				throw std::runtime_error("Ghost buffer overflow");
			}
			all_ghosts[ghost_idx++] = p;
		}
	}
	N_ghosts = ghost_idx;

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
