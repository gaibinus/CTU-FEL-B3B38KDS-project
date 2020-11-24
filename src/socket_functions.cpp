#include "socket_functions.h"


// VARIABLES ///////////////////////////////////////////////////////////////////////////////////////////////////

std::map<char, std::string> frameTypeDict = {
    {'H', "HASH"},
    {'+', "positive"},
    {'-', "negative"},
    {'N', "name"},
    {'S', "size"},
    {'D', "data"},
};


// SOCKET CLASS FUNCTIONS //////////////////////////////////////////////////////////////////////////////////////

void socketClass::init(std::string set_gender) {
    // set socket gender
    if(set_gender.compare("server") == 0) gender = true;
    else if (set_gender.compare("client") == 0) gender = false;
    else errPrintf("initializing socket gender");

    // initialize address sizes
    server_size = sizeof(server_addr);
    client_size = sizeof(client_addr);

    // set memory size for addresses
    memset(&server_addr, 0, server_size); 
    memset(&client_addr, 0, client_size); 

    // create socket file descriptor 
    if((fileDescr = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        errPrintf("creating socket failed"); 
    
    printf("Socket created successfully\n");

    // fill server information 
    server_addr.sin_family = AF_INET;                   // IPv4
    server_addr.sin_port = htons(PORT);                 // listen to port
    server_addr.sin_addr.s_addr = inet_addr(IP_ADDR);   // listen to address
}


void socketClass::bindServer(void) {
    // bind the socket with the server address
    if(bind(fileDescr, (const struct sockaddr *)&server_addr, server_size) < 0 )
        errPrintf("binding socket failed"); 
    
    printf("Socket bind successfull\n\n");
}


void socketClass::updateTimeout(int sec, int usec) {
    // update timeout parameters
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;

    // update socket
    if(setsockopt(fileDescr, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        errPrintf("setting socket timeout failed"); 
}


void socketClass::waifForPing(void) {
    // update timeout to no limit
    updateTimeout(0, 0);

    // receive anything and throw it away
    uint8_t tmp[FRAME_SIZE];
    if(recvfrom(fileDescr, tmp, FRAME_SIZE, 0, (struct sockaddr *)&client_addr, &client_size) < 0)
        errPrintf("incoming ping corrupted");

    printf("- ping from client received\n");
}


void socketClass::sendPing(void) {
    if(sendto(fileDescr, "You deserve someone better...", 30, 0, (struct sockaddr *)&server_addr, server_size) == -1)
        errPrintf("pinging server failed");

    printf("Ping to server sent\n");
}


void socketClass::sendDataFrame(struct dataFrameClass* frame) {
    // add CRC to frame
    frame->crc = CRC::Calculate(frame, FRAME_SIZE-4, CRC::CRC_32());

    if(sendto(fileDescr, frame, FRAME_SIZE, 0, (struct sockaddr *)&client_addr, client_size) == -1)
        errPrintf("sending data frame no. " + std::to_string(frame->id));

    printf("SENT: %d -> %s\n", frame->id, frameTypeDict[frame->type].c_str());
}


void socketClass::sendAckFrame(struct ackFrameClass* frame) {
    // add CRC to frame
    frame->crc = CRC::Calculate(frame, 5, CRC::CRC_32());

    if(sendto(fileDescr, frame, 9, 0, (struct sockaddr *)&server_addr, server_size) == -1)
        errPrintf("sending ack frame no. " + std::to_string(frame->id));

    printf("SENT: %d -> %s\n", frame->id, frameTypeDict[frame->type].c_str());
}


int socketClass::receiveDataFrame(struct dataFrameClass* frame) {
    // check if timeout
    if(recvfrom(fileDescr, frame, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, &server_size) == -1) {
        printf("WARNING: data frame no. %d timeout\n", frame->id);
        return -1;
    }

    // check if CRC is that fucking magic number
    if(CRC::Calculate(frame, FRAME_SIZE, CRC::CRC_32()) != CRCmagic) {
        printf("WARNING: data frame no. %d CRC fraud\n", frame->id);
        return -2;
    }
    
    printf("RECV: %d <- %s\n", frame->id, frameTypeDict[frame->type].c_str());
    return 0;
}


int socketClass::receiveAckFrame(struct ackFrameClass* frame) {
    // check if timeout
    if(recvfrom(fileDescr, frame, 9, 0, (struct sockaddr *)&client_addr, &client_size) == -1) {
        printf("WARNING: data frame no. %d timeout\n", frame->id);
        return -1;
    }

    // check if CRC is that fucking magic number
    if(CRC::Calculate(frame, 9, CRC::CRC_32()) != CRCmagic) {
        printf("WARNING: data frame no. %d CRC fraud\n", frame->id);
        return -2;
    }
    
    printf("RECV: %d <- %s\n", frame->id, frameTypeDict[frame->type].c_str());
    return 0;
}
