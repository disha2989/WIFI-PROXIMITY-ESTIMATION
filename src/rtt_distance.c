#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <errno.h>
#include <math.h>

#define PING_PKT_SIZE 64
#define TIMEOUT 1
#define SPEED_OF_LIGHT 299792458.0

// Calculates checksum for ICMP packet
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
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

double measure_rtt(const char *ip_addr) {
    struct sockaddr_in addr;
    int sockfd;
    struct icmphdr icmp_hdr;
    char packet[PING_PKT_SIZE];
    struct timeval start, end;

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("Socket");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);

    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.type = ICMP_ECHO;
    icmp_hdr.un.echo.id = getpid();
    icmp_hdr.un.echo.sequence = 1;
    icmp_hdr.checksum = checksum(&icmp_hdr, sizeof(icmp_hdr));

    memset(packet, 0, sizeof(packet));
    memcpy(packet, &icmp_hdr, sizeof(icmp_hdr));

    gettimeofday(&start, NULL);
    if (sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&addr, sizeof(addr)) <= 0) {
        perror("Sendto");
        close(sockfd);
        return -1;
    }

    struct sockaddr_in r_addr;
    socklen_t addr_len = sizeof(r_addr);
    char buf[1024];

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    struct timeval timeout = {TIMEOUT, 0}; // 1 sec

    if (select(sockfd + 1, &readfds, NULL, NULL, &timeout) > 0) {
        if (recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&r_addr, &addr_len) <= 0) {
            perror("Recvfrom");
            close(sockfd);
            return -1;
        } else {
            gettimeofday(&end, NULL);
            double rtt_usec = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
            close(sockfd);
            return rtt_usec;
        }
    } else {
        printf("Timeout from %s\n", ip_addr);
        close(sockfd);
        return -1;
    }
}

double estimate_distance_from_rtt_us(double rtt_us) {
    double rtt_sec = rtt_us * 1e-6;
    return (rtt_sec * SPEED_OF_LIGHT) / 2.0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <IP-ADDRESS>\n", argv[0]);
        return 1;
    }

    const char *target_ip = argv[1];

    printf("Measuring RTT to %s ...\n", target_ip);
    double rtt = measure_rtt(target_ip);
    if (rtt < 0) {
        printf("Failed to measure RTT\n");
        return 1;
    }

    printf("RTT: %.2f microseconds\n", rtt);
    double distance = estimate_distance_from_rtt_us(rtt);
    printf("Estimated distance: %.2f meters (approx)\n", distance);

    return 0;
}
