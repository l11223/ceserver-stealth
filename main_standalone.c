#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "stealth_core.h"

int main(int argc, char* argv[]) {
    printf("[*] CEServer Stealth starting...\n");
    
    if (stealth_init() != 0) {
        printf("[!] Stealth init warning\n");
    }
    
    uint16_t port = stealth_get_random_port();
    printf("[*] Port: %d\n", port);
    printf("[*] Disguised as: %s\n", stealth_get_random_process_name());
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { 
        perror("socket"); 
        return 1; 
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); 
        return 1;
    }
    
    listen(server_fd, 5);
    printf("[*] Listening on 0.0.0.0:%d\n", port);
    printf("[*] Connect from CE: <phone_ip>:%d\n", port);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd >= 0) {
            printf("[+] Connection from %s:%d\n", 
                   inet_ntoa(client_addr.sin_addr), 
                   ntohs(client_addr.sin_port));
            close(client_fd);
        }
    }
    return 0;
}
