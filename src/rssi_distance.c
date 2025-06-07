#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define MAX_LINE_LEN 256
#define MAX_DEVICES 64
#define MAX_SAMPLES 10
#define INTERFACE "phy0-ap0"

typedef struct {
    char mac[32];
    int rssi_samples[MAX_SAMPLES];
    int sample_count;
} station_info_t;

station_info_t stations[MAX_DEVICES];
int station_count = 0;

double estimate_distance_from_rssi(int rssi_avg) {
    int ptx = 20;     // Tx power (dBm)
    int pl_d0 = 40;   // Path loss at 1m
    double n = 3.0;   // Path-loss exponent
    return pow(10.0, ((ptx - rssi_avg - pl_d0) / (10.0 * n)));
}

station_info_t* create_station(const char *mac) {
    for (int count = 0; count < station_count; count++) {
        if (strcmp(stations[count].mac, mac) == 0) {
            return &stations[count];
        }
    }
    if (station_count >= MAX_DEVICES) 
		return NULL;
    strncpy(stations[station_count].mac, mac, sizeof(stations[station_count].mac) - 1);
    stations[station_count].sample_count = 0;
    return &stations[station_count++];
}

void sample_rssi() {
	int rssi;
    char command[128];
	char line[MAX_LINE_LEN], mac[32];
	FILE *fp = NULL;
    station_info_t *station = NULL;

    snprintf(command, sizeof(command), "iw dev %s station dump", INTERFACE);
    fp = popen(command, "r");
    if (!fp) {
        perror("popen failed");
        exit(1);
    }

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "Station ", 8) == 0) {
            sscanf(line, "Station %31s", mac);
            station = create_station(mac);
        } else if (station && strstr(line, "signal avg")) {
            if (sscanf(line, " signal avg: %d dBm", &rssi) == 1) {
                if (station->sample_count < MAX_SAMPLES) {
                    station->rssi_samples[station->sample_count++] = rssi;
                }
            }
        }
    }

    pclose(fp);
}

int main() {
	station_info_t *info;
	int sum = 0, avg_rssi = 0;
	int sample = 0, count = 0;
	double dist = 0;

    printf("Collecting signal data from connected stations...\n");

    for (sample = 0; sample < MAX_SAMPLES; sample++) {
        sample_rssi();
        sleep(1);
    }

    printf("\n%-20s %-12s %-10s\n", "MAC Address", "Avg RSSI", "Distance (m)");
    printf("---------------------------------------------------\n");

    for (count = 0; count < station_count; count++) {
        info = &stations[count];
        sum = 0;
        for (sample = 0; sample < info->sample_count; sample++) {
            sum += info->rssi_samples[sample];
        }
        avg_rssi = sum / info->sample_count;
        dist = estimate_distance_from_rssi(avg_rssi);

        printf("%-20s %-12d %-10.2f\n", info->mac, avg_rssi, dist);
    }

    return 0;
}
