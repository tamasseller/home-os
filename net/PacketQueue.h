/*
 * PacketQueue.h
 *
 *  Created on: 2017.12.20.
 *      Author: tooma
 */

#ifndef PACKETQUEUE_H_
#define PACKETQUEUE_H_

template<class S, class... Args>
class Network<S, Args...>::PacketQueue
{
    PacketChain input, output;

public:
    inline void putPacketChain(Packet packet) {
    	input.push(packet);
    }

    inline bool isEmpty() {
    	return output.isEmpty() && input.isEmpty();
    }

    inline bool takePacketFromQueue(Packet &ret) {
    	if(output.isEmpty()) {
    		if(input.isEmpty())
    			return false;

    		input.flip();
    		output = input;
    		input.reset();
    	}

    	ret = output.pop();
    	return true;
	}
};


#endif /* PACKETQUEUE_H_ */
