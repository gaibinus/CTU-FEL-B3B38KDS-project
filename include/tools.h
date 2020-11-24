#ifndef TOOLS_H
#define TOOLS_H

#pragma once

#include <stdlib.h> 
#include <iostream>
#include <string.h>

#include <stdio.h>          // file mamanegemnt
#include <algorithm>        // arguent parsing
#include <openssl/sha.h>    // HASH sha256


/* class for file */
class fileClass{      
    public:
        FILE*       pointer;        // pointer to open file
        uint32_t    size;           // size of the file
        std::string path;           // path to file
        std::string name;           // name of the file
        std::string hash;           // file hash info

        void processFile(char* new_path);   // open file, process info
};


/* returns value of specific argument */
char* getCmdOption(char** begin, char** end, const std::string & option);


/* checks if specific argument exists */
bool cmdOptionExists(char** begin, char** end, const std::string& option);


/* prints error message and terminates program */
void errPrintf(std::string msg);


/* computes file sha256 HASH */
std::string sha256(FILE* pointer);


# endif
