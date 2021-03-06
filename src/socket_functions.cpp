#include "socket_functions.h"


// VARIABLES ///////////////////////////////////////////////////////////////////////////////////////////////////

std::map<char, std::string> frameTypeDict = {
    {'H', "HASH"},
    {'+', "positive"},
    {'-', "negative"},
    {'N', "name"},
    {'S', "size"},
    {'D', "data"},
    {'P', "ping"}
};

// FUNCTIONS ///////////////////////////////////////////////////////////////////////////////////////////////////

double getCurrentTime() {
    // get current time
    struct timeval timeObj;
    gettimeofday(&timeObj, 0);

    // merge seconds and miliseconds
    return timeObj.tv_sec + timeObj.tv_usec*1e-6;
}

double getElapsedTime(double start) {
    // get current time
    double end = getCurrentTime();

    // return elapsed time
    return end - start;
}


// SOCKET CLASS FUNCTIONS //////////////////////////////////////////////////////////////////////////////////////

void socketClass::init(const char* ip, int port, bool bindFlag) {
    // initialize address size
    addrSize = sizeof(addr);

    // set memory size for address
    memset(&addr, 0, addrSize); 

    // create socket file descriptor 
    if((descr = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        errPrintf("creating socket failed"); 
    
    printf("Socket created successfully\n");

    // fill server information 
    addr.sin_family = AF_INET;                   // IPv4
    addr.sin_port = htons(port);                 // listen to port
    addr.sin_addr.s_addr = inet_addr(ip);   // listen to address

    // skip if no bind
    if(bindFlag) {
        // bind the socket with the server address
        if(bind(descr, (const struct sockaddr *)&addr, addrSize) < 0 )
            errPrintf("binding socket failed"); 

        printf("Socket bind successfull\n\n");
    }
}


void socketClass::updateTimeout(int sec, int usec) {
    // update timeout parameters
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;

    // update socket
    if(setsockopt(descr, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        errPrintf("setting socket timeout failed"); 
}


void socketClass::waifForPing(void) {
    // update timeout to no limit
    updateTimeout(0, 0);

    // receive anything and throw it away
    uint8_t tmp[FRAME_SIZE];
    if(recvfrom(descr, tmp, FRAME_SIZE, 0, (struct sockaddr *)&addr, &addrSize) < 0)
        errPrintf("incoming ping corrupted");

    printf("- ping from client received\n");
}


void socketClass::sendPing(void) {
    if(sendto(descr, "You deserve someone better...", 30, 0, (struct sockaddr *)&addr, addrSize) == -1)
        errPrintf("pinging server failed");

    printf("Ping to server sent\n");
}


void socketClass::sendDataFrame(struct dataFrameClass* frame) {
    // add CRC to frame
    frame->crc = CRC::Calculate(frame, FRAME_SIZE-4, CRC::CRC_32());

    if(sendto(descr, frame, FRAME_SIZE, 0, (struct sockaddr *)&addr, addrSize) == -1)
        errPrintf("sending data frame no. " + std::to_string(frame->id));

    printf("SENT: %u -> %s\n", frame->id, frameTypeDict[frame->type].c_str());
}


void socketClass::sendAckFrame(struct ackFrameClass* frame) {
    // add CRC to frame
    frame->crc = CRC::Calculate(frame, 5, CRC::CRC_32());

    if(sendto(descr, frame, 9, 0, (struct sockaddr *)&addr, addrSize) == -1)
        errPrintf("sending ack frame no. " + std::to_string(frame->id));

    printf("SENT: %u -> %s\n", frame->id, frameTypeDict[frame->type].c_str());
}


int socketClass::receiveDataFrame(struct dataFrameClass* frame) {
    // check if timeout
    if(recvfrom(descr, frame, FRAME_SIZE, 0, (struct sockaddr *)&addr, &addrSize) == -1) {
        if(frame->type == 'P')
            printf("WARNING: ping timeout\n");

        return -1;
    }

    // check if CRC is that fucking magic number
    if(CRC::Calculate(frame, FRAME_SIZE, CRC::CRC_32()) != CRCmagic) {
        printf("WARNING: data frame CRC fraud\n");
        return -2;
    }
    
    printf("RECV: %u <- %s\n", frame->id, frameTypeDict[frame->type].c_str());
    return 0;
}


int socketClass::receiveAckFrame(struct ackFrameClass* frame) {
    // check if timeout
    if(recvfrom(descr, frame, 9, 0, (struct sockaddr *)&addr, &addrSize) == -1) {
        return -1;
    }

    // check if CRC is that fucking magic number
    if(CRC::Calculate(frame, 9, CRC::CRC_32()) != CRCmagic) {
        printf("WARNING: ack frame CRC fraud\n");
        return -2;
    }
    
    printf("RECV: %u <- %s\n", frame->id, frameTypeDict[frame->type].c_str());
    return 0;
}


void windowClass::fillQueue(uint32_t fileSize) {
    // loop thru queue and add frame IDs
    for(uint32_t i=0; i<(uint32_t)ceil((double)fileSize/(double)(FRAME_SIZE-11)); i++)
        queue.push_back(i+2);
}
