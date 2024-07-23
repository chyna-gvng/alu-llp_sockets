#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void *save_message(void *message) {
    FILE *file = fopen("log.txt", "a");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }
    fprintf(file, "%s\n", (char *)message);
    fclose(file);
    return NULL;
}

void *count_words(void *message) {
    char *str = (char *)message;
    int count = 0;
    int in_word = 0;
    
    while (*str) {
        if (*str == ' ' || *str == '\n' || *str == '\t') {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            count++;
        }
        str++;
    }
    
    return (void *)(intptr_t)count;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        
        int valread = read(new_socket, buffer, BUFFER_SIZE);
        
        pthread_t thread1, thread2;
        void *word_count;
        
        pthread_create(&thread1, NULL, save_message, (void *)buffer);
        pthread_create(&thread2, NULL, count_words, (void *)buffer);
        
        pthread_join(thread1, NULL);
        pthread_join(thread2, &word_count);
        
        int count = (int)(intptr_t)word_count;
        char count_str[20];
        sprintf(count_str, "%d", count);
        send(new_socket, count_str, strlen(count_str), 0);
        
        close(new_socket);
    }
    
    return 0;
}
