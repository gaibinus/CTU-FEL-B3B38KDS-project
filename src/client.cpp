#include "socket_functions.h"
#include "tools.h"

#include <sys/types.h> 
#include <sys/time.h>
#include <netinet/in.h>

using namespace std;


// CLASSES & VARIABLES & CONSTANTS //////////////////////////////////////////////////////////////////////////



// FUNCTIONS ////////////////////////////////////////////////////////////////////////////////////////////////



// MAIN /////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv){

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
    file.path.append(string(arg_parsed));


    // SOCKET CREATION ///////////////////////////////////////////////////////////////////////////////////////

    // create socket class and init it as server
    socketClass sock;
    sock.init("client");
    

    // COMMUNICATION /////////////////////////////////////////////////////////////////////////////////////////

    // create frame class
    dataFrameClass dataFrame;
    ackFrameClass ackFrame; ackFrame.id = 0;

    // update socket timeout
    sock.updateTimeout(10, 0);

    // ping scheck
    bool ping= false;

    // keep receiving frames
    while(true) {
        // ping server
        if(!ping) sock.sendPing();

        // wait for server responce
        int state = sock.receiveDataFrame(&dataFrame);

        // timeout -> repeat, timeot + hash -> finito
        if(state == -1) {
            if(dataFrame.type == 'H') break;
            else continue;
        }

        // CRC fraud -> bad ACK
        else if(state == -2) {
            ackFrame.type = '-';
            sock.sendAckFrame(&ackFrame);
        }

        // frame id is old -> good ACK
        if(dataFrame.id < ackFrame.id);
        
        // this should not happen -> if yes RUN
        else if(dataFrame.id > ackFrame.id)
            errPrintf("received data ID greater than sent ACK ID + 1");

        // file name -> process
        else if(dataFrame.type == 'N') {
            // assign file name and update path
            file.name.assign(string(dataFrame.data));
            file.path.append('/' + file.name);

            // inform user
            printf("- received file name: %s\n", file.name.c_str());

            // remove timeout and ping
            sock.updateTimeout(0, 0);
            ping = true;
        }

        // file size -> process
        else if(dataFrame.type == 'S') {
            // assign file size
            memcpy(&file.size, &dataFrame.data, sizeof(uint32_t));

            // inform user
            printf("- received file size: %d[B]\n", file.size);

            // crate destination file and store its pointer
            file.pointer = fopen(file.path.c_str(), "wb+");
            if(file.pointer==NULL) errPrintf("file creating failed");
            printf("- file created successfully\n");
        }

        // data -> process
        else if(dataFrame.type == 'D') {
            if((uint32_t)fwrite(dataFrame.data, sizeof(uint8_t), dataFrame.dataLen, file.pointer) != dataFrame.dataLen)
                errPrintf("writing to file at data frame no. " + to_string(dataFrame.id));
        }
        
        // hash -> process
        else if(dataFrame.type == 'H') {
            // compute file hash
            file.hash = sha256(file.pointer);

            // compare hashes
            if(strcmp(file.hash.c_str(), (char *)dataFrame.data) != 0)
                printf("WARNING: hash not match");

            // close file
            fclose(file.pointer);

            // set terminal timeout
            sock.updateTimeout(1, 0);
        }

        // send good ack frame
        ackFrame.id = dataFrame.id;
        ackFrame.type = '+';
        sock.sendAckFrame(&ackFrame);

        // expect next data
        ackFrame.id = dataFrame.id + 1;
    }
    
    return 0;
}