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


// SOCKET CLASS FUNCTIONS //////////////////////////////////////////////////////////////////////////////////////

void socketClass::init(int port, bool bindFlag) {
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
    addr.sin_addr.s_addr = inet_addr(IP_ADDR);   // listen to address

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

    printf("SENT: %d -> %s\n", frame->id, frameTypeDict[frame->type].c_str());
}


void socketClass::sendAckFrame(struct ackFrameClass* frame) {
    // add CRC to frame
    frame->crc = CRC::Calculate(frame, 5, CRC::CRC_32());

    if(sendto(descr, frame, 9, 0, (struct sockaddr *)&addr, addrSize) == -1)
        errPrintf("sending ack frame no. " + std::to_string(frame->id));

    printf("SENT: %d -> %s\n", frame->id, frameTypeDict[frame->type].c_str());
}


int socketClass::receiveDataFrame(struct dataFrameClass* frame, uint32_t idExp) {
    // check if timeout
    if(recvfrom(descr, frame, FRAME_SIZE, 0, (struct sockaddr *)&addr, &addrSize) == -1) {
        if(frame->type == 'P')
            printf("WARNING: ping timeout\n");

        else
            printf("WARNING: data frame no. %d (exp) timeout\n", idExp);

        return -1;
    }

    // check if CRC is that fucking magic number
    if(CRC::Calculate(frame, FRAME_SIZE, CRC::CRC_32()) != CRCmagic) {
        printf("WARNING: data frame no. %d (exp) CRC fraud\n", idExp);
        return -2;
    }
    
    printf("RECV: %d <- %s\n", frame->id, frameTypeDict[frame->type].c_str());
    return 0;
}


int socketClass::receiveAckFrame(struct ackFrameClass* frame, uint32_t idExp) {
    // check if timeout
    if(recvfrom(descr, frame, 9, 0, (struct sockaddr *)&addr, &addrSize) == -1) {
        printf("WARNING: data frame no. %d (exp) timeout\n", idExp);
        return -1;
    }

    // check if CRC is that fucking magic number
    if(CRC::Calculate(frame, 9, CRC::CRC_32()) != CRCmagic) {
        printf("WARNING: data frame no. %d (exp) CRC fraud\n", idExp);
        return -2;
    }
    
    printf("RECV: %d <- %s\n", frame->id, frameTypeDict[frame->type].c_str());
    return 0;
}
