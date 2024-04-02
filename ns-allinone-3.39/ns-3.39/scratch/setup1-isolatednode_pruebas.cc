
/*
 * This is a basic example that compares CoDel and PfifoFast queues using a simple, single-flow
 * topology:
 *
 * source_{1-N} --------------------------Router_marking--------------------------QoS_router------------------------DU_sink
 *                   inf Mb/s, 0 ms                          inf Mb/s, 0 ms                       5 Mb/s, 5ms
 *                                                                                                 bottleneck
 *
 * The source generates traffic across the network using BulkSendApplication.
 * The default TCP version in ns-3, TcpNewReno, is used as the transport-layer protocol.
 * Packets transmitted during a simulation run are captured into a .pcap file, and
 * congestion window values are also traced.
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/enum.h"
#include "ns3/error-model.h"
#include "ns3/event-id.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-helper.h"

#include "logs.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("setup1");



class RxTracerHelper {
    public:
        RxTracerHelper(int num, const std::string& filename) : num(num), filename(filename) {
            RxFile.open(filename + "RxFile" + std::to_string(num) + ".log", std::fstream::out);
        }

        ~RxTracerHelper() {
            RxFile.close(); // Close the file in the destructor
        }

        void RxTracer(Ptr<const Packet> pkt, const Address &) {
            RxFile << contRX << " " << Simulator::Now().GetSeconds() << " " << pkt->GetSize() << std::endl;
            contRX++;
            // Don't close the file here; it will be automatically closed in the destructor
        }


    private:
        std::ofstream RxFile;
        int num;
        std::string filename;
        uint32_t contRX = 0;
};


class TxTracerHelper {
    public:
        TxTracerHelper(int num, const std::string& filename) : num(num), filename(filename) {
            TxFile.open(filename + "TxFile" + std::to_string(num) + ".log", std::fstream::out);
        }

        ~TxTracerHelper() {
            TxFile.close(); // Close the file in the destructor
        }

        void TxTracer(Ptr<const Packet> pkt) {
            TxFile << contTX << " " << Simulator::Now().GetSeconds() << " " << pkt->GetSize() << std::endl;
            contTX++;
            // Don't close the file here; it will be automatically closed in the destructor
        }


    private:
        std::ofstream TxFile;
        int num;
        std::string filename;
        uint32_t contTX = 0;
};




int
main(int argc, char* argv[])
{
    RngSeedManager::SetSeed(time(NULL));
    uint32_t pktsize = 1458;
    TxTracerHelper txTracerHelper1(1, "./sim_results/"), txTracerHelper2(2, "./sim_results/"), 
                   txTracerHelper3(3, "./sim_results/"), txTracerHelper4(4, "./sim_results/");

    RxTracerHelper rxTracerHelper1(1, "./sim_results/"), rxTracerHelper2(2, "./sim_results/"),
                   rxTracerHelper3(3, "./sim_results/"), rxTracerHelper4(4, "./sim_results/");
   
    LogComponentEnable("WdrrQueueDisc", LOG_LEVEL_DEBUG);
    // Config::SetDefault("ns3::UdpSocket::SegmentSize", UintegerValue(pktsize));
    
    NodeContainer nodes;
    int RUnodes = 3;
    nodes.Create(RUnodes + 3);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("0ms"));
    pointToPoint.SetDeviceAttribute("Mtu", UintegerValue(1500));
    // pointToPoint.SetDeviceAttribute("Mtu", UintegerValue(1460));
    // pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", ns3::QueueSizeValue(QueueSize(QueueSizeUnit::BYTES, (0))));
    std::vector<NetDeviceContainer> Devs;
    for (int RUnodes_i=0; RUnodes_i < RUnodes; RUnodes_i++){
        NetDeviceContainer p2pDev = pointToPoint.Install(NodeContainer{nodes.Get(RUnodes_i), nodes.Get(RUnodes)});
        Devs.push_back(p2pDev);
    }

    NetDeviceContainer p2p_mark_qos = pointToPoint.Install(NodeContainer{nodes.Get(RUnodes), nodes.Get(RUnodes + 1)});


    PointToPointHelper pointToPoint_bottleneck;
    pointToPoint_bottleneck.SetDeviceAttribute("DataRate", StringValue("50Mbps")); // BW
    pointToPoint_bottleneck.SetChannelAttribute("Delay", StringValue("0ms"));        // delay
    pointToPoint_bottleneck.SetDeviceAttribute("Mtu", UintegerValue(1500));
    // pointToPoint_bottleneck.SetQueue("ns3::DropTailQueue", "MaxSize", ns3::QueueSizeValue(QueueSize(QueueSizeUnit::BYTES, (0))));
    NetDeviceContainer p2p_qos_du = pointToPoint_bottleneck.Install(NodeContainer{nodes.Get(RUnodes + 1), nodes.Get(RUnodes + 2)});


    InternetStackHelper stack;
    for (int i = 0; i < RUnodes + 3; i++)
    {
        stack.Install(nodes.Get(i));
    }

    TrafficControlHelper tch1;
    tch1.SetRootQueueDisc("ns3::MarkerQueueDisc");
    tch1.Install(p2p_mark_qos.Get(0));

    TrafficControlHelper tch2;
    // Set up the root queue disc with PrioQueueDisc
    uint16_t rootHandle = tch2.SetRootQueueDisc("ns3::PrioQueueDscpDisc");
    // Get ClassIdList for the second-level queues
    TrafficControlHelper::ClassIdList cid = tch2.AddQueueDiscClasses(rootHandle, 2, "ns3::QueueDiscClass");
    tch2.AddChildQueueDisc(rootHandle, cid[0], "ns3::FifoQueueDisc");
    tch2.AddChildQueueDisc(rootHandle, cid[1], "ns3::WdrrQueueDisc");
    //tch2.SetRootQueueDisc("ns3::WrrQueueDisc", "Quantum", StringValue("1000 3000"));
    tch2.Install(p2p_qos_du.Get(0));

    std::string str;
    std::vector<Ipv4InterfaceContainer> ifacesP2PRU_gateway;
    for (int i = 0; i < RUnodes; i++)
    {
        Ipv4AddressHelper addressesP2p;
        str = "11." + std::to_string(i + 1) + ".0.0";
        const char *adr = str.c_str();
        // NS_LOG_INFO(MAGENTA << "INFO: " << RESET << "Creating P2P Network " << i + 1 << ": " << str);
        addressesP2p.SetBase(adr, "255.255.255.0");
        Ipv4InterfaceContainer ifacP2p = addressesP2p.Assign(Devs.at(i)); // Assign fuction returns a Ipv4InterfaceConatiner
        ifacesP2PRU_gateway.push_back(ifacP2p);

        // NS_LOG_INFO(MAGENTA << "INFO: " << RESET << "Ifaces P2P are " << ifacesP2PRU_gateway.GetN());
    }

    Ipv4AddressHelper addressesP2p_marker_router;
    str = "11." + std::to_string(RUnodes + 1) + ".0.0";
    const char *adr1 = str.c_str();
    addressesP2p_marker_router.SetBase(adr1, "255.255.255.0");
    Ipv4InterfaceContainer ifacP2P_Marker_QoS = addressesP2p_marker_router.Assign(p2p_mark_qos); // Assign fuction returns a Ipv4InterfaceConatiner

    Ipv4AddressHelper addressesP2p_router_DU;
    str = "11." + std::to_string(RUnodes + 2) + ".0.0";
    const char *adr2 = str.c_str();
    addressesP2p_router_DU.SetBase(adr2, "255.255.255.0");
    Ipv4InterfaceContainer ifacP2P_Router_DU = addressesP2p_router_DU.Assign(p2p_qos_du); // Assign fuction returns a Ipv4InterfaceConatiner
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    

    // int tos[4] = {0x28,0x50,0x70,0xB8}; //in case, i would like to mark each flow with a diffserv code point
        
    ApplicationContainer Client_app, Server_app;
    int port = 8080; 
    for (int i = 0; i < RUnodes; i++){
        // Create a packet sink on the end of the chain
        InetSocketAddress Server_Address(InetSocketAddress(ifacP2P_Router_DU.GetAddress(1,0), port + i));
        // Server_Address.SetTos(tos[i]);
        PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", Address(Server_Address));
        Server_app.Add(packetSinkHelper.Install(nodes.Get(RUnodes + 2))); // Last node server
        // Create the OnOff APP to send TCP to the server
        OnOffHelper clientHelper("ns3::UdpSocketFactory", Address(Server_Address));
        clientHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
        clientHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
        clientHelper.SetAttribute("DataRate", StringValue("80Mbps"));
        clientHelper.SetAttribute("MaxBytes", UintegerValue(1e6));
        clientHelper.SetAttribute("PacketSize", UintegerValue(pktsize));
        Client_app.Add(clientHelper.Install(NodeContainer{nodes.Get(i)}));

    }

    
    Server_app.Start(Seconds(0));
    Client_app.Start(Seconds(0));
    NS_LOG_INFO("Simulation starts here!");
    // pointToPoint.EnablePcap ("./sim_results/second", p2p_mark_qos.Get(1), 1);
    NS_LOG_INFO(MAGENTA << "INFO: " << RESET << "Conf-scenario finished");
    //   Use the member function of the class as the callback
    Config::ConnectWithoutContext("/NodeList/0/ApplicationList/0/$ns3::OnOffApplication/Tx",
                                  MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper1));
    Config::ConnectWithoutContext("/NodeList/1/ApplicationList/0/$ns3::OnOffApplication/Tx",
                                  MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper2));
    Config::ConnectWithoutContext("/NodeList/2/ApplicationList/0/$ns3::OnOffApplication/Tx",
                                  MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper3));
    // Config::ConnectWithoutContext("/NodeList/3/ApplicationList/0/$ns3::OnOffApplication/Tx",
    //                               MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper4));

    Config::ConnectWithoutContext("/NodeList/" + std::to_string(RUnodes + 2) + "/ApplicationList/0/$ns3::PacketSink/Rx", 
                                MakeCallback(&RxTracerHelper::RxTracer, &rxTracerHelper1));
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(RUnodes + 2) + "/ApplicationList/1/$ns3::PacketSink/Rx", 
                                MakeCallback(&RxTracerHelper::RxTracer, &rxTracerHelper2));
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(RUnodes + 2) + "/ApplicationList/2/$ns3::PacketSink/Rx", 
                                MakeCallback(&RxTracerHelper::RxTracer, &rxTracerHelper3));
    // Config::ConnectWithoutContext("/NodeList/" + std::to_string(RUnodes + 2) + "/ApplicationList/3/$ns3::PacketSink/Rx", 
    //                             MakeCallback(&RxTracerHelper::RxTracer, &rxTracerHelper4));

    // Config::ConnectWithoutContext("/NodeList/" + std::to_string(RUnodes + 3) + "/ApplicationList/1/$ns3::PacketSink/Rx", 
    //                               MakeCallback(&RxTracerHelper::RxTracer, &rxTracerHelper2));

    // Config::ConnectWithoutContext("/NodeList/" + std::to_string(RUnodes + 3) + "/ApplicationList/2/$ns3::PacketSink/Rx", 
    //                             MakeCallback(&RxTracerHelper::RxTracer, &rxTracerHelper2));


    pointToPoint.EnablePcap("./sim_results/sched.pcap", nodes, true);
    // Flow monitor
    // Ptr<FlowMonitor> flowMonitor;
    // FlowMonitorHelper flowHelper;
    // flowMonitor = flowHelper.InstallAll();
    
    // Ptr<PacketSink> Server_trace = StaticCast<PacketSink>(sinkApps.Get(0));
    Simulator::Stop(Seconds(3000));
    Simulator::Run();
    Simulator::Destroy();
    // std::cout << MAGENTA << "INFO: " << RESET << "Sink: Total RX - " << Server_trace->GetTotalRx() << " bytes" << std::endl;
    return 0;
}
 