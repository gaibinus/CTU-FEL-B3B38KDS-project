#include "socket_functions.h"
#include "tools.h"

#include <sys/types.h> 
#include <sys/time.h>
#include <netinet/in.h>

using namespace std;


// CLASSES & VARIABLES & CONSTANTS //////////////////////////////////////////////////////////////////////////



// FUNCTIONS ////////////////////////////////////////////////////////////////////////////////////////////////



// MAIN /////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {

    // FILE MANAGEMENT ///////////////////////////////////////////////////////////////////////////////////////
    
    // create file class
    fileClass file;

    // parse destination folder from arguments
    char* arg_parsed = getCmdOption(argv, argv + argc, "-f");
    if(!arg_parsed){
        printf("ERROR: folder path argument parsing failed\n");
        return -1;
    }

    // assign file path
    file.path.assign(string(arg_parsed));


    // SOCKET CREATION ///////////////////////////////////////////////////////////////////////////////////////

    // create socket classes and init them
    socketClass local;
    socketClass target;
    local.init("0.0.0.0", CLIENT_LOCAL, true);
    target.init(SERVER_IP, CLIENT_TARGET, false);
    

    // COMMUNICATION /////////////////////////////////////////////////////////////////////////////////////////

    // create frame class
    dataFrameClass dataFrame; dataFrame.type = 'P';
    ackFrameClass ackFrame; ackFrame.id = 0;

    // update socket timeout
    local.updateTimeout(TIMEOUT_S, TIMEOUT_US);

    // ping scheck
    bool ping = false;
    
    // scheduler 0:file name, 1:file size, 2:data, 3:hash
    int scheduler = 0;

    // keep receiving frames
    while(true) {
        // ping server
        if(!ping) target.sendPing();

        // wait for server responce
        int state = local.receiveDataFrame(&dataFrame);

        // timeout -> repeat, timeot + hash -> finito
        if(state == -1) {
            if(dataFrame.type == 'H') break;
            else continue;
        }
        // CRC fraud -> bad ACK
        else if(state == -2) {
            ackFrame.type = '-';
            target.sendAckFrame(&ackFrame);
            continue;
        }

        // already processed frame -> send good ack
        if((dataFrame.type == 'N' && scheduler > 0) || (dataFrame.type == 'S' && scheduler > 1)) {
            // save expected ID
            uint32_t tmpId = ackFrame.id;

            // send positive ACK to received frame
            ackFrame.id = dataFrame.id;
            ackFrame.type = '+';
            target.sendAckFrame(&ackFrame);

            // restore expected ID
            ackFrame.id = tmpId;
            continue;
        }

        // file name -> process
        if(dataFrame.type == 'N') {
            // assign file name and update path
            file.name.assign(string(dataFrame.data));
            file.path.append('/' + file.name);

            // inform user and move scheduler
            printf("- received file name: %s\n", file.name.c_str());
            scheduler = 1;

            // remove timeout and ping
            local.updateTimeout(0, 0);
            ping = true;
        }
        // file size -> process
        else if(dataFrame.type == 'S') {
            // assign file size
            memcpy(&file.size, &dataFrame.data, sizeof(uint32_t));

            // inform user and move scheduler
            printf("- received file size: %d[B]\n", file.size);
            scheduler = 2;

            // crate destination file and store its pointer
            file.pointer = fopen(file.path.c_str(), "wb+");
            if(file.pointer==NULL) errPrintf("file creating failed");
            printf("- file created successfully\n");
        }
        // data -> process
        else if(dataFrame.type == 'D') {
            // move pointer
            fseek(file.pointer, (dataFrame.id-2)*(FRAME_SIZE-11), 0);

            // write to file
            if((uint32_t)fwrite(dataFrame.data, sizeof(uint8_t), dataFrame.dataLen, file.pointer) != dataFrame.dataLen)
                errPrintf("writing to file at data frame no. " + to_string(dataFrame.id));
        }
        // hash -> process
        else if(dataFrame.type == 'H') {
            // reset pointer nad compute hash
            fseek(file.pointer, 0, 0);
            file.hash = sha256(file.pointer);

            // compare hashes
            if(strcmp(file.hash.c_str(), dataFrame.data) != 0) {
                printf("#### FAIL: hash not match\n");
                printf("received: %s\n", dataFrame.data);
                printf("computed: %s\n", file.hash.c_str());
            }
            printf("#### PASS: hash match\n");

            // close file and move scheduler
            fclose(file.pointer);
            scheduler = 3;

            // set terminal timeout
            local.updateTimeout(2, 0);
        }

        // send good ack frame
        ackFrame.id = dataFrame.id;
        ackFrame.type = '+';
        target.sendAckFrame(&ackFrame);
    }
    
    return 0;
}