This application is made by Aryan Raj, Sayak Dutta and Sachin Giri.

TCP Hybla     ->    H1 ---+      +--- H4
            		  |      |
TCP Westwood+ ->    H2 ---R1 -- R2--- H5
            		  |      |
TCP YeAH-TCP  ->    H3 ---+      +--- H6

A Dumbbell topology with two routers R1 and R2 connected by a (10 Mbps, 50 ms) wired link.
Each of the routers is connected to 3 hosts
i.e., H1, H2 and H3 are connected to R1,
and H4, H5 and H6 are connected to R2.
All the hosts are attached to the routers with (100 Mbps, 20ms) links.
Both the routers use drop-tail queues with a equal queue size set according to bandwidth-delay product.
Senders (i.e. H1, H2 and H3) are attached with TCP Hybla, TCP Westwood+, and TCP YeAH-TCP agents respectively.
Packet size is 1.5 KB.

We simulated the computer network for the given application using the discrete event network simulator ns-3 and 
used ns-3’s Flow Monitor module to collect and store the performance data of network protocols from the simulation.

How to Build and Run:
(i) FIRST COPY proj4_a.cc and proj4_b.cc in ns-3.30.1/scratch folder
(ii) Then in terminal (with pwd (present working directory) at /ns-3.30.1), Enter command:
     For part 1: ./waf --run scratch/proj4_a --vis
     For part 2: ./waf --run scratch/proj4_b --vis
 
How to simulate: Click on simulate on the python monitor screen.

About the files that are created after the simulation completes:
.tp files: This stores Throughput (in Kbps) w.r.t time
.cwnd files: This stores Evolution of congestion window (in Bytes) w.r.t time
.gp files: This stores Goodput (in Kbps) w.r.t time
.congestion_loss files: This stores information about the flow i.e. congestion loss, total packets lost, packets successfully transferred, Maximum Throughput, etc.

