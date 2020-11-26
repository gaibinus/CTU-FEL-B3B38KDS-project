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


int main(int argc, char **argv) {
    
    // FILE MANAGEMENT //////////////////////////////////////////////////////////////////////////////////////
    
    // create file class
    fileClass file;

    // parse filename from arguments
    char* arg_parsed = getCmdOption(argv, argv + argc, "-f");
    if(!arg_parsed) errPrintf("file path argument parsing failed\n");

    // process file
    file.processFile(arg_parsed);


    // SOCKET CREATION ///////////////////////////////////////////////////////////////////////////////////////

    // create socket classes, init them and bind local one
    socketClass target;
    socketClass local;
    target.init("0.0.0.0", SERVER_TARGET, false);
    local.init("0.0.0.0", SERVER_LOCAL, true);

    // COMMUNICATION /////////////////////////////////////////////////////////////////////////////////////////

    // create frame classes
    dataFrameClass dataFrame; dataFrame.id = 0;
    ackFrameClass ackFrame;

    // wait for client to ping
    printf("Ready to serve client\n");
    local.waifForPing();

    // update socket timeout
    local.updateTimeout(TIMEOUT_S, TIMEOUT_US);

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
        target.sendDataFrame(&dataFrame);

        // waits for responce (skip if timeout or bad CRC)
        if(local.receiveAckFrame(&ackFrame, dataFrame.id) != 0)
            continue;
        
        // positive ACK and frame ID matches -> move to next frame
        if(ackFrame.type == '+' && ackFrame.id == dataFrame.id) {
            // move scheduler
            if(scheduler != 2) scheduler++;
            else if(scheduler == 2 && (dataFrame.id-2) * (FRAME_SIZE-11) >= file.size) scheduler++;

            // move to next frame
            dataFrame.id++;
        }
        // anything else -> repeat frame
        else
            continue;
    }

    return 0;
}
