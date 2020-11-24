#include "socket_functions.h"
#include "tools.h"

#include <sys/types.h> 
#include <sys/time.h>
#include <netinet/in.h>

using namespace std;


struct test{
    char data[8];
    uint32_t crc;
}__attribute__((packed));


int main(int argc, char **argv){
    
    // FILE MANAGEMENT //////////////////////////////////////////////////////////////////////////////////////
    
    // create file class
    fileClass file;

    // parse filename from arguments
    char* arg_parsed = getCmdOption(argv, argv + argc, "-f");
    if(!arg_parsed) errPrintf("file path argument parsing failed\n");

    // process file
    file.processFile(arg_parsed);


    // SOCKET CREATION ///////////////////////////////////////////////////////////////////////////////////////

    // create socket class and init it as server
    socketClass sock;
    sock.init("server");

    // bind server socket
    sock.bindServer();


    // COMMUNICATION /////////////////////////////////////////////////////////////////////////////////////////

    // create frame classes
    dataFrameClass dataFrame; dataFrame.id = 0;
    ackFrameClass ackFrame;

    // wait for client to ping
    printf("Ready to serve client\n");
    sock.waifForPing();

    // update socket timeout
    sock.updateTimeout(10, 0);

    // scheduler 0:file name, 1:file size, 2:data, 3:hash
    int scheduler = 0;

    // send file until there is nothing to send
    while(scheduler < 4) {
        // create frame with file name
        if(scheduler == 0) {
            dataFrame.type = 'N';
            dataFrame.dataLen = file.name.length()+1;
            memcpy(&dataFrame.data, file.name.c_str(), dataFrame.dataLen);
        }

        // create frame with file size 
        else if(scheduler == 1) {
            dataFrame.type = 'S';
            dataFrame.dataLen = sizeof(uint32_t);
            memcpy(&dataFrame.data, (uint8_t *)&file.size, dataFrame.dataLen);
        }

        // create frame with data
        else if(scheduler == 2) {
            dataFrame.type = 'D';
            fseek(file.pointer, ((dataFrame.id-2) * (FRAME_SIZE-11)), SEEK_SET);
            dataFrame.dataLen = fread(&dataFrame.data, 1, (FRAME_SIZE-11), file.pointer);
        }

        // create frame with HASH
        else if(scheduler == 3) {
            dataFrame.type = 'H';
            dataFrame.dataLen = file.hash.length()+1;
            memcpy(&dataFrame.data, file.hash.c_str(), dataFrame.dataLen);
        }
        
        // send created frame and update expected id
        sock.sendDataFrame(&dataFrame);

        // waits for responce (skip if timeout or bad CRC)
        if(sock.receiveAckFrame(&ackFrame) != 0) continue;
        
        // check if is positive ACK and frame ID matches
        if(ackFrame.type == '+' && ackFrame.id == dataFrame.id) {
            // move scheduler
            if(scheduler != 2) scheduler++;
            else if(scheduler == 2 && (dataFrame.id-2) * (FRAME_SIZE-11) >= file.size) scheduler++;

            dataFrame.id++;
        }
        else if (ackFrame.id > dataFrame.id) 
            errPrintf("received ACK ID greater than sent data ID");
    }

    return 0;
}
