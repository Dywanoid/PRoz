#include "richman.h"

// RICHMAN STATES
#define INITIAL 0
#define RESTING 1
#define LOOKING_FOR_TUNNEL 2
#define IN_TUNNEL 3
#define IN_PARADISE 4

// TUNNEL STATES
#define BOTH_WAYS 1
#define TO_PARADISE 2
#define TO_REAL_WORLD 3

// MESSAGE TYPES
#define TUNNEL_REQ 1
#define TUNNEL_ACK 2
#define TRIP 3
#define TRIP_FINISHED 4

// CONSTS
#define NO_MSG_VALUE -1
const std::string typeMap[] = {"", "TUNNEL_REQ", "TUNNEL_ACK", "TRIP", "TRIP_FINISHED"};

std::string colorId(int id) {
    return "\033[3" +std::to_string((id+1)) + "m" +std::to_string(id) + "\033[0m";
};

Richman::Richman(int maxGroupSize, int tunnelCapacity, int tunnelCount) {
    this->state = INITIAL;
    this->clock = 0;

    this->size = MPI::COMM_WORLD.Get_size();
	this->rank = MPI::COMM_WORLD.Get_rank();
    srand(time(NULL) + this->rank);

    this->tunnelAckCounter = 0;

    this->groupSize = rand()%(maxGroupSize-2) + 2;
    this->log("my group size: " +std::to_string(this->groupSize));
    this->currentDirection = TO_PARADISE;
    this->currentTunnelId = -1;

    for (int i = 0; i < tunnelCount; i++) {
		s_tunnel* tunnel = new s_tunnel();
		tunnel->id = i;
		tunnel->capacity = tunnelCapacity;
        tunnel->capacityTaken = 0;
		tunnel->direction = BOTH_WAYS;
		this->tunnels.push_back(tunnel);
	}

    this->otherProcessesClocks = std::vector<int>(size, 0);

    this->log("Initialized!");
    this->makeMonitorThread();
}

void Richman::log(std::string msg) {
    printf("[ID: %s][CLOCK: %d][STATE: %d]: %s\n", colorId(this->rank).c_str(), this->clock, this->state, msg.c_str());
};

void Richman::makeMonitorThread() {
    new std::thread(&Richman::monitorThread, this);
};

void Richman::setState(int newState) {
    this->state_mutex.lock();
    this->state = newState;
    this->state_mutex.unlock();
}

s_message Richman::createMessage(int type, int tunnel_id=NO_MSG_VALUE, int capacity=NO_MSG_VALUE, int direction=NO_MSG_VALUE) {
    s_message message;
    message.sender_id = this->rank;
    message.type = type;

    message.tunnel_id = tunnel_id;
    message.taken_capacity = capacity;
    message.direction = direction;

    this->clock_mutex.lock();
    message.clock = this->clock;
    this->clock++;
    this->clock_mutex.unlock();
    
    return message;
};

bool Richman::doesReceivedMessageHaveHigherPriority(s_message receivedMessage) {
    return receivedMessage.clock < this->myMsg.clock || (receivedMessage.clock == this->myMsg.clock && receivedMessage.sender_id < this->myMsg.sender_id);
}

void Richman::chooseTunnel() {
    tunnels_mutex.lock();
    for(s_tunnel* t : this->tunnels) {
        bool directionRequirement = t->direction == BOTH_WAYS || t->direction == this->currentDirection;
        bool capacityRequirement = (t->capacity - t->capacityTaken) >= this->groupSize;
        if(directionRequirement && capacityRequirement) {
            this->currentTunnelId = t->id;
            t->capacityTaken += this->groupSize;
            t->direction = this->currentDirection;
            t->richmanIds.push_back(this->rank);


            s_message message = createMessage(TRIP, this->currentTunnelId, this->groupSize, this->currentDirection);
            sendToAll(message);

            for(s_message request : this->heldRequests) {
                s_message message = this->createMessage(TUNNEL_ACK, this->currentTunnelId, this->groupSize, this->currentDirection);
                this->sendMessage(message, request.sender_id);                            
            }

            this->heldRequests.clear();

            this->tunnel_ack_mutex.lock();
            this->tunnelAckCounter = 0;
            this->tunnel_ack_mutex.unlock();

            this->setState(IN_TUNNEL);
            break;
        }
    }
    tunnels_mutex.unlock();
}

void Richman::processMessage(s_message receivedMessage, bool sentManually=false) {
    switch(receivedMessage.type) {
        case TRIP: {
            this->updateOrAddRichManToTunnel(receivedMessage);
            break;
        }
        case TRIP_FINISHED: {
            this->removeRichManFromTunnel(receivedMessage);
            this->tunnel_ack_mutex.lock();
            bool canChooseTunnel = this->tunnelAckCounter == (size - 1);
            this->tunnel_ack_mutex.unlock();

            if(canChooseTunnel) {
                this->chooseTunnel();
            }
            break;
        }
    }

    switch(this->state) {
        case IN_PARADISE:
        case RESTING: {
            switch(receivedMessage.type) {
                case TUNNEL_REQ: {
                    s_message message = this->createMessage(TUNNEL_ACK);
                    this->sendMessage(message, receivedMessage.sender_id);
                    break;
                }
            }
            break;
        }
        case LOOKING_FOR_TUNNEL: {
            switch(receivedMessage.type) {
                case TUNNEL_REQ: {
                    if(this->doesReceivedMessageHaveHigherPriority(receivedMessage)) {
                        s_message message = this->createMessage(TUNNEL_ACK);
                        this->sendMessage(message, receivedMessage.sender_id);
                    } else {
                        this->heldRequests.push_back(receivedMessage);
                    }
                   
                    break;
                }
                case TUNNEL_ACK: {
                    this->tunnel_ack_mutex.lock();
                    this->tunnelAckCounter++;
                    bool canChooseTunnel = this->tunnelAckCounter == (size - 1);
                    this->tunnel_ack_mutex.unlock();

                    if(receivedMessage.tunnel_id != NO_MSG_VALUE) {
                       this->updateOrAddRichManToTunnel(receivedMessage);
                    }

                    if(canChooseTunnel) {
                        this->chooseTunnel();
                    }
                    break;
                }
            }
            break;
        }
        case IN_TUNNEL: {
            switch(receivedMessage.type) {
                case TUNNEL_REQ: {
                    s_message message = this->createMessage(TUNNEL_ACK, this->currentTunnelId, this->groupSize, this->currentDirection);
                    this->sendMessage(message, receivedMessage.sender_id);
                }
            }
            break;
        }
    }
};

void Richman::monitorThread() {
    s_message message;
    MPI_Status status;
	while(true) {
		MPI_Recv(&message, sizeof(s_message), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		message.type = status.MPI_TAG;

        this->log("Dostalem wiadomosc od " + colorId(message.sender_id) +" typu " + typeMap[message.type] + "("+std::to_string(message.tunnel_id)+"|"+std::to_string(message.taken_capacity)+"|"+std::to_string(message.direction)+")");
        this->otherProcessesClocks[message.sender_id] = message.clock;

        processMessage(message);

		this->clock_mutex.lock();
		this->clock = std::max(this->clock, message.clock) + 1;
		this->clock_mutex.unlock();
	}
}

void Richman::sendMessage(s_message message, int toWho) {
    MPI_Send(&message, sizeof(s_message), MPI_BYTE, toWho, message.type, MPI_COMM_WORLD);
    this->log("Wysylam wiadomosc o typie " + typeMap[message.type] + " do " + colorId(toWho));
}

void Richman::sendToAll(s_message message) {
    for (int i = 0; i < this->size; i++){
		if (i != this->rank) {
            this->sendMessage(message, i);
		}
	}
}

void Richman::removeRichManFromTunnel(s_message message) {
    this->tunnels_mutex.lock();
    s_tunnel* tunnel = this->tunnels[message.tunnel_id];
    std::vector<int>* ids = &(tunnel->richmanIds);
    int taskIndex = 0;
    for (int i = 0; i < (int)(ids->size()); i++) {
        if (message.sender_id == ids->at(i)) {
            taskIndex = i;
            break;
        }
    }

    ids->erase(ids->begin() + taskIndex);
    tunnel->capacityTaken -= message.taken_capacity;

    if(ids->size() == 0) {
        tunnel->direction = BOTH_WAYS;
    }

    this->tunnels_mutex.unlock();
}

void Richman::updateOrAddRichManToTunnel(s_message message) {
    this->tunnels_mutex.lock();
    s_tunnel* tunnel = this->tunnels[message.tunnel_id];

    bool isAlreadyThere = false;
    for(int id : tunnel->richmanIds) 
        if(message.sender_id == id) {
            isAlreadyThere = true;
            break;
        }
    
    if(!isAlreadyThere) {
        tunnel->capacityTaken += message.taken_capacity;
        tunnel->direction = message.direction;
        tunnel->richmanIds.push_back(message.sender_id);
    }

    this->tunnels_mutex.unlock(); 
}

void Richman::beRichMan() {
    s_message message;
    while(true) {
        this->rest();

        this->setState(LOOKING_FOR_TUNNEL);
        this->log("Im looking for tunnel to paradise!");

        message = createMessage(TUNNEL_REQ);
        this->myMsg = message;
        sendToAll(message);

        // TEMPORARY STOP
        while(this->state != IN_TUNNEL){
            std::this_thread::sleep_for(std::chrono::milliseconds((rand()%15) + 10));
        }

        this->log("Im traveling to paradise!");
        std::this_thread::sleep_for(std::chrono::milliseconds((rand()%3000) + 2000));

        message = createMessage(TRIP_FINISHED, this->currentTunnelId, this->groupSize, this->currentDirection);
        removeRichManFromTunnel(message);
        this->currentDirection = TO_REAL_WORLD;
        this->currentTunnelId = -1;
        sendToAll(message);

        this->setState(IN_PARADISE);
        this->log("Im in paradise!");

        std::this_thread::sleep_for(std::chrono::milliseconds((rand()%3000) + 1000));
        this->setState(LOOKING_FOR_TUNNEL);
        this->log("Im looking for tunnel to real world!");

        message = createMessage(TUNNEL_REQ);
        this->myMsg = message;
        sendToAll(message);

        // TEMPORARY STOP
        while(this->state != IN_TUNNEL){
            std::this_thread::sleep_for(std::chrono::milliseconds((rand()%15) + 10));
        }

        this->log("Im traveling to real world!");
        std::this_thread::sleep_for(std::chrono::milliseconds((rand()%3000) + 2000));

        message = createMessage(TRIP_FINISHED, this->currentTunnelId, this->groupSize, this->currentDirection);
        removeRichManFromTunnel(message);
        this->currentDirection = TO_PARADISE;
        this->currentTunnelId = -1;
    
        sendToAll(message);
    }
}

void Richman::rest() {
    this->setState(RESTING);
    this->log("Im in real world!");

    this->tunnel_ack_mutex.lock();
    this->tunnelAckCounter = 0;
    this->tunnel_ack_mutex.unlock();

    std::this_thread::sleep_for(std::chrono::milliseconds((rand()%3000) + 1000));

}