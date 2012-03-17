#include "KnowledgeSource.h"
#include "BlackboardConnection.h"

KnowledgeSource::KnowledgeSource(bbtag _tag) {
    tag = _tag;
}

KnowledgeSource::~KnowledgeSource() {
}

bool KnowledgeSource::connectKS() {
    return (connectBB() && registerForKI());
}

void KnowledgeSource::disconnectKS() {
    // unregister for KI
}

bool KnowledgeSource::registerForKI() {
    log("Register for %d\n", tag);
    
    // send request
    Packet request;
    request.resize(8);
    request.setU32(0, BO_KS_SUBSCRIBE_AS);
    request.setU32(4, tag);
    sendPacket(request);
    
    log("request sent\n");
    
    // TODO: handle the possibility where we get a packet and and the send queue is blocked (loop over this basically)
    processMsgQueue();
    
    Packet response;
    
    while (true) {
        waitForEvents(); // block until we have something
        processMsgQueue(); 
        
        // if we have a packet to work with, deal with it
        if (recvPacket(response)) {
            if (response.getU32(0) == BO_KS_SUBSCRIPTION_SUCCESS) {
                return true;
            } 
            if (response.getU32(0) == BO_KS_SUBSCRIPTION_FAILED) {
                return true;
            } 
        }
        
        // TODO: handle any other possible packets (none yet)
    }
}

bool KnowledgeSource::update(DataPoint point) {
    Packet response;
    
    response.resize(8);
    response.setU32(0, BO_KS_UPDATE);
    response.setU32(4, tag);
    response.insert(response.end(), point.begin(), point.end());
    
    sendPacket(response);
     
    this->processMsgQueue();

    // actually confirm response 
    return true;
}