#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <string>
#include <mpi.h>

typedef struct {
	int type;
	int value;
	int sender_id;
	int clock;
} s_message;


typedef struct {
	int id; 
	int capacity; 
    int direction;
	std::vector<int> richmanIds;
} s_tunnel;

class Richman {
    private:
        int state;

        int clock;
        std::mutex clock_mutex;

        int size;
        int rank;

        int groupSize;
        std::vector<s_tunnel*> tunnels;

        int groupBossId;
        int bossClock;
        std::vector<int> groupMembersIds;

        void log(std::string);
        void monitorThread();
        s_message createMessage(int, int);
        void processMessage(s_message*);

    public:
        Richman(int, int, int);
        void makeMonitorThread();
        // void sendMsgToRandom();
        void beRichMan();
        void sendToAll(s_message*);

};