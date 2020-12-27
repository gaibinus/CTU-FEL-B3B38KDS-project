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


    // SELECTIVE REPEAT MANAGEMENT ///////////////////////////////////////////////////////////////////////////

    // create window class and init queue
    windowClass window;
    window.fillQueue(file.size);


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
        // create frame with file name & send it
        if(scheduler == 0) {
            dataFrame.id = 0;
            dataFrame.type = 'N';
            dataFrame.dataLen = file.name.length()+1;
            memcpy(&dataFrame.data, file.name.c_str(), dataFrame.dataLen);
            target.sendDataFrame(&dataFrame);
        }
        // create frame with file size & send it
        else if(scheduler == 1) {
            dataFrame.id = 1;
            dataFrame.type = 'S';
            dataFrame.dataLen = sizeof(uint32_t);
            memcpy(&dataFrame.data, (uint8_t *)&file.size, dataFrame.dataLen);
            target.sendDataFrame(&dataFrame);
        }
        // manage data frames
        else if(scheduler == 2) {
            // loop thru window records and erase timeouts
            for(uint32_t i=0; i<window.onAir.size(); i++) {
                if(getElapsedTime(window.onAir[i].stamp) > (double)TIMEOUT_S + (double)TIMEOUT_US*1e-6) {
                    // inform user
                    printf("WARNING: frame no. %u timeout\n", window.onAir[i].id);

                    // push frame id back to queue
                    window.queue.push_front(window.onAir[i].id);

                    // remove from onAir
                    window.onAir.erase(window.onAir.begin() + i);
                }
            }

            // add frame to window & send them
            while(window.onAir.size() < WINDOW_SIZE && !window.queue.empty()) {
                // get frame ID from queue
                dataFrame.id = window.queue.front();
                window.queue.pop_front();

                // create frame record and push it to window class
                window.onAir.push_back(frameRecord(dataFrame.id, getCurrentTime()));

                // create frame and send it
                dataFrame.type = 'D';
                fseek(file.pointer, ((dataFrame.id-2) * (FRAME_SIZE-11)), SEEK_SET);
                dataFrame.dataLen = fread(&dataFrame.data, 1, (FRAME_SIZE-11), file.pointer);
                target.sendDataFrame(&dataFrame);
            }
        }
        // create frame with HASH & send it
        else if(scheduler == 3) {
            dataFrame.id = UINT32_MAX;
            dataFrame.type = 'H';
            dataFrame.dataLen = file.hash.length()+1;
            memcpy(&dataFrame.data, file.hash.c_str(), dataFrame.dataLen);
            target.sendDataFrame(&dataFrame);
        }
        
        // process all responces (skip if timeout or bad CRC)
        while(true) {
            // check input state - ignore bad CRC, exit when timeout
            int state = local.receiveAckFrame(&ackFrame);
            if(state == -1) break;
            else if(state == -2) continue;

            // positive ACK
            if(ackFrame.type == '+') {
                // move scheduler and update timeout
                if(scheduler == 0 && ackFrame.id == 0)
                    scheduler = 1;
                else if(scheduler == 1 && ackFrame.id == 1) {
                    scheduler = 2;
                    local.updateTimeout(0,1);
                }
                else if(scheduler == 3 && ackFrame.id == UINT32_MAX)
                    return 0;
                
                // remove frame record from onAir
                if(scheduler == 2) {
                    for(uint32_t i=0; i<window.onAir.size(); i++) {
                        if(window.onAir[i].id == ackFrame.id) {
                            window.onAir.erase(window.onAir.begin() + i);
                            break;
                        }
                    }
                }

                // check if file transfer completed
                if(scheduler == 2 && window.onAir.empty() && window.queue.empty()) {
                        scheduler = 3;
                        local.updateTimeout(TIMEOUT_S, TIMEOUT_US);
                }
            }

            // negative ACK -> remove from onAir and push to queue
            else if(ackFrame.type == '-' && scheduler == 2) {
                    window.queue.push_front(ackFrame.id);
                    for(uint32_t i=0; i<window.onAir.size(); i++) {
                        if(window.onAir[i].id == ackFrame.id) {
                            window.onAir.erase(window.onAir.begin() + i);
                            break;
                        }
                    }
            }
        }
    }
}
