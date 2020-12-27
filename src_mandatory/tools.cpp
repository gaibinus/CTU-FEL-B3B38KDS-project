#include "tools.h"


// FILE CLASS FUNCTIONS ////////////////////////////////////////////////////////////////////////////////////////

void fileClass::processFile(char* new_path) {
    // conver path to string
    path.assign(std::string(new_path));

    // find last position of separator
    const long unsigned int idx = path.rfind('/', path.length());

    // if exist, name is content from right of it
    if(idx == std::string::npos) errPrintf("parsing file name");
    name.assign(path.substr(idx+1, path.length()-idx));

    // open file and store its pointer
    pointer = fopen(path.c_str(), "rb");
    if(pointer==NULL) errPrintf("opening file");

    // compute and store hash (before size check)
    hash = sha256(pointer);

    // determine file size
    if(fseek(pointer, 0, SEEK_END) != 0) errPrintf("loading file size");
    size = ftell(pointer);

    // inform user
    printf("File processed successfully\n");
    printf("- file name: %s\n", name.c_str());
    printf("- file size: %d[B]\n\n", size);
}

// FUNCTIONS ///////////////////////////////////////////////////////////////////////////////////////////////////

char* getCmdOption(char** begin, char** end, const std::string & option) {
    char** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) return *itr;
    return 0;
}


bool cmdOptionExists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}


void errPrintf(std::string msg) {
    printf("ERROR: %s\n", msg.c_str());
    exit(EXIT_FAILURE);
}


std::string sha256(FILE* file) {
    // prepare buffers and vars
    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned char buffer[256];
    int bytesRead = 0;

    // initialize SHA class
    SHA256_CTX sha256;
    if(!SHA256_Init(&sha256)) printf("initializing SHA");

    // loop thru file while updating SHA class, close file
    while((bytesRead = fread(buffer, 1, 256, file)))
        if(!SHA256_Update(&sha256, buffer, bytesRead)) printf("updating SHA");

    // finalize SHA class
    if(!SHA256_Final(hash, &sha256)) errPrintf("finalize SHA");

    // format hash to HEX
    char tmp_buff[65];
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(tmp_buff + 2*i, "%02x", hash[i]);
    tmp_buff[64] = '\0';

    // return string
    return std::string(tmp_buff);
}
