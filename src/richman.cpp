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

const std::string typeMap[] = {"", "TUNNEL_REQ", "TUNNEL_ACK", "TRIP", "TRIP_FINISHED"};

// CONSTS
#define NO_MSG_VALUE -1

std::string colorId(int id) {
    return "\033[3" +std::to_string((id+1)) + "m" +std::to_string(id) + "\033[0m";
};

Richman::Richman(int maxGroupSize, int tunnelCapacity, int tunnelCount) {
    this->state = INITIAL;
    this->clock = 0;

    size = MPI::COMM_WORLD.Get_size();
	rank = MPI::COMM_WORLD.Get_rank();

    this->tunnelAckCounter = 0;
    this->groupSize = rand()%(maxGroupSize-4) + 4;
    this->currentDirection = TO_PARADISE;
    this->currentTunnelId = -1;

    tunnels_mutex.lock();
    for (int i = 0; i < tunnelCount; i++) {
		s_tunnel* tunnel = new s_tunnel();
		tunnel->id = i;
		tunnel->capacity = tunnelCapacity;
        tunnel->capacityTaken = 0;
		tunnel->direction = BOTH_WAYS;
		this->tunnels.push_back(tunnel);
	}
    tunnels_mutex.unlock();

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

    // VALUES
    message.tunnel_id = tunnel_id;
    message.taken_capacity = capacity;
    message.direction = direction;

    this->clock_mutex.lock();
    message.clock = this->clock;
    this->clock++;
    this->clock_mutex.unlock();
    
    return message;
};

bool Richman::doesReceivedMessageHavePriority(s_message receivedMessage) {
    s_message myMsg;

    //TODO: maybe don't store my request in this vector and do it separately?
    for(s_message iterMsg : this->currentRequests) {
        if(iterMsg.sender_id == this->rank) {
            myMsg = iterMsg;
            break;
        }
    }

    return receivedMessage.clock < myMsg.clock || (receivedMessage.clock == myMsg.clock && receivedMessage.sender_id < myMsg.sender_id);
}

void Richman::processMessage(s_message receivedMessage, bool sentManually=false) {

    switch(receivedMessage.type) {
        case TRIP: {
            this->updateOrAddRichManToTunnel(receivedMessage);
            break;
        }
        case TRIP_FINISHED: {
            this->removeRichManFromTunnel(receivedMessage);
            if(this->tunnelAckCounter == (size - 1)) {
                tunnels_mutex.lock();
                for(s_tunnel* t : this->tunnels) {
                    bool directionRequirement = t->direction == BOTH_WAYS || t->direction == this->currentDirection;
                    bool capacityRequirement = (t->capacity - t->capacityTaken) >= this->groupSize;
                    if(directionRequirement && capacityRequirement) {
                        this->log("I("+std::to_string(this->groupSize)+") have chosen tunnnel id: " + std::to_string(t->id));
                        this->currentTunnelId = t->id;
                        t->capacityTaken += this->groupSize;
                        t->direction = this->currentDirection;
                        t->richmanIds.push_back(this->rank);


                        s_message message = createMessage(TRIP, this->currentTunnelId, this->groupSize, this->currentDirection);
                        sendToAll(message);
                        for(s_message msg : this->currentRequests) {
                            if(msg.sender_id != this->rank) {
                                s_message message = this->createMessage(TUNNEL_ACK, this->currentTunnelId, this->groupSize, this->currentDirection);
                                MPI_Send(&message, sizeof(s_message), MPI_BYTE, msg.sender_id, message.type, MPI_COMM_WORLD);
                                this->log("Wysylam wiadomosc o typie " + typeMap[message.type] + " do " + colorId(msg.sender_id));
                            }
                        }

                        this->currentRequests.clear();

                        this->setState(IN_TUNNEL);
                        break;
                    }
                }
                tunnels_mutex.unlock();
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
                    MPI_Send(&message, sizeof(s_message), MPI_BYTE, receivedMessage.sender_id, message.type, MPI_COMM_WORLD);
                    this->log("Wysylam wiadomosc o typie " + typeMap[message.type] + " do " + colorId(receivedMessage.sender_id));
                    break;
                }
            }
            break;
        }
        case LOOKING_FOR_TUNNEL: {
            switch(receivedMessage.type) {
                case TUNNEL_REQ: {
                    if(this->doesReceivedMessageHavePriority(receivedMessage)) {
                        s_message message = this->createMessage(TUNNEL_ACK);
                        MPI_Send(&message, sizeof(s_message), MPI_BYTE, receivedMessage.sender_id, message.type, MPI_COMM_WORLD);
                        this->log("Wysylam wiadomosc o typie " + typeMap[message.type] + " do " + colorId(receivedMessage.sender_id));
                    } else {
                        this->currentRequests.push_back(receivedMessage);
                    }
                   
                    break;
                }
                case TUNNEL_ACK: {
                    this->tunnelAckCounter++;

                    if(receivedMessage.tunnel_id != NO_MSG_VALUE) {
                       this->updateOrAddRichManToTunnel(receivedMessage);
                    }

                    if(this->tunnelAckCounter == (size - 1)) {
                        this->log("I have tunnel choosing power!");

                        tunnels_mutex.lock();
                        for(s_tunnel* t : this->tunnels) {
                            bool directionRequirement = t->direction == BOTH_WAYS || t->direction == this->currentDirection;
                            bool capacityRequirement = (t->capacity - t->capacityTaken) >= this->groupSize;
                            if(directionRequirement && capacityRequirement) {
                                this->log("I("+std::to_string(this->groupSize)+") have chosen tunnnel id: " + std::to_string(t->id));
                                this->currentTunnelId = t->id;
                                t->capacityTaken += this->groupSize;
                                t->direction = this->currentDirection;
                                t->richmanIds.push_back(this->rank);


                                s_message message = createMessage(TRIP, this->currentTunnelId, this->groupSize, this->currentDirection);
                                sendToAll(message);
                                for(s_message msg : this->currentRequests) {
                                    if(msg.sender_id != this->rank) {
                                        s_message message = this->createMessage(TUNNEL_ACK, this->currentTunnelId, this->groupSize, this->currentDirection);
                                        MPI_Send(&message, sizeof(s_message), MPI_BYTE, msg.sender_id, message.type, MPI_COMM_WORLD);
                                        this->log("Wysylam wiadomosc o typie " + typeMap[message.type] + " do " + colorId(msg.sender_id));
                                    }
                                }

                                this->currentRequests.clear();

                                this->setState(IN_TUNNEL);
                                break;
                            }
                        }
                        tunnels_mutex.unlock();
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
                    MPI_Send(&message, sizeof(s_message), MPI_BYTE, receivedMessage.sender_id, message.type, MPI_COMM_WORLD);
                    this->log("Wysylam wiadomosc o typie " + typeMap[message.type] + " do " + colorId(receivedMessage.sender_id));
                }
            }
            break;
        }
        default: {
            this->log("Stan nie ma obsługi!");
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

void Richman::sendToAll(s_message message) {
    for (int i = 0; i < this->size; i++){
		if (i != this->rank) {
			MPI_Send(&message, sizeof(s_message), MPI_BYTE, i, message.type, MPI_COMM_WORLD);
            this->log("Wysylam wiadomosc o typie " + typeMap[message.type] + " do " + colorId(i));
		}
	}
}

void Richman::removeRichManFromTunnel(s_message message) {
    this->tunnels_mutex.lock();
    s_tunnel* t = this->tunnels[message.tunnel_id];
    std::vector<int>* v = &(t->richmanIds);
    int taskIndex = 0;
    for (int i = 0; i < (int)(v->size()); i++) {
        if (message.sender_id == v->at(i)) {
            taskIndex = i;
            break;
        }
    }

    v->erase(v->begin() + taskIndex);
    t->capacityTaken -= message.taken_capacity;

    if(v->size() == 0) {
        t->direction = BOTH_WAYS;
    }

    this->tunnels_mutex.unlock();
}

void Richman::updateOrAddRichManToTunnel(s_message message) {
    tunnels_mutex.lock();
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
    
    tunnels_mutex.unlock();
    
}

void Richman::beRichMan() {
    s_message message;
    while(true) {
        this->rest();
        this->setState(LOOKING_FOR_TUNNEL);

        // TUNNEL ACTION STARTS HERE
        message = createMessage(TUNNEL_REQ);
        this->currentRequests.push_back(message);
        sendToAll(message);

        // TEMPORARY STOP
        while(this->state != IN_TUNNEL){}
        std::this_thread::sleep_for(std::chrono::milliseconds((rand()%3000) + 2000));
        message = createMessage(TRIP_FINISHED, this->currentTunnelId, this->groupSize, this->currentDirection);
        removeRichManFromTunnel(message);
        this->currentDirection = TO_REAL_WORLD;
        this->log("I left tunnel id: " + std::to_string(this->currentTunnelId));
        this->currentTunnelId = -1;
        sendToAll(message);
        this->setState(IN_PARADISE);
        std::this_thread::sleep_for(std::chrono::milliseconds((rand()%3000) + 1000));
        message = createMessage(TUNNEL_REQ);
        this->currentRequests.push_back(message);
        sendToAll(message);

        // TEMPORARY STOP
        while(this->state != IN_TUNNEL){}
        std::this_thread::sleep_for(std::chrono::milliseconds((rand()%3000) + 2000));
        message = createMessage(TRIP_FINISHED, this->currentTunnelId, this->groupSize, this->currentDirection);
        removeRichManFromTunnel(message);
        this->currentDirection = TO_PARADISE;
        this->currentTunnelId = -1;
    
        sendToAll(message);
        this->rest();
    }
}

void Richman::rest() {
    this->setState(RESTING);
    this->tunnelAckCounter = 0;
    std::this_thread::sleep_for(std::chrono::milliseconds((rand()%3000) + 1000));

}