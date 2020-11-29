#include "richman.h"

// RICHMAN STATES
#define INITIAL 0
#define RESTING 1
#define LOOKING_FOR_GROUP 2
#define IN_GROUP 3
#define LOOKING_FOR_TUNNEL 4
#define WAITING_FOR_TUNNEL 5
#define IN_TUNEL 6
#define IN_PARADISE 7


// TUNNEL STATES
#define BOTH_WAYS 1
#define TO_DIMENSION 2
#define TO_REAL_WORLD 3

// MESSAGE TYPES
#define GROUP_REQ 1
#define GROUP_ACK 2
#define GROUP_WHO_REQ 3
#define GROUP_WHO_ACK 4
#define GROUP_COMPLETED 5
#define TUNNEL_REQ 6
#define TUNNEL_ACK 7
#define TRIP 8
#define TRIP_FINISHED 9

// CONSTS
#define NO_MSG_VALUE 0

std::string colorId(int id) {
    return "\033[3" +std::to_string((id+1)) + "m" +std::to_string(id) + "\033[0m";
};

Richman::Richman(int groupSize, int tunnelCapacity, int tunnelCount) {
    this->state = INITIAL;
    this->clock = 0;

    size = MPI::COMM_WORLD.Get_size();
	rank = MPI::COMM_WORLD.Get_rank();
    this->groupSize = groupSize;
    this->amIheBoss = false;
    this->group_ack_counter = 0;
    this->group_who_ack_counter = 0;

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
    printf("[ID: %s][CLOCK: %d][STATE: %d]: %s\n", colorId(this->rank).c_str(), this->clock, this->state, msg.c_str());
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

bool Richman::determinePriority(s_message msg) {
    s_message myMsg;
    for(s_message iMsg : this->messageQueue) {
        if(iMsg.sender_id == this->rank) {
            myMsg = iMsg;
            break;
        }
    }

    // return msg->clock < this->clock || (msg->clock == this->clock && msg->sender_id < this->rank);
    return msg.clock < myMsg.clock || (msg.clock == myMsg.clock && msg.sender_id < myMsg.sender_id);
}

void Richman::processMessage(s_message receivedMessage, bool sentManually=false) {
    switch(this->state) {
        case RESTING: {
            switch(receivedMessage.type) {
                case GROUP_REQ: {
                    s_message msgToSend = this->createMessage(NO_MSG_VALUE, GROUP_ACK);
                    MPI_Send(&msgToSend, sizeof(s_message), MPI_BYTE, receivedMessage.sender_id, msgToSend.type, MPI_COMM_WORLD);
                    this->log("Odpoczywam, wiec pozawalam: " + colorId(receivedMessage.sender_id));
                }
            }

            break;
        }
        case LOOKING_FOR_GROUP: {
            switch(receivedMessage.type) {
                case GROUP_REQ: {
                    bool incomingMsgHasHigherPriority = determinePriority(receivedMessage);
                    if(incomingMsgHasHigherPriority) {
                        s_message msgToSend = this->createMessage(NO_MSG_VALUE, GROUP_ACK);
                        MPI_Send(&msgToSend, sizeof(s_message), MPI_BYTE, receivedMessage.sender_id, GROUP_ACK, MPI_COMM_WORLD);
                        this->log("Wysylam wiadomosc o typie " + std::to_string(msgToSend.type) + " do " + colorId(receivedMessage.sender_id));
                    } else {
                        if(!sentManually) { 
                            this->log("Dodaje " + colorId(receivedMessage.sender_id) + " do kolejki!");
                            this->messageQueue.push_back(receivedMessage);
                        }
                    }
                   
                    break;
                }
                case GROUP_ACK: {
                    this->group_ack_counter++;
                    if(this->size - this->groupSize == this->group_ack_counter) {
                        this->group_ack_counter = 0;
                        this->log("Mamo! Udalo sie!");
                        this->state = IN_GROUP;
                        s_message tempMsg = this->createMessage(NO_MSG_VALUE, GROUP_WHO_REQ);
                        this->sendToAll(tempMsg);
                    }
                    break;
                }
                case GROUP_COMPLETED: {
                    // GROUP WAS FORMED, SO EVERYTHING HAS TO START OVER
                    this->messageQueue.clear();
                    this->group_ack_counter = 0;
                    s_message message = createMessage(0, GROUP_REQ);
                    this->messageQueue.push_back(message);
                    this->sendToAll(message);
                    break;
                }
            }

            break;
        }
        case IN_GROUP: {
            switch(receivedMessage.type) {
                case GROUP_REQ: {
                    if(!sentManually) { 
                        this->messageQueue.push_back(receivedMessage);
                    }
                    break;
                }
                case GROUP_WHO_REQ: {
                    s_message msgToSend = this->createMessage(NO_MSG_VALUE, GROUP_WHO_ACK);
                    MPI_Send(&msgToSend, sizeof(s_message), MPI_BYTE, receivedMessage.sender_id, msgToSend.type, MPI_COMM_WORLD);
                    break;
                }
                case GROUP_WHO_ACK: {
                    this->group_who_ack_counter++;

                    if(this->groupSize - 1 == this->group_who_ack_counter) {
                        this->log("I AM THE BOSS!");
                        this->amIheBoss = true;
                        this->state = LOOKING_FOR_TUNNEL;
                        s_message msgToSend = this->createMessage(NO_MSG_VALUE, GROUP_COMPLETED);
                        this->sendToAll(msgToSend);

                        // for(s_message msgPtr : this->messageQueue) {
                        //     if(msgPtr.sender_id != this->rank) {
                        //         this->log("ratowany: " + colorId(msgPtr.sender_id));
                        //         this->processMessage(msgPtr, true);
                        //     }
                        // }
                        this->messageQueue.clear();
                    }

                    break;
                }
                case GROUP_COMPLETED: {
                    this->state = WAITING_FOR_TUNNEL;
                    this->log("Czekam na tunel!");
                    // for(s_message msgPtr : this->messageQueue) {
                    //     if(msgPtr.sender_id != this->rank) {
                    //         this->log("ratowany: " + colorId(msgPtr.sender_id));
                    //         this->processMessage(msgPtr, true);
                    //     }
                    // }
                    this->messageQueue.clear();
                    break;
                }
            }

            break;
        }
        case LOOKING_FOR_TUNNEL: {
            switch(receivedMessage.type) {
                case GROUP_REQ: {
                    s_message msgToSend = this->createMessage(NO_MSG_VALUE, GROUP_ACK);
                    MPI_Send(&msgToSend, sizeof(s_message), MPI_BYTE, receivedMessage.sender_id, msgToSend.type, MPI_COMM_WORLD);
                    break;
                }
            }
            break;
        }
        case WAITING_FOR_TUNNEL: {
             switch(receivedMessage.type) {
                case GROUP_REQ: {
                    s_message msgToSend = this->createMessage(NO_MSG_VALUE, GROUP_ACK);
                    MPI_Send(&msgToSend, sizeof(s_message), MPI_BYTE, receivedMessage.sender_id, msgToSend.type, MPI_COMM_WORLD);
                    break;
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
	while(true) {
		s_message message;
		MPI_Status status;
		MPI_Recv(&message, sizeof(s_message), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		message.type = status.MPI_TAG;
		
        this->log("Dostalem wiadomosc typu " + std::to_string(message.type) + " od " + colorId(message.sender_id) + " value: " + std::to_string(message.value));
		processMessage(message);

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

void Richman::sendToAll(s_message message) {
    for (int i = 0; i < this->size; i++){
		if (i != this->rank) {
			MPI_Send(&message, sizeof(s_message), MPI_BYTE, i, message.type, MPI_COMM_WORLD);
            this->log("Wysylam wiadomosc o typie " + std::to_string(message.type) + " do " + colorId(i));
		}
	}
}

void Richman::beRichMan() {
    while(true) {
		std::this_thread::sleep_for(std::chrono::milliseconds((rand()%6000) + 1000));
        this->state = LOOKING_FOR_GROUP;
        s_message message = createMessage(0, GROUP_REQ);
        messageQueue.push_back(message);
        sendToAll(message);

        while(true){}
    }
}