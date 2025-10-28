#ifndef _XYZ_WRITER_H_
#define _XYZ_WRITER_H_

#include "gassim2d.h"

#ifndef __cplusplus
#include <stdio.h>
#endif

inline void write_xyz(FILE *thefile, const phys_particle_t *p_array, const int N){
	fprintf(thefile, "%d\n",N);//Number of particles
	fprintf(thefile, "\n");//Comment line
	for(int j = 0;j<N;j++)
		fprintf(thefile, "Ar %f %f \n", p_array[j].p.x, p_array[j].p.y);
	fflush(thefile);
}


#ifdef __cplusplus
#include <string>
#include <cstdio>

//#include <iostream>

class XYZWriter {
public:
	virtual void open(const std::string filename) = 0;
	virtual void open(const std::string filename, const std::string mode) = 0;
	virtual void close() = 0;
	virtual void write_xyz(const phys_particle_t *p_array, const int N) = 0;
	virtual void flush() = 0;
	virtual const std::string& get_filename() const = 0;
};

class CFILEXYZWriter : public XYZWriter {
private:
	std::string my_filename = "";
	FILE *outfile = NULL;
public:
	virtual void open(const std::string filename, const std::string mode){
			my_filename = filename;
			outfile = fopen(filename.c_str(),mode.c_str());
			//fflush(outfile);
			fclose(outfile);
			outfile = NULL;
		}
	virtual void open(const std::string filename){
			this->open(filename,"w");
		}
	virtual void close(){/* if(outfile != NULL){ fclose(outfile);} outfile=NULL; */}
	virtual void write_xyz(const phys_particle_t *p_array, const int N){
		//std::cerr << "Trying reopen" << std::endl;
		//outfile = freopen(my_filename.c_str(), "a", outfile);
		outfile = fopen(my_filename.c_str(),"a");
		::write_xyz(outfile, p_array, N);
		fflush(outfile);
		fclose(outfile);
		outfile = NULL;
	}
	virtual void flush(){}
	virtual const std::string& get_filename() const {return my_filename;}
};

/*

//Later versions of the class.

class MPIXYZWriter {
private:
	MPI_comm my_comm;
	MPI_File my_file;
	MPI_Status my_status;
public:
	MPIXYZWriter(MPI_comm _comm) : my_comm(_comm) {}
	~MPIXYZWriter(){};

	virtual void open(const std::string filename){MPI_File_open(my_comm, filename.c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL, &my_file); }
	virtual void close(){MPI_File_close(&my_file);}
	virtual void write_xyz(const phys_particle_t *p_array, const int N) = 0;
	virtual void flush() = 0;
	virtual const std::string& get_filename() const = 0;
};
*/


#endif /* __cplusplus */

#endif /* _XYZ_WRITER_H_ */
