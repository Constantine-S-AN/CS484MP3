#ifndef __CHARMBLOCK_H__
#define __CHARMBLOCK_H__


#include <vector>

#include "gassim2d.h"
#include "simblock.h"
#include "xyz_writer.h"
#include "gas_constants.h"


class CharmSimBlock : public SimulationBlock {
public:
		CharmSimBlock() : SimulationBlock() {}
		CharmSimBlock(gas_simulation _in_sim, SimulationGrid _grid, int which_block);
		~CharmSimBlock(){};

		void pup(PUP::er &p);
private:
};

class CharmBlock : public CBase_CharmBlock {
	CharmBlock_SDAG_CODE
private:
	XYZWriter *file_output = nullptr;
	CharmSimBlock *my_sim_block = nullptr;
public:

	CharmBlock(
		int xsize, int ysize,
		int N_steps,
		std::string file_output_basename, int _output_at_iteration,
		int particle_migration_frequency,
		double spatial_extent,
		double temp_kelvin,
		double lattice_pitch, double high_density, double low_density,
		int generator_seed,
		int _process_migration_frequency
		);
	CharmBlock(CkMigrateMessage *msg);
	~CharmBlock(){if(nullptr != file_output){delete file_output;} if(nullptr != my_sim_block){delete my_sim_block;} }
	void hello();

	int output_at_iteration;

	int current_iteration;
	int N_steps;
	int n_neighbors;
	int n_received;

	int process_migration_frequency;

	void send_migrants();
	void send_ghosts();
	void receive_migrants_impl(int phase, int from, int direction, std::vector<phys_particle_t> migrants);
	void receive_ghosts_impl(int phase, int from, int direction, std::vector<phys_particle_t> ghosts);
	void handle_phase();

	void pup(PUP::er &p);
};

#endif /* __CHARMBLOCK_H__ */
