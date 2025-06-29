192.168.1.2 (client) - NORTH (192.168.1.1) - EAST - AP (192.168.5.1) - 192.168.5.3 (client)

ping -c 10 192.168.5.3 > ping.txt
ping -i 0.1 -c 100 192.168.5.3 > ping_burst.txt

traceroute 192.168.5.3 > traceroute.txt

iperf3 -c 192.168.5.3 -b#m > tcp_#m.txt
iperf3 -c 192.168.5.3 -b#m -R > tcp_R#m.txt

iperf3 -c 192.168.5.3 -u -b#m > udp_#m.txt
iperf3 -c 192.168.5.3 -u -b#m -R > udp_R#m.txt

