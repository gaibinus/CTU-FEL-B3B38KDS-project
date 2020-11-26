#ifndef SOCKET_FUNCTIONS_H
#define SOCKET_FUNCTIONS_H

#pragma once

#include "tools.h"
#define  CRCPP_USE_CPP11 // enables C++11
#include "CRC.h"

#include <stdlib.h> 
#include <iostream>
#include <string.h>

#include <sys/socket.h> // sockets
#include <arpa/inet.h>  // sockaddr_in
#include <map>          // dictionary
#include <unistd.h>     // close()


/* definitions to share between client and server */
# define TIMEOUT_US 50000
# define FRAME_SIZE 1024
# define SERVER_IP "127.0.0.1"
# define CLIENT_IP "25.59.70.175"

# define SERVER_LOCAL 15001
# define SERVER_TARGET 14000
# define CLIENT_LOCAL 15000
# define CLIENT_TARGET 14001

/* CRC32 magic number */
# define CRCmagic 558161692


// VARIABLES ////////////////////////////////////////////////////////////////////////////////////////////////

/* map for frame data types */
extern std::map<char, std::string> frameTypeDict;


// CLASSES //////////////////////////////////////////////////////////////////////////////////////////////////

/* class for socket info */
class socketClass{
    private:
        int                 descr;                              // socket file descriptor
        struct sockaddr_in  addr;                               // server addres
        socklen_t           addrSize;                           // client addres
        struct timeval      timeout;                            // socket timeout structure

    public:
        void init(const char* ip, int port, bool bindFlag);    // initialize socket
        ~socketClass() {close(descr);}                          // class descrutor

        void updateTimeout(int sec, int usec);                  // update socket timeout

        void waifForPing(void);                                 // wait for incoming ping
        void sendPing(void);                                    // ping server

        void sendDataFrame(struct dataFrameClass* frame);       // send data frame
        void sendAckFrame(struct ackFrameClass* frame);         // send acknowledgement frame

        int  receiveDataFrame(struct dataFrameClass* frame, uint32_t idExp);    // receive data frame
        int  receiveAckFrame(struct ackFrameClass* frame, uint32_t idExp);      // receive acknowledgement class
};


/* class for data frame */
struct dataFrameClass {
    uint32_t id;                    // frame ID
    uint8_t  type;                  // type
    uint16_t dataLen;               // size of data
    char     data[FRAME_SIZE-11];   // frame data
    uint32_t crc;                   // CRC
}__attribute__((packed));


/* class for acknowledgement frame */
struct ackFrameClass {
    uint32_t id;                    // frame ID
    uint8_t  type;                  // type: positive or negative ACK
    uint32_t crc;                   // CRC
}__attribute__((packed));


#endif
