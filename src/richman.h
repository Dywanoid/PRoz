#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <mpi.h>

typedef struct {
	int type;
	int tunnel_id;
    int taken_capacity;
    int direction;
	int sender_id;
	int clock;
} s_message;

typedef struct {
	int id; 
	int capacity; 
    int capacityTaken;
    int direction;
	std::vector<int> richmanIds;
} s_tunnel;

typedef struct {
    int type;
    const char* name;
} s_type;

class Richman {
    private:
        // MPI RELATED
        int size;
        int rank;

        // PROCESS RELATED
        int state;
        std::mutex state_mutex;

        int clock;
        std::mutex clock_mutex;

        std::vector<int> otherProcessesClocks;

        std::vector<s_message> heldRequests;
        s_message myMsg;
        int tunnelAckCounter;
        std::mutex tunnel_ack_mutex;

        // PROJECT RELATED
        int groupSize;
        std::vector<s_tunnel*> tunnels;
        std::mutex tunnels_mutex;

        int currentDirection;
        int currentTunnelId;

        // METHODS
        void log(std::string);
        void monitorThread();
        s_message createMessage(int, int, int, int);
        void processMessage(s_message, bool);
        bool doesReceivedMessageHaveHigherPriority(s_message);
        void removeRichManFromTunnel(s_message);
        void updateOrAddRichManToTunnel(s_message);
        void rest();
        void chooseTunnel();
        void makeMonitorThread();
        void sendMessage(s_message, int);
        void sendToAll(s_message);
        void setState(int);
    public:
        Richman(int, int, int);
        void beRichMan();
};