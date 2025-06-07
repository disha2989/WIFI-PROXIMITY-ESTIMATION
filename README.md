# WIFI-PROXIMITY-ESTIMATION
Signal Aware Proximity detection in WiFi networks

This project provides a set of lightweight, C-based user-space tools to estimate the distance of client devices from an OpenWRT Wi-Fi router. It leverages physical-layer metrics such as **RSSI**, **SNR**, and **RTT** to perform distance approximation. The tools are designed to be executed directly on the router and are cross-compiled using the official **OpenWRT SDK**.

## Repository Structure
-> src/ # Source files for the distance estimation tools
rssi_distance.c # Estimates distance using average RSSI values based on the log-distance path loss model. It parses the average RSSI value for each station from "iw dev phy0-ap0 station dump" output, polls this value for 10s to get the average RSSI and estimates the distance of that connected station.

snr_distance.c # Uses both RSSI and noise floor to computer SNR. Using this SNR, it adjusts the path-loss exponent dynamically to estimate distance with better accuracy compared to rssi_distance in noise environments.

rtt_distance.c # It measures RTT (ICMP ping-based round-trip time) to the stations. It applies speed-of-light propagation model to then estimate the relative distance of two stations. Since, ping has multiple layers of overhead in the network stack and due to WiFi MAC/PHY delays, this RTT measure might not be accurate estimate of distance. But, I want to highlight that FTM RTT defined in IEEE MAC 802.1mc contains AP and the station exchanging timestamped frames at the PHY layer. And since, it uses hardware clocks for getting the RTT times, it has higher precision and results in higher accuracy of distance estimation.

-> bin # Output directory for cross-compiled binaries
rssi_distance:

root@OpenWrt:~# ./rssi_distance 
Collecting signal data from connected stations...

MAC Address          Avg RSSI     Distance (m)

28:6b:35:a4:87:56    -33          2.71      
e2:9c:c9:54:f0:84    -38          3.98      
ea:ba:d6:08:cb:2d    -41          5.01      
9e:38:29:60:b3:98    -36          3.41      
18:02:ae:62:0c:4b    -61          23.26     
74:a6:cd:d5:14:bd    -37          3.69

snr_distance:

root@OpenWrt:~# ./snr_distance 
Using noise floor: -89 dBm
Sampling RSSI data...

MAC Address          Avg RSSI   SNR    Distance (m)

28:6b:35:a4:87:56    -33        56     3.03      
e2:9c:c9:54:f0:84    -34        55     3.30      
ea:ba:d6:08:cb:2d    -40        49     4.89      
9e:38:29:60:b3:98    -35        54     3.59      
18:02:ae:62:0c:4b    -59        30     18.12     
74:a6:cd:d5:14:bd    -38        51     4.64

rtt_distance:

Measuring RTTs using ICMP...

IP: 192.168.1.238 | Median RTT: 10130.00 us 
IP: 192.168.1.163 | Median RTT: 3487.00 us 

--- Relative Distance Estimation ---
IP: 192.168.1.238 | Relative Distance: 2.91
IP: 192.168.1.163 | Relative Distance: 1.00

-> openwrt-sdk* # OpenWRT SDK that is used for cross-compilation. I have compiled these binaries on my local Ubuntu machine and transfered the binaries to the OpenWRT router.

Download the appropriate OpenWRT SDK for your router architecture from [OpenWRT SDK Downloads](https://downloads.openwrt.org/), then extract it using `tar -xJf <sdk-file>.tar.zst`.

-> Makefile # Build script to cross-compile the src files and copy the binaries to the OpenWRT router
