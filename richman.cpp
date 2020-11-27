#include "richman.h"

// RICHMAN STATES
#define INITIAL 1
#define RESTING 2
#define LOOKING_FOR_GROUP 3
#define IN_GROUP 4
#define LOOKING_FOR_PORTAL 5
#define IN_PORTAL 6
#define IN_DIMENSION 7


// TUNNEL STATES
#define BOTH_WAYS 1
#define TO_DIMENSION 2
#define TO_REAL_WORLD 3

// MESSAGE TYPES
#define GREQ 1
#define GACK 2

Richman::Richman(int groupSize, int tunnelCapacity, int tunnelCount) {
    this->state = INITIAL;
    this->clock = 0;

    size = MPI::COMM_WORLD.Get_size();
	rank = MPI::COMM_WORLD.Get_rank();
    this->groupSize = groupSize;
    this->groupBossId = -1;
    this->bossClock = -1;

    for (int i = 0; i < tunnelCount; i++) {
		s_tunnel* tunnel = new s_tunnel();
		tunnel->id = i;
		tunnel->capacity = tunnelCapacity;
		tunnel->direction = BOTH_WAYS;
		this->tunnels.push_back(tunnel);
	}

    this->state = RESTING;
    this->log("Initialized!");
}

void Richman::log(std::string msg) {
    printf("[ID: %d][CLOCK: %d][STATE: %d]: %s\n", this->rank, this->clock, this->state, msg.c_str());
};

void Richman::makeMonitorThread() {
    new std::thread(&Richman::monitorThread, this);
};

s_message Richman::createMessage(int value, int type) {
    s_message message;
    message.value = value;
    message.sender_id = this->rank;
    message.clock = this->clock;
    message.type = type;

    this->clock_mutex.lock();
    this->clock++;
    this->clock_mutex.unlock();
    
    return message;
};

void Richman::processMessage(s_message* message) {
    switch(message->type) {
        case GREQ: {
            switch(this->state) {
                case LOOKING_FOR_GROUP: {
                    int bossId = this->groupBossId != -1 ? this->groupBossId : this->rank;
                    int bossClock = this->bossClock != -1 ? this->bossClock : this->clock;
                    if(message->clock < bossClock || (message->clock == bossClock && message->sender_id < bossId)) {
                        bossId = message->sender_id;
                    }

                    this->groupBossId = bossId;
                    s_message msgToSend = this->createMessage(bossId, GACK);
                    MPI_Send(&msgToSend, sizeof(s_message), MPI_BYTE, message->sender_id, GACK, MPI_COMM_WORLD);
                    break;
                }
                default: {
                    // TODO: ADD TO QUEUE
                }
            }

            break;
        }
        default: {
            this->log("JESZCZE NIE OBSLUZONE!");
        }
    }
};

void Richman::monitorThread() {
	while(true) {
		s_message message;
		MPI_Status status;
		MPI_Recv(&message, sizeof(s_message), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		message.type = status.MPI_TAG;
		
		processMessage(&message);
        this->log("Dostalem wiadomosc od " + std::to_string(message.sender_id) + " value: " + std::to_string(message.value));

		this->clock_mutex.lock();
		this->clock = std::max(this->clock, message.clock) + 1;
		this->clock_mutex.unlock();
	}
}

// void Richman::sendMsgToRandom() {
//     int randomId = this->rank;
//     while(this->rank == randomId) {
//         randomId++;// = rand() % size;
//         randomId = randomId % size;
//     }

//     s_message msg = this->createMessage(69);
//     this->log("Sending msg to " + std::to_string(randomId));
//     MPI_Send(&msg, sizeof(s_message), MPI_BYTE, randomId, 1, MPI_COMM_WORLD);
// }

void Richman::sendToAll(s_message* message) {
    for (int i = 0; i < this->size; i++){
		if (i != this->rank) {
			MPI_Send(message, sizeof(s_message), MPI_BYTE, i, message->type, MPI_COMM_WORLD);
            this->log("Sent msg of type " + std::to_string(message->type) + " to " + std::to_string(i));
		}
	}
}

void Richman::beRichMan() {
    while(true) {
		std::this_thread::sleep_for(std::chrono::milliseconds((rand()%2000) + 1000));
        this->state = LOOKING_FOR_GROUP;
        s_message message = createMessage(0, GREQ);
        sendToAll(&message);

        while(true){}
    }
}