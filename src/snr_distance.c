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
int noise_floor_dbm = -95; // default fallback

double get_dynamic_n(int snr) {
    if (snr >= 50) return 2.7;       
    else if (snr >= 40) return 2.9;  
    else if (snr >= 30) return 3.1;  
    else if (snr >= 20) return 3.3;  
    else return 3.6;                
}

double estimate_distance_from_snr(int rssi, int snr) {
    int ptx = 20;        // Tx power (dBm)
    int pl_d0 = 40;      // Path loss at 1 meter (dB)
    double n = get_dynamic_n(snr);  // Use dynamic path loss exponent
    double distance;

    distance = pow(10.0, ((ptx - rssi - pl_d0) / (10.0 * n)));
    return distance;
}

station_info_t* create_station(const char *mac) {
    int i = 0;

    for (i = 0; i < station_count; i++) {
        if (strcmp(stations[i].mac, mac) == 0) {
            return &stations[i];
        }
    }

    if (station_count >= MAX_DEVICES)
        return NULL;

    strncpy(stations[station_count].mac, mac, sizeof(stations[station_count].mac) - 1);
    stations[station_count].sample_count = 0;
    return &stations[station_count++];
}

void get_noise_floor() {
    FILE *fp = NULL;
    char line[MAX_LINE_LEN];

    fp = popen("iw dev " INTERFACE " survey dump | grep noise", "r");
    if (!fp) {
        perror("popen noise");
        return;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, " noise: %d dBm", &noise_floor_dbm) == 1) {
            break;
        }
    }

    pclose(fp);
}

void sample_rssi() {
    FILE *fp = NULL;
    char command[128];
    char line[MAX_LINE_LEN];
    char mac[32];
    int rssi = 0;
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
    int sample = 0, count = 0;
    int sum = 0, avg_rssi = 0, snr = 0;
    double dist = 0.0;
    station_info_t *info = NULL;

    get_noise_floor();
    printf("Using noise floor: %d dBm\n", noise_floor_dbm);

    printf("Sampling RSSI data...\n");
    for (sample = 0; sample < MAX_SAMPLES; sample++) {
        sample_rssi();
        sleep(1);
    }

    printf("\n%-20s %-10s %-6s %-10s\n", "MAC Address", "Avg RSSI", "SNR", "Distance (m)");
    printf("---------------------------------------------------------------\n");

    for (count = 0; count < station_count; count++) {
        info = &stations[count];
        sum = 0;

        for (sample = 0; sample < info->sample_count; sample++) {
            sum += info->rssi_samples[sample];
        }

        avg_rssi = sum / info->sample_count;
        snr = avg_rssi - noise_floor_dbm;
        dist = estimate_distance_from_snr(avg_rssi, snr);

        printf("%-20s %-10d %-6d %-10.2f\n", info->mac, avg_rssi, snr, dist);
    }

    return 0;
}
