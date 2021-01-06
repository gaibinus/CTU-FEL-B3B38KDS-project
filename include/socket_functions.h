#ifndef SOCKET_FUNCTIONS_H
#define SOCKET_FUNCTIONS_H

#pragma once

#include "tools.h"
#define  CRCPP_USE_CPP11 // enables C++11
#include "CRC.h"

#include <stdlib.h> 
#include <iostream>
#include <string.h>
#include <deque>
#include <vector>

#include <sys/socket.h> // sockets
#include <arpa/inet.h>  // sockaddr_in
#include <map>          // dictionary
#include <unistd.h>     // close()
#include <sys/time.h>   // time measurement
#include <math.h>       // ceil


/* definitions to share between client and server */
# define TIMEOUT_S 0
# define TIMEOUT_US 100000
# define FRAME_SIZE 1024
# define WINDOW_SIZE 10
# define SERVER_IP "25.58.246.19"
# define CLIENT_IP "127.0.0.1"

# define SERVER_LOCAL  15001    // A
# define SERVER_TARGET 14000    // B
# define CLIENT_LOCAL  15000    // A
# define CLIENT_TARGET 14001    // B

/* CRC32 magic number */
# define CRCmagic 558161692


// VARIABLES ////////////////////////////////////////////////////////////////////////////////////////////////

/* map for frame data types */
extern std::map<char, std::string> frameTypeDict;


// FUNCTIONS ////////////////////////////////////////////////////////////////////////////////////////////////

/* returns current time */
double getCurrentTime(); 

/* returns elapsed time */
double getElapsedTime(double start);


// CLASSES //////////////////////////////////////////////////////////////////////////////////////////////////

/* class for socket info */
class socketClass{
    private:
        int                 descr;                              // socket file descriptor
        struct sockaddr_in  addr;                               // server addres
        socklen_t           addrSize;                           // client addres
        struct timeval      timeout;                            // socket timeout structure

    public:
        void init(const char* ip, int port, bool bindFlag);     // initialize socket
        ~socketClass() {close(descr);}                          // class descrutor

        void updateTimeout(int sec, int usec);                  // update socket timeout

        void waifForPing(void);                                 // wait for incoming ping
        void sendPing(void);                                    // ping server

        void sendDataFrame(struct dataFrameClass* frame);       // send data frame
        void sendAckFrame(struct ackFrameClass* frame);         // send acknowledgement frame

        int  receiveDataFrame(struct dataFrameClass* frame);    // receive data frame
        int  receiveAckFrame(struct ackFrameClass* frame);      // receive acknowledgement class
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


/* class for frame dequeue */
struct frameRecord {
    uint32_t id;                    // frame ID
    double   stamp;                 // timestamp of frame sending

    frameRecord(uint32_t id_in, double stamp_in) {  // class constructor
        id = id_in;
        stamp = stamp_in;
    }
};


/* class for window management */
struct windowClass {
    std::deque<uint32_t> queue;         // waiting queue for frames
    std::vector<frameRecord> onAir;     // vector of active frames

    void fillQueue(uint32_t fileSize);  // fills waiting queue
};


#endif
