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
# define IP_ADDR "127.0.0.1"
# define PORT 8080
# define FRAME_SIZE 1024

/* CRC32 magic number */
# define CRCmagic 558161692


// VARIABLES ////////////////////////////////////////////////////////////////////////////////////////////////

/* map for frame data types */
extern std::map<char, std::string> frameTypeDict;


// CLASSES //////////////////////////////////////////////////////////////////////////////////////////////////

/* class for socket info */
class socketClass{
    private:
        int                 fileDescr;                          // socket file descriptor
        bool                gender;                             // socket identity: true = server, false = client
        struct sockaddr_in  server_addr;                        // server addres
        socklen_t           server_size;                        // client addres
        struct sockaddr_in  client_addr;                        // client addres
        socklen_t           client_size;                        // client addres size
        struct timeval      timeout;                            // socket timeout structure

    public:
        void init(std::string set_gender);                      // initialize socket
        ~socketClass() {close(fileDescr);}                      // class descrutor

        void bindServer(void);                                  // bind server
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


#endif
