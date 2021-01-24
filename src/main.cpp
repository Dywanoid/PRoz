#include <stdio.h>
#include <mpi.h>
#include "richman.h"

#define MAX_GROUP_SIZE 1
#define TUNNEL_CAPACITY 2
#define TUNNELS_COUNT 2

int main(int argc, char **argv) {
    int groupSize = MAX_GROUP_SIZE;
    int tunnelCapacity = TUNNEL_CAPACITY;
    int tunnelsCount = TUNNELS_COUNT;

    if(argc == 4) {
        groupSize = atoi(argv[1]);
        tunnelCapacity = atoi(argv[2]);
        tunnelsCount = atoi(argv[3]);
    }

	MPI::Init_thread(MPI_THREAD_MULTIPLE);
	
    Richman rich(groupSize, tunnelCapacity, tunnelsCount);

    rich.beRichMan();

	MPI::Finalize();
}