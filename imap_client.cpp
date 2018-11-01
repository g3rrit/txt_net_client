#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>

#ifdef _WIN32

typedef SOCKET socket_t;

#define closesocket close

#define WIN32_LEAN_AND_MEAN

#include <windows>
#include <winsock2>
#include <ws2tcpip>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#else

typedef int socket_t;

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#endif


int _wsa_inited = 0;

void init_winsock(void) {
#ifdef _WIN32
    if(_wsa_inited) {
        return;
    }

    WSADATA wsa_data;
    int res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if(res) {
        printf("WSAStartup failed exiting...\n");
        WSACleanup();
        exit(-1);
    }

    _wsa_inited = 1;
#endif
}

void delete_winsock(void) {
#ifdef _WIN32
    WSACleanup();

    _wsa_inited = 0;
#endif
}

void print_socket_err(int err) {
#ifdef _WIN32
#else
    printf("error: [%s]\n", gai_strerror(err));
    printf("errno_error: [%s]\n", strerror(errno));
#endif
}



struct Imap_Client {
    socket_t csocket = 0;    

    Imap_Client() {
        init_winsock();
    }

    ~Imap_Client() {
        if(csocket) {
            close(csocket);
        }
        delete_winsock();
    }


    void init(char *addr, char *port) {
        int res = 0;         

        struct addrinfo *result = 0, *ptr = 0, hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        
        res = getaddrinfo(addr, port, &hints, &result);
        if(res) {
            printf("getaddrinfo failed with error: %i\n", res);
            print_socket_err(res);
            exit(-1);
        }

        for(ptr = result; !ptr; ptr = ptr->ai_next) {
            if(ptr->ai_family == AF_INET) {
                csocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
                if(csocket < 0) {
                    printf("failed to establish socket with error: %i\n", csocket); 
                }
            }
    
            res = connect(csocket, ptr->ai_addr, (int)ptr->ai_addrlen);
            if(res < 0) {
                close(csocket);
                csocket = 0;
                continue;
            }
            break;
        }

        freeaddrinfo(result);

        if(csocket <= 0) {
            printf("unable to connect to server\n");
            delete_winsock();
            exit(-1);
        }

        printf("established connection\n");
    }

    void disconnect() {
        if(csocket) {
            close(csocket);
        }
    }



};

int main(int argc, char **argv) {
    if(argc < 3) {
        printf("invalid input!\nUsage: ./imap_client <server> <port>\n");
        return 0;
    }

    printf("Starting imap client\n");

    printf("server: %s\n", argv[1]);
    printf("port: %s\n", argv[2]);
    
    Imap_Client client;

    client.init(argv[1], argv[2]);

    client.disconnect();

    return 0;
}