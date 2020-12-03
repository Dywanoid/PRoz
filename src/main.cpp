#include <stdio.h>
#include <mpi.h>
#include "richman.h"

#define MAX_GROUP_SIZE 8
#define TUNNEL_CAPACITY 8
#define TUNNELS_COUNT 2

int main(int argc, char **argv) {
	MPI::Init_thread(MPI_THREAD_MULTIPLE);
	
    Richman rich(MAX_GROUP_SIZE, TUNNEL_CAPACITY, TUNNELS_COUNT);

    rich.beRichMan();

	MPI::Finalize();
}