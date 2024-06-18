# net-rush
### Simple network packet flooder

## Compilation
First of all, you need G++ installed: `sudo apt install g++ -y`.<br>
To compile main.cpp you can use this command: `g++ main.cpp -o main`

## Available methods
### normal methods
`utcpsyn` - SYN flood
`utcpflood` - TCP flood with junk packets (but it have a problem: it sends a really few amount of packets: 20-30 per second).
`uudpflood` - UDP flood.

### root-required methods
`rtcpflood` - flooding junk TCP packets with raw sockets.
**Important note: ** I checked this method on dstat.cc server and i did not saw incoming packets, but I saw a lot of outgoing packets from my pc network interface.

### You can see more information using `./main --help` command.
