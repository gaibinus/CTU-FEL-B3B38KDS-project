# B3B38KDS semestral project
Simple UDP server to client file transfer console application.

## BUILD
After changing any of the header files, you may call:
```
make clean
```
Make will compile separate server and client applications:
```
make
```

## USAGE
To run the server, you need to specify which file you want to send:
```
./server -f <file path>
```
To run the client, you need to specify the folder where to save the received file:
```
./client -f <folder path>
