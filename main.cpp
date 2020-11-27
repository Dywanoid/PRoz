#include <stdio.h>
#include <mpi.h>
#include "richman.h"

#define GROUP_SIZE 3
#define TUNNEL_CAPACITY 4
#define TUNNELS_COUNT 2

int main(int argc, char **argv) {
	MPI::Init_thread(MPI_THREAD_MULTIPLE);
	
    // CREATE EVERYTHING HERE!
    Richman rich(GROUP_SIZE, TUNNEL_CAPACITY, TUNNELS_COUNT);
    rich.makeMonitorThread();
    // rich.sendMsgToRandom();
    rich.beRichMan();

	MPI::Finalize();
}