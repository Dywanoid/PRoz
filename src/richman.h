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

        int group_ack_counter;
        int group_who_ack_counter;
        bool amIheBoss;
        std::vector<int> groupMembersIds;

        std::vector<s_message> messageQueue;
        void log(std::string);
        void monitorThread();
        s_message createMessage(int, int);
        void processMessage(s_message, bool);
        bool determinePriority(s_message);

    public:
        Richman(int, int, int);
        void makeMonitorThread();
        // void sendMsgToRandom();
        void beRichMan();
        void sendToAll(s_message);

};