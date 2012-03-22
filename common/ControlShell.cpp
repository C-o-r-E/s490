/* 
 * File:   ControlShell.cpp
 * Author: chanman
 * 
 * Created on March 16, 2012, 9:45 PM
 */

#include "ControlShell.h"
#include "Packet.h"
#include <cassert>

ControlShell::ControlShell(bbtag _tag) {
    tag = _tag;
}

ControlShell::~ControlShell() {
}

bool ControlShell::connectCS() {
    log("connect\n");
    return connectBB();
}

void ControlShell::disconnectCS() {
    log("disconnect\n");
    disconnectBB();
}

/*
  This function is for getting a set of data points from the 
  corresponding knowledgeItem in the Blackboard. 
 */
std::deque<DataPoint> ControlShell::getRecent(int numRequested) {
    // assume that this is connection is only used for this tag
    // and that all GET_RECENT requests have completed (i.e. no data left in the recf queue)

    log("get recent %d\n", numRequested);

    // send request
    Packet request;
    request.resize(12);
    request.setU32(0, BO_CS_GET_RECENT);
    request.setU32(4, tag);
    request.setU32(8, numRequested);
    sendPacket(request);

    Packet response;
    std::deque<DataPoint> ret;	
	
	processOutgoing();

    // wait for response
    while (true) {
        waitForEvents();
		
        // if we have a packet to work with, deal with it
        if (recvPacket(response)) {
			uint32_t opcode = response.getU32(0);
			switch(opcode){
			case BO_CS_RECENT:
			{
				assert(response.getU32(4) == tag);
                uint32_t numDataPoints = response.getU32(8);
				uint32_t pos = 12;
				for(uint32_t i=0; i< numDataPoints; ++i) {
					DataPoint dp;
					uint32_t dp_size = response.getU32(pos);
					pos += 4;
					//now read that many bytes 
					for(uint32_t j=0; j<dp_size; ++j) {
						dp.push_back(response.getU8(pos));
						log("%#X ", dp.back());
						pos += 1;
					}
					ret.push_back(dp);					
				}
				break;
			}
			default:
			{
				log("\n\nUnknown Packet %d\n\n", opcode);
			}
			}

			break;
        }
    }

    return ret;
}

DataPoint ControlShell::getMostRecent() {
    std::deque<DataPoint> retSet = getRecent(1);
    if (retSet.size() == 0) {
        return DataPoint();
    }
    return retSet.front();
}

//TODO: make it so callbacks only happen for the corresponding tag 
void ControlShell::registerCallback(void (*callback)(bbtag, DataPoint)) {
    callbacks.insert(callback);
}

void ControlShell::releaseCallback(void (*callback)(bbtag, DataPoint)) {
    callbacks.erase(callback);
}

void ControlShell::checkForUpdates() {
	Packet pack;
	if(recvPacket(pack)){
		uint32_t opcode = pack.getU32(0);
		if(opcode == BO_CS_UPDATE){
			uint32_t tag = pack.getU32(4);
			uint32_t len = pack.getU32(8);
			DataPoint dp;
			
			for(uint32_t i=0; i<len; ++i) {
				dp.push_back(pack.getU8(12+i));
			}

			//TODO: make it so callbacks only happen for the corresponding tag 
			std::set< void (*)(bbtag, DataPoint) >::const_iterator it;
			for(it=callbacks.begin(); it!=callbacks.end(); ++it){
				(*(*it))(tag, dp);
			}

		} else {
			//TODO: what now motha fucka?
			log("checkForUpdates: recv packet that is not mine... \n");
		}
	}
}
