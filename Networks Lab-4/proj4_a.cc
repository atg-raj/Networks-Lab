#include <string>
#include <fstream>
#include <cstdlib>
#include <map>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/gnuplot.h"
#include "ns3/tcp-westwood.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/tcp-hybla.h"
#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-yeah.h"
#include "ns3/log.h"
#include "ns3/tcp-scalable.h"
#include "ns3/sequence-number.h"
#include "ns3/traced-value.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/enum.h"


using namespace ns3;
using namespace std;


NS_LOG_COMPONENT_DEFINE ("proj4");

map<uint32_t, uint32_t> DroppedPackets;  //data structure to store the id and count of each packet dropped
map<Address, double> TotalBytes;            //data structure to store total no of useful bytes sent
map<string, double> BytesReceivedIPV4, maximum;   //data structure to store total bytes


/* Setting up class for each application */
class APP: public Application
{

	public:
		APP();
		virtual ~APP();

		void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);
		void recv(int n);
	private:
		
		virtual void StartApplication(void);
		virtual void StopApplication(void);

		void ScheduleTx(void);
		void SendPacket(void);

		Ptr<Socket>     sock;
		Address         Peer;
		uint32_t PacketSize;
		uint32_t NPackets;
		DataRate        rate;
		EventId         SendEvent;
		bool            Running;
		uint32_t PacketsSent;
};

APP::APP()		//Default constructor to initialize the attributes
	: sock(0),
          Peer(),
          PacketSize(0),
          NPackets(0),
          rate(0),
          SendEvent(),
          Running(false),
          PacketsSent(0)
{
}
APP::~APP() 
{
 sock = 0;
}

void APP::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate) 
{
	/* Setting up the socket required to handle the application data */
	sock = socket;
	Peer = address;
	PacketSize = packetSize;
	NPackets = nPackets;
	rate = dataRate;
}

void APP::StartApplication() 	//Binds the socket and connects to the receiver using address stored in peer
{
	Running = true;
	PacketsSent = 0;
	sock->Bind();
	sock->Connect(Peer);
	SendPacket();
}

void APP::StopApplication() //stops the application and closes the socket
{
	Running = false;
	if(SendEvent.IsRunning())
		Simulator::Cancel(SendEvent);
	if(sock)
		sock->Close();
}

void APP::SendPacket()     //Function to send packets from the host to receiver after duration is calculated by ScheduleTx()
{
	Ptr<Packet> packet = Create<Packet>(PacketSize);
	sock->Send(packet);
	if(++PacketsSent < NPackets)
		ScheduleTx();
}

void APP::ScheduleTx()   //Schedules the next packet to be sent that is calculated from the packetsize and data rate
{
	if(Running) 
	{
		Time tNext(Seconds(PacketSize*8/static_cast<double>(rate.GetBitRate())));
		SendEvent = Simulator::Schedule(tNext, &APP::SendPacket, this);
	}
}

static void CwndUpdater(Ptr<OutputStreamWrapper> stream, double startTime,uint32_t oldCwnd, uint32_t newCwnd) //prints the updated congestion window size in bytes after each update
{
	*stream->GetStream() << Simulator::Now ().GetSeconds () - startTime << "\t" << newCwnd << std::endl;
}


void Throughput(Ptr<OutputStreamWrapper> stream, double startTime, std::string str, Ptr<const Packet> p, Ptr<Ipv4> ipv4, uint32_t interface) 
{
	/* Calculates the total no of bytes sent over a duration and calculates the throughput */
	double time = Simulator::Now().GetSeconds();

	if(BytesReceivedIPV4.find(str) == BytesReceivedIPV4.end())
		BytesReceivedIPV4[str] = 0;
	if(maximum.find(str) == maximum.end())
		maximum[str] = 0;
	BytesReceivedIPV4[str] += p->GetSize();
	double speed = (((BytesReceivedIPV4[str] * 8.0) / 1000)/(time-startTime));
	*stream->GetStream() << time-startTime << "\t" <<  speed<< std::endl;
	if(maximum[str] < speed)
		maximum[str] = speed;		//storing the maximum throughput achieved
	
}

static void packetDrop(Ptr<OutputStreamWrapper> stream, double startTime, uint32_t Id) 
{
	*stream->GetStream() << Simulator::Now ().GetSeconds () - startTime << "\t" << std::endl;
	if(DroppedPackets.find(Id) == DroppedPackets.end())	//keeps track of each packet that are dropped.
		DroppedPackets[Id] = 0;
	DroppedPackets[Id]++;
}

void goodput(Ptr<OutputStreamWrapper> stream, double startTime, string str, Ptr<const Packet> p, const Address& addr)
{
	/* Calculates the no of useful bytes sent from no of packets dropped till that point and total bytes sent and calculates the goodput */
	double time = Simulator::Now().GetSeconds();

	if(TotalBytes.find(addr) == TotalBytes.end())
		TotalBytes[addr] = 0;
	TotalBytes[addr] += p->GetSize();
	double speed = (((TotalBytes[addr] * 8.0) / 1000)/(time-startTime));
	*stream->GetStream() << time-startTime << "\t" <<  speed << std::endl;
}


Ptr<Socket> flow(Address sinkAddress, 
					uint32_t Port, 
					string tcp, 
					Ptr<Node> host, 
					Ptr<Node> sink, 
					double start, 
					double stop,
					uint32_t packetSize,
					uint32_t numPackets,
					string dataRate,
					double appStart,
					double appStop) {
/* General function to take the name of TCP protocol as function parameter and set up the corresponding TCP socket	*/
	if(tcp.compare("TcpYeah") == 0) {
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpYeah::GetTypeId()));
	} else if(tcp.compare("TcpWestwood") == 0) {
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));
	} else if(tcp.compare("TcpHybla") == 0) {
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
	} else {
		fprintf(stderr, "Invalid TCP version\n");   //Error function for handling TCP protocol other than the ones specified
		exit(EXIT_FAILURE);
	}
	PacketSinkHelper a("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), Port));
	ApplicationContainer b = a.Install(sink);
	b.Start(Seconds(start));
	b.Stop(Seconds(stop));

	Ptr<Socket> TcpSocket = Socket::CreateSocket(host, TcpSocketFactory::GetTypeId());
	

	Ptr<APP> app = CreateObject<APP>();			//creating the application process
	app->Setup(TcpSocket, sinkAddress, packetSize, numPackets, DataRate(dataRate));
	host->AddApplication(app);
	app->SetStartTime(Seconds(appStart));
	app->SetStopTime(Seconds(appStop));

	return TcpSocket;
}
/* Function for part A) */
void part_A() {
	std::cout << "Part A started..." << std::endl;
	std::string rateHR = "100Mbps";
	std::string latencyHR = "20ms";
	std::string rateRR = "10Mbps";
	std::string latencyRR = "50ms";

	uint32_t packetSize = 1.5*1024;		

	double errorP = 0.000001;

	PointToPointHelper p2pHR, p2pRR;	//variables to set up point to point links with specified characteristics

/* Host to router links	*/
	p2pHR.SetDeviceAttribute("DataRate", StringValue(rateHR));
	p2pHR.SetChannelAttribute("Delay", StringValue(latencyHR));
	p2pHR.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue(QueueSize("163p")));  //queue size set according to bandwidth delay product(i.e bandwidth(in bps) *delay/packet size(in bits))

/* Router to Router LInks	*/
	p2pRR.SetDeviceAttribute("DataRate", StringValue(rateRR));
	p2pRR.SetChannelAttribute("Delay", StringValue(latencyRR));
	p2pRR.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue(QueueSize("41p")));

	Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel> ("ErrorRate", DoubleValue (errorP));

	NodeContainer routers, senders, receivers;
	
	routers.Create(2);
	senders.Create(3);
	receivers.Create(3);



	NetDeviceContainer routerDevices = p2pRR.Install(routers);
	NetDeviceContainer leftRouterDevices, rightRouterDevices, senderDevices, receiverDevices;

	//Adding links
	std::cout << "Adding links" << std::endl;
	for(uint32_t i = 0; i < 3; ++i) {
		NetDeviceContainer cleft = p2pHR.Install(routers.Get(0), senders.Get(i));
		leftRouterDevices.Add(cleft.Get(0));
		senderDevices.Add(cleft.Get(1));
		cleft.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));

		NetDeviceContainer cright = p2pHR.Install(routers.Get(1), receivers.Get(i));
		rightRouterDevices.Add(cright.Get(0));
		receiverDevices.Add(cright.Get(1));
		cright.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
	}

	//Install Internet Stack
	/*
		For each node in the input container, aggregate implementations of 
		the ns3::Ipv4, ns3::Ipv6, ns3::Udp, and, ns3::Tcp classes. 
	*/
	std::cout << "Install internet stack" << std::endl;
	InternetStackHelper stack;
	stack.Install(routers);
	stack.Install(senders);
	stack.Install(receivers);
	//Adding IP addresses
	std::cout << "Adding IP addresses" << std::endl;
	Ipv4AddressHelper routerIP = Ipv4AddressHelper("10.3.0.0", "255.255.255.0");	//(network, mask)
	Ipv4AddressHelper senderIP = Ipv4AddressHelper("10.1.0.0", "255.255.255.0");
	Ipv4AddressHelper receiverIP = Ipv4AddressHelper("10.2.0.0", "255.255.255.0");
	

	Ipv4InterfaceContainer routerIFC, senderIFCs, receiverIFCs, leftRouterIFCs, rightRouterIFCs;

	//Assign IP addresses to the net devices specified in the container 
	//based on the current network prefix and address base
	routerIFC = routerIP.Assign(routerDevices);

	for(uint32_t i = 0; i < 3; ++i) {
		NetDeviceContainer senderDevice;
		senderDevice.Add(senderDevices.Get(i));
		senderDevice.Add(leftRouterDevices.Get(i));
		Ipv4InterfaceContainer senderIFC = senderIP.Assign(senderDevice);
		senderIFCs.Add(senderIFC.Get(0));
		leftRouterIFCs.Add(senderIFC.Get(1));
		//Increment the network number and reset the IP address counter 
		//to the base value provided in the SetBase method.
		senderIP.NewNetwork();

		NetDeviceContainer receiverDevice;
		receiverDevice.Add(receiverDevices.Get(i));
		receiverDevice.Add(rightRouterDevices.Get(i));
		Ipv4InterfaceContainer receiverIFC = receiverIP.Assign(receiverDevice);
		receiverIFCs.Add(receiverIFC.Get(0));
		rightRouterIFCs.Add(receiverIFC.Get(1));
		receiverIP.NewNetwork();
	}

	/*
		Measuring Performance of each TCP variant
	*/

	std::cout << "Measuring Performance of each TCP variant..." << std::endl;
	
	double durationGap = 500;
	double netDuration = 0;
	uint32_t port = 9000;
	uint32_t numPackets = 1000000;
	string transferSpeed = "50Mbps";	

	//TCP Hybla from H1 to H4
	AsciiTraceHelper asciiTraceHelper;
	Ptr<OutputStreamWrapper> stream1CWND = asciiTraceHelper.CreateFileStream("application_1_h1_h4_a.cwnd");
	Ptr<OutputStreamWrapper> stream1PD = asciiTraceHelper.CreateFileStream("application_1_h1_h4_a.congestion_loss");
	Ptr<OutputStreamWrapper> stream1TP = asciiTraceHelper.CreateFileStream("application_1_h1_h4_a.tp");
	Ptr<OutputStreamWrapper> stream1GP = asciiTraceHelper.CreateFileStream("application_1_h1_h4_a.gp");
	Ptr<Socket> ns3TcpSocket1 = flow(InetSocketAddress(receiverIFCs.GetAddress(0), port), port, "TcpHybla", senders.Get(0), receivers.Get(0), netDuration, netDuration+durationGap, packetSize, numPackets, transferSpeed, netDuration, netDuration+durationGap);

	ns3TcpSocket1->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback (&CwndUpdater, stream1CWND, netDuration));
	ns3TcpSocket1->TraceConnectWithoutContext("Drop", MakeBoundCallback (&packetDrop, stream1PD, netDuration, 1));
	
	// Measure PacketSinks
	string sink = "/NodeList/5/ApplicationList/0/$ns3::PacketSink/Rx";
	Config::Connect(sink, MakeBoundCallback(&goodput, stream1GP, netDuration));

	std::string sink_ = "/NodeList/5/$ns3::Ipv4L3Protocol/Rx";
	Config::Connect(sink_, MakeBoundCallback(&Throughput, stream1TP, netDuration));

	netDuration += durationGap;

	
	//TCP Westwood from H2 to H5
	Ptr<OutputStreamWrapper> stream2CWND = asciiTraceHelper.CreateFileStream("application_1_h2_h5_a.cwnd");
	Ptr<OutputStreamWrapper> stream2PD = asciiTraceHelper.CreateFileStream("application_1_h2_h5_a.congestion_loss");
	Ptr<OutputStreamWrapper> stream2TP = asciiTraceHelper.CreateFileStream("application_1_h2_h5_a.tp");
	Ptr<OutputStreamWrapper> stream2GP = asciiTraceHelper.CreateFileStream("application_1_h2_h5_a.gp");
	Ptr<Socket> ns3TcpSocket2 = flow(InetSocketAddress(receiverIFCs.GetAddress(1), port), port, "TcpWestwood", senders.Get(1), receivers.Get(1), netDuration, netDuration+durationGap, packetSize, numPackets, transferSpeed, netDuration, netDuration+durationGap);
	ns3TcpSocket2->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback (&CwndUpdater, stream2CWND, netDuration));
	ns3TcpSocket2->TraceConnectWithoutContext("Drop", MakeBoundCallback (&packetDrop, stream2PD, netDuration, 2));

	sink = "/NodeList/6/ApplicationList/0/$ns3::PacketSink/Rx";
	Config::Connect(sink, MakeBoundCallback(&goodput, stream2GP, netDuration));
	sink_ = "/NodeList/6/$ns3::Ipv4L3Protocol/Rx";
	Config::Connect(sink_, MakeBoundCallback(&Throughput, stream2TP, netDuration));
	netDuration += durationGap;

	//TCP Yeah from H3 to H6
	Ptr<OutputStreamWrapper> stream3CWND = asciiTraceHelper.CreateFileStream("application_1_h3_h6_a.cwnd");
	Ptr<OutputStreamWrapper> stream3PD = asciiTraceHelper.CreateFileStream("application_1_h3_h6_a.congestion_loss");
	Ptr<OutputStreamWrapper> stream3TP = asciiTraceHelper.CreateFileStream("application_1_h3_h6_a.tp");
	Ptr<OutputStreamWrapper> stream3GP = asciiTraceHelper.CreateFileStream("application_1_h3_h6_a.gp");
	Ptr<Socket> ns3TcpSocket3 = flow(InetSocketAddress(receiverIFCs.GetAddress(2), port), port, "TcpYeah", senders.Get(2), receivers.Get(2), netDuration, netDuration+durationGap, packetSize, numPackets, transferSpeed, netDuration, netDuration+durationGap);
	ns3TcpSocket3->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback (&CwndUpdater, stream3CWND, netDuration));
	ns3TcpSocket3->TraceConnectWithoutContext("Drop", MakeBoundCallback (&packetDrop, stream3PD, netDuration, 3));

	sink = "/NodeList/7/ApplicationList/0/$ns3::PacketSink/Rx";
	Config::Connect(sink, MakeBoundCallback(&goodput, stream3GP, netDuration));
	sink_ = "/NodeList/7/$ns3::Ipv4L3Protocol/Rx";
	Config::Connect(sink_, MakeBoundCallback(&Throughput, stream3TP, netDuration));
	netDuration += durationGap;

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
	
	Ptr<FlowMonitor> flowmon;
	FlowMonitorHelper flowmonHelper;
	flowmon = flowmonHelper.InstallAll();
	Simulator::Stop(Seconds(netDuration));
	Simulator::Run();
	flowmon->CheckForLostPackets();
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
	std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
		if(t.sourceAddress == "10.1.0.1") {
			if(DroppedPackets.find(1)==DroppedPackets.end())
				DroppedPackets[1] = 0;
			*stream1PD->GetStream() << "TCP Hybla flow "<<endl;
			*stream1PD->GetStream() << "Total Packets transmitted: "<<(std::to_string( numPackets )).c_str()<<"\n";
			*stream1PD->GetStream()  << "Net Packet Lost: " << i->second.lostPackets << "\n";
			*stream1PD->GetStream()  << "Packets Lost due to buffer overflow: " << DroppedPackets[1] << "\n";
			*stream1PD->GetStream()  << "Packets Lost due to Congestion: " << i->second.lostPackets - DroppedPackets[1] << "\n";
			*stream1PD->GetStream() << "Maximum Throughput(in kbps): " <<maximum["/NodeList/5/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
			*stream1PD->GetStream() << "Packets successfully transferred: "<<(std::to_string(  numPackets- i->second.lostPackets )).c_str()<<"\n";
			*stream1PD->GetStream() << "Percentage of packet lost total: "<<(std::to_string( double(i->second.lostPackets*100)/double(numPackets) )).c_str()<<"\n";
			*stream1PD->GetStream() << "Percentage of packet lost(buffer overflow): "<<(std::to_string(  double(DroppedPackets[1]*100)/double(numPackets))).c_str()<<"\n";
			*stream1PD->GetStream() << "Percentage of packet lost(due to congestion): "<<(std::to_string( double((i->second.lostPackets - DroppedPackets[1])*100)/double(numPackets))).c_str()<<"\n";
		} else if(t.sourceAddress == "10.1.1.1") {
			if(DroppedPackets.find(2)==DroppedPackets.end())
				DroppedPackets[2] = 0;
			*stream2PD->GetStream() << "TCP Westwood flow "<<endl;
			*stream2PD->GetStream() << "Total Packets transmitted: "<<(std::to_string( numPackets )).c_str()<<"\n";
			*stream2PD->GetStream()  << "Net Packet Lost: " << i->second.lostPackets << "\n";
			*stream2PD->GetStream()  << "Packets Lost due to buffer overflow: " << DroppedPackets[2] << "\n";
			*stream2PD->GetStream()  << "Packets Lost due to Congestion: " << i->second.lostPackets - DroppedPackets[2] << "\n";
			*stream2PD->GetStream() << "Maximum Throughput(in kbps): " <<maximum["/NodeList/6/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
			*stream2PD->GetStream() << "Packets successfully transferred: "<<(std::to_string(  numPackets- i->second.lostPackets )).c_str()<<"\n";
			*stream2PD->GetStream() << "Percentage of packet lost total: "<<(std::to_string( double(i->second.lostPackets*100)/double(numPackets) )).c_str()<<"\n";
			*stream2PD->GetStream() << "Percentage of packet lost(buffer overflow): "<<(std::to_string(  double(DroppedPackets[2]*100)/double(numPackets))).c_str()<<"\n";
			*stream2PD->GetStream() << "Percentage of packet lost(due to congestion): "<<(std::to_string( double((i->second.lostPackets - DroppedPackets[2])*100)/double(numPackets))).c_str()<<"\n";
		} else if(t.sourceAddress == "10.1.2.1") {
			if(DroppedPackets.find(3)==DroppedPackets.end())
				DroppedPackets[3] = 0;
			*stream3PD->GetStream() << "TCP Yeah flow "<<endl;
			*stream3PD->GetStream() << "Total Packets transmitted: "<<(std::to_string( numPackets )).c_str()<<"\n";
			*stream3PD->GetStream()  << "Net Packet Lost: " << i->second.lostPackets << "\n";
			*stream3PD->GetStream()  << "Packets Lost due to buffer overflow: " << DroppedPackets[3] << "\n";
			*stream3PD->GetStream()  << "Packets Lost due to Congestion: " << i->second.lostPackets - DroppedPackets[3] << "\n";
			*stream3PD->GetStream() << "Maximum Throughput(in kbps): " <<maximum["/NodeList/7/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
			*stream3PD->GetStream() << "Packets successfully transferred: "<<(std::to_string(  numPackets- i->second.lostPackets )).c_str()<<"\n";
			*stream3PD->GetStream() << "Percentage of packet lost total: "<<(std::to_string( double(i->second.lostPackets*100)/double(numPackets) )).c_str()<<"\n";
			*stream3PD->GetStream() << "Percentage of packet lost(buffer overflow): "<<(std::to_string(  double(DroppedPackets[3]*100)/double(numPackets))).c_str()<<"\n";
			*stream3PD->GetStream() << "Percentage of packet lost(due to congestion): "<<(std::to_string( double((i->second.lostPackets - DroppedPackets[3])*100)/double(numPackets))).c_str()<<"\n";
		}
	}

	std::cout << "Simulation finished" << std::endl;
	Simulator::Destroy();
}




int main(int argc,char *argv[]) 
{
		CommandLine cmd;
  		cmd.Parse (argc, argv);
		part_A();
}
