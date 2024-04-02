
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
    #include "json.hpp"

    #include "logs.h"

    #include <fstream>
    #include <iostream>
    #include <string>
    #include <vector>


    using namespace ns3;
    using json = nlohmann::json;
    
    NS_LOG_COMPONENT_DEFINE("setupjuniper");



    class RxTracerHelper {
    public:
        RxTracerHelper(int num, const std::string& filename) : num(num), filename(filename) {
            RxFile.open(filename + "RxFile" + std::to_string(num) + ".log", std::fstream::out);
        }

        ~RxTracerHelper() {
            RxFile.close(); // Close the file in the destructor
        }

        void RxTracer(Ptr<const Packet> pkt, const Address &) {
            RxFile << pkt->GetUid() << " " << Simulator::Now().GetSeconds() << " " << pkt->GetSize() << std::endl;
           
            // Don't close the file here; it will be automatically closed in the destructor
        }


    private:
        std::ofstream RxFile;
        int num;
        std::string filename;
      
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
            TxFile << pkt->GetUid() << " " << Simulator::Now().GetSeconds() << " " << pkt->GetSize() <<std::endl;
            
            // Don't close the file here; it will be automatically closed in the destructor
        }


    private:
        std::ofstream TxFile;
        int num;
        std::string filename;
       
    };

    // Function to print total received bytes
    void PrintTotalRx(Ptr<PacketSink> serverSink) {
        std::cout << MAGENTA << "INFO: " << RESET << "Sink: Total RX - " << serverSink->GetTotalRx() << " bytes" << std::endl;
        Simulator::Schedule(Seconds(Simulator::Now().GetSeconds()+2), &PrintTotalRx, serverSink);
    }


    int
    main(int argc, char* argv[])
    {
        RngSeedManager::SetSeed(time(NULL));
        std::string JSONpath = "./scratch/scen.json";
        std::ifstream f(JSONpath);
        json data = json::parse(f);
        uint32_t netMTU = 1500, pktsize = 1000; 
        bool enabletracing = true, enablepcap = false, enableRTtraffic = true, enablehqos = true;
        int Routers = 10;
   

        // LogComponentEnable("WfqQueueDisc", LOG_LEVEL_INFO);
        // LogComponentEnable("PrioQueueDscpDisc", LOG_LEVEL_INFO);
        // Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(pktsize));
        TxTracerHelper txTracerHelper1(1, "./sim_results/"), txTracerHelper2(2, "./sim_results/"), 
                        txTracerHelper3(3, "./sim_results/"), txTracerHelper4(4, "./sim_results/"),  
                        txTracerHelper5(5, "./sim_results/"), txTracerHelper6(6, "./sim_results/") ;

        RxTracerHelper rxTracerHelper1(1, "./sim_results/"), rxTracerHelper2(2, "./sim_results/"),
                        rxTracerHelper3(3, "./sim_results/"), rxTracerHelper4(4, "./sim_results/"),
                        rxTracerHelper5(5, "./sim_results/"), rxTracerHelper6(6, "./sim_results/");
   
        
        NodeContainer nodes;
        nodes.Create(Routers);

    
        /************************************************
        ************ Creating links features ************
        *************************************************/

        PointToPointHelper p2p;
        p2p.SetDeviceAttribute("DataRate", StringValue("100Gbps"));
        p2p.SetChannelAttribute("Delay", StringValue("0ms"));
        p2p.SetDeviceAttribute("Mtu", UintegerValue(netMTU));


        PointToPointHelper Access;
        Access.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
        Access.SetChannelAttribute("Delay", StringValue("0ms"));
        Access.SetDeviceAttribute("Mtu", UintegerValue(netMTU));
       
        // Consider R4 as a marker node from a RU
        NetDeviceContainer R3R0 = p2p.Install(NodeContainer{nodes.Get(0), nodes.Get(1)});
        NetDeviceContainer R3R8 = p2p.Install(NodeContainer{nodes.Get(0), nodes.Get(3)}); 
        NetDeviceContainer R3R9 = p2p.Install(NodeContainer{nodes.Get(0), nodes.Get(4)}); 

        NetDeviceContainer R0R4 = p2p.Install(NodeContainer{nodes.Get(1), nodes.Get(2)}); 
        NetDeviceContainer R0R8 = p2p.Install(NodeContainer{nodes.Get(1), nodes.Get(3)});
        NetDeviceContainer R0R9 = p2p.Install(NodeContainer{nodes.Get(1), nodes.Get(4)}); 

        NetDeviceContainer R4R8 = p2p.Install(NodeContainer{nodes.Get(2), nodes.Get(3)}); 
        NetDeviceContainer R4R9 = p2p.Install(NodeContainer{nodes.Get(2), nodes.Get(4)});

        NetDeviceContainer R8R9 = p2p.Install(NodeContainer{nodes.Get(3), nodes.Get(4)});

        NetDeviceContainer R8Du = Access.Install(NodeContainer{nodes.Get(3), nodes.Get(5)});
        NetDeviceContainer R9Du = Access.Install(NodeContainer{nodes.Get(4), nodes.Get(5)});


        NetDeviceContainer RT11R3 = Access.Install(NodeContainer{nodes.Get(6), nodes.Get(0)});
        NetDeviceContainer R8RT12 = Access.Install(NodeContainer{nodes.Get(3), nodes.Get(7)});


        NetDeviceContainer RT11R0 = Access.Install(NodeContainer{nodes.Get(8), nodes.Get(1)});
        NetDeviceContainer R9RT12 = Access.Install(NodeContainer{nodes.Get(4), nodes.Get(9)});
        // NetDeviceContainer Ru_links;
        // for (int RUnodes_i=0; RUnodes_i < Ru_flows; RUnodes_i++){
        //     NetDeviceContainer p2pDev = pointToPoint.Install(NodeContainer{nodes.Get(RUnodes_i), nodes.Get(RUnodes)});
        //     Devs.push_back(p2pDev);
        // }

        /************************************************
        ******************* IP stack ********************
        *************************************************/

        InternetStackHelper stack;
        for (int i = 0; i < Routers; i++)
        {
            stack.Install(nodes.Get(i));
        }


        /************************************************
        *************** TCL - policies ******************
        *************************************************/



        if(enablehqos){

            //// MARKING 
            TrafficControlHelper tch1;
            tch1.SetRootQueueDisc("ns3::MarkerQueueDisc", "MarkingQueue", StringValue("8080 46 8081 8 8082 16 8083 24 8084 32"));
            // tch1.Install(R0R4.Get(0));
            tch1.Install(R4R8.Get(0));
            tch1.Install(R4R9.Get(0));

            //// Policies
            TrafficControlHelper tch2;
            // Set up the root queue disc with PrioQueueDisc
            uint16_t rootHandle = tch2.SetRootQueueDisc("ns3::PrioQueueDscpDisc");
            // Get ClassIdList for the second-level queues
            TrafficControlHelper::ClassIdList cid = tch2.AddQueueDiscClasses(rootHandle, 2, "ns3::QueueDiscClass");
            tch2.AddChildQueueDisc(rootHandle, cid[0], "ns3::FifoQueueDisc");
            // tch2.AddChildQueueDisc(rootHandle, cid[1], "ns3::WfqQueueDisc", "Quantum", StringValue("90 10 10"), "MapQueue", StringValue("8 0 16 1 24 2"));
            tch2.AddChildQueueDisc(rootHandle, cid[1], "ns3::WrrQueueDisc", "Quantum", StringValue("9 1 1"), "MapQueue", StringValue("8 0 16 1 24 2"));

            tch2.Install(R8Du.Get(0));
            // tch2.Install(R9Du.Get(0));
        }
        

        // tch2.Install(R8RT12.Get(1));
        /*******************************************************************
        ************** Creating networks from spine leaf nodes *************
        ********************************************************************/
        Ipv4InterfaceContainer Spine_1, Spine_2;
        Ipv4AddressHelper Spine1address;
        Spine1address.SetBase("10.0.38.0", "255.255.255.252");
        Spine_1.Add(Spine1address.Assign(R3R8)); // Assign fuction returns a Ipv4InterfaceConatiner
        Spine1address.SetBase("10.0.8.0", "255.255.255.252");
        Spine_1.Add(Spine1address.Assign(R0R8));
        Spine1address.SetBase("10.0.48.0", "255.255.255.252");
        Spine_1.Add(Spine1address.Assign(R4R8));
        Spine1address.SetBase("10.0.30.0", "255.255.255.252");
        Spine_1.Add(Spine1address.Assign(R3R0));
        Spine1address.SetBase("10.0.33.0", "255.255.255.252");
        Spine_1.Add(Spine1address.Assign(RT11R3));
        Spine1address.SetBase("10.0.88.0", "255.255.255.252");
        Spine_1.Add(Spine1address.Assign(R8RT12));


        Ipv4AddressHelper Spine2address;
        Spine2address.SetBase("10.0.39.0", "255.255.255.252");
        Spine_2.Add(Spine2address.Assign(R3R9)); // Assign fuction returns a Ipv4InterfaceConatiner
        Spine2address.SetBase("10.0.9.0", "255.255.255.252");
        Spine_2.Add(Spine2address.Assign(R0R9)); // Assign fuction returns a Ipv4InterfaceConatiner 
        Spine2address.SetBase("10.0.49.0", "255.255.255.252");
        Spine_2.Add(Spine2address.Assign(R4R9)); // Assign fuction returns a Ipv4InterfaceConatiner
        Spine2address.SetBase("10.0.4.0", "255.255.255.252");
        Spine_2.Add(Spine2address.Assign(R0R4));
        Spine2address.SetBase("10.0.11.0", "255.255.255.252");
        Spine_2.Add(Spine2address.Assign(RT11R0));
        Spine2address.SetBase("10.0.99.0", "255.255.255.252");
        Spine_2.Add(Spine2address.Assign(R9RT12));

    
        
        /*******************************************************************
        ********************** DU node configuration ***********************
        ********************************************************************/
        Ipv4InterfaceContainer Du_net;
        Ipv4AddressHelper Duaddress;
        Duaddress.SetBase("128.192.8.0", "255.255.255.0");
        Du_net.Add(Duaddress.Assign(R8Du));
        Duaddress.SetBase("128.192.9.0", "255.255.255.0");
        Du_net.Add(Duaddress.Assign(R9Du));
        Duaddress.SetBase("128.192.89.0", "255.255.255.0");
        Du_net.Add(Duaddress.Assign(R8R9));


        /*********************************************************************
        ********************** Populate Routing Tables ***********************
        **********************************************************************/
        // Ipv4GlobalRoutingHelper::PopulateRoutingTables();
        Ipv4StaticRoutingHelper ipv4RoutingHelper;
        Ptr<Ipv4StaticRouting> staticRoutingR4 = ipv4RoutingHelper.GetStaticRouting (nodes.Get(2)->GetObject<Ipv4> ());
        staticRoutingR4->AddNetworkRouteTo(Ipv4Address("128.192.8.0"), Ipv4Mask("255.255.255.0"), Spine_1.GetAddress(5,0), 1, 0);
        staticRoutingR4->AddNetworkRouteTo(Ipv4Address("128.192.89.0"), Ipv4Mask("255.255.255.0"), Spine_1.GetAddress(5,0), 1, 0);
        staticRoutingR4->AddNetworkRouteTo(Ipv4Address("10.0.88.0"), Ipv4Mask("255.255.255.252"), Spine_1.GetAddress(5,0), 1, 0);
        staticRoutingR4->AddNetworkRouteTo(Ipv4Address("10.0.99.0"), Ipv4Mask("255.255.255.252"), Spine_1.GetAddress(5,0), 1, 0);
        
        Ptr<Ipv4StaticRouting> staticRoutingR3 = ipv4RoutingHelper.GetStaticRouting (nodes.Get(0)->GetObject<Ipv4> ());
        staticRoutingR3->AddNetworkRouteTo(Ipv4Address("128.192.89.0"), Ipv4Mask("255.255.255.0"), Spine_1.GetAddress(7,0), 2, 0);
        staticRoutingR3->AddNetworkRouteTo(Ipv4Address("10.0.88.0"), Ipv4Mask("255.255.255.252"), Spine_1.GetAddress(7,0), 2, 0);
        // staticRoutingR3->AddNetworkRouteTo(Ipv4Address("10.0.99.0"), Ipv4Mask("255.255.255.252"), Spine_1.GetAddress(7,0), 2, 0);
       
        Ptr<Ipv4StaticRouting> staticRoutingR0 = ipv4RoutingHelper.GetStaticRouting (nodes.Get(1)->GetObject<Ipv4> ());
        staticRoutingR0->AddNetworkRouteTo(Ipv4Address("128.192.89.0"), Ipv4Mask("255.255.255.0"), Spine_2.GetAddress(7,0), 4, 0);
        staticRoutingR0->AddNetworkRouteTo(Ipv4Address("10.0.88.0"), Ipv4Mask("255.255.255.252"), Spine_2.GetAddress(7,0), 4, 0);
        staticRoutingR0->AddNetworkRouteTo(Ipv4Address("10.0.99.0"), Ipv4Mask("255.255.255.252"), Spine_2.GetAddress(7,0), 4, 0);
        
       
        Ptr<Ipv4StaticRouting> staticRoutingRTR3 = ipv4RoutingHelper.GetStaticRouting (nodes.Get(6)->GetObject<Ipv4> ());
        staticRoutingRTR3->AddNetworkRouteTo(Ipv4Address("10.0.88.0"), Ipv4Mask("255.255.255.252"), Spine_1.GetAddress(9,0), 1, 0);

        Ptr<Ipv4StaticRouting> staticRoutingRTR0 = ipv4RoutingHelper.GetStaticRouting (nodes.Get(8)->GetObject<Ipv4> ());
        staticRoutingRTR0->AddNetworkRouteTo(Ipv4Address("10.0.99.0"), Ipv4Mask("255.255.255.252"), Spine_2.GetAddress(9,0), 1, 0);

        Ptr<Ipv4StaticRouting> staticRoutingR8 = ipv4RoutingHelper.GetStaticRouting (nodes.Get(3)->GetObject<Ipv4> ());
        staticRoutingR8->AddNetworkRouteTo(Ipv4Address("10.0.99.0"), Ipv4Mask("255.255.255.252"), Du_net.GetAddress(5,0), 6, 0);


        Ptr<OutputStreamWrapper> allroutingtable = Create<OutputStreamWrapper>("./sim_results/routingtableallnodes",std::ios::out);
        ipv4RoutingHelper.PrintRoutingTableAllAt(Seconds(15), allroutingtable);
 
    
        /*******************************************************************
        ********************** RU node configuration ***********************
        ********************************************************************/

       ApplicationContainer Client_app, Server_app;
       int port = 8080; 
       for (int i = 0; i < (int)data.at("NumRuflows"); i++){
            // Create a packet sink on the end of the chain
            InetSocketAddress Server_Address(InetSocketAddress("128.192.8.2", port + i));
            // Server_Address.SetTos(tos[i]);
            PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", Address(Server_Address));
            Server_app.Add(packetSinkHelper.Install(nodes.Get(5))); // Last node server
            // Create the OnOff APP to send TCP to the server
            OnOffHelper clientHelper("ns3::UdpSocketFactory", Address(Server_Address));
            clientHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant="+ to_string(data.at("RuFeatures")[i]["OnTime"])+"]"));
            clientHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant="+ to_string(data.at("RuFeatures")[i]["OffTime"])+"]"));
            clientHelper.SetAttribute("DataRate", StringValue(data.at("RuFeatures")[i]["Rate"]));
            clientHelper.SetAttribute("MaxBytes", UintegerValue(data.at("RuFeatures")[i]["MaxBytes"]));
            clientHelper.SetAttribute("PacketSize", UintegerValue(pktsize));
            Client_app.Add(clientHelper.Install(NodeContainer{nodes.Get(2)}));
        }

      if(enableRTtraffic){

            // Create a packet sink on the end of the chain
            InetSocketAddress Server_Address(InetSocketAddress("10.0.88.2", 9090));
            // Server_Address.SetTos(tos[i]);
            PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", Address(Server_Address));
            Server_app.Add(packetSinkHelper.Install(nodes.Get(7))); // Last node server
            // Create the OnOff APP to send TCP to the server
            OnOffHelper clientHelper("ns3::UdpSocketFactory", Address(Server_Address));
            clientHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
            clientHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
            clientHelper.SetAttribute("DataRate", StringValue("10Gbps"));
            clientHelper.SetAttribute("MaxBytes", UintegerValue(1e6));
            clientHelper.SetAttribute("PacketSize", UintegerValue(pktsize));
            Client_app.Add(clientHelper.Install(NodeContainer{nodes.Get(6)}));

                    // Create a packet sink on the end of the chain
            InetSocketAddress Server_Address1(InetSocketAddress("10.0.99.2", 9091));
            // Server_Address.SetTos(tos[i]);
            PacketSinkHelper packetSinkHelper1("ns3::UdpSocketFactory", Address(Server_Address1));
            Server_app.Add(packetSinkHelper1.Install(nodes.Get(9))); // Last node server
            // Create the OnOff APP to send TCP to the server
            OnOffHelper clientHelper1("ns3::UdpSocketFactory", Address(Server_Address1));
            clientHelper1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
            clientHelper1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
            clientHelper1.SetAttribute("DataRate", StringValue("10Gbps"));
            clientHelper1.SetAttribute("MaxBytes", UintegerValue(1e6));
            clientHelper1.SetAttribute("PacketSize", UintegerValue(pktsize));
            Client_app.Add(clientHelper1.Install(NodeContainer{nodes.Get(8)}));

      }
       
        
    
        Server_app.Start(Seconds(0));
        Client_app.Start(Seconds(0));
        // Client_app.Stop(Seconds(30));
        // Server_app.Stop(Seconds(35));
        std::cout << GREEN << "Config. has finished" << RESET << std::endl;
        
        if (enabletracing){
            Config::ConnectWithoutContext("/NodeList/2/ApplicationList/0/$ns3::OnOffApplication/Tx",
                MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper1));
            Config::ConnectWithoutContext("/NodeList/2/ApplicationList/0/$ns3::OnOffApplication/Tx",
                MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper2));
            Config::ConnectWithoutContext("/NodeList/2/ApplicationList/0/$ns3::OnOffApplication/Tx",
                MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper3));
            Config::ConnectWithoutContext("/NodeList/2/ApplicationList/0/$ns3::OnOffApplication/Tx",
                MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper4));
           
        
            Config::ConnectWithoutContext("/NodeList/5/ApplicationList/0/$ns3::PacketSink/Rx", 
                MakeCallback(&RxTracerHelper::RxTracer, &rxTracerHelper1));
            Config::ConnectWithoutContext("/NodeList/5/ApplicationList/1/$ns3::PacketSink/Rx", 
                MakeCallback(&RxTracerHelper::RxTracer, &rxTracerHelper2));
            Config::ConnectWithoutContext("/NodeList/5/ApplicationList/2/$ns3::PacketSink/Rx", 
                MakeCallback(&RxTracerHelper::RxTracer, &rxTracerHelper3));
            Config::ConnectWithoutContext("/NodeList/5/ApplicationList/3/$ns3::PacketSink/Rx", 
                MakeCallback(&RxTracerHelper::RxTracer, &rxTracerHelper4));

            if(enableRTtraffic){
                Config::ConnectWithoutContext("/NodeList/6/ApplicationList/0/$ns3::OnOffApplication/Tx",
                    MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper5));
                Config::ConnectWithoutContext("/NodeList/8/ApplicationList/0/$ns3::OnOffApplication/Tx",
                    MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper6));

                Config::ConnectWithoutContext("/NodeList/7/ApplicationList/0/$ns3::PacketSink/Rx", 
                    MakeCallback(&RxTracerHelper::RxTracer, &rxTracerHelper5));
                Config::ConnectWithoutContext("/NodeList/9/ApplicationList/0/$ns3::PacketSink/Rx", 
                    MakeCallback(&RxTracerHelper::RxTracer, &rxTracerHelper6));
            }
           
        }

        if (enablepcap){
            p2p.EnablePcap("./sim_results/sched.pcap", nodes, true);
        }
        
        
    


        Ptr<FlowMonitor> flowMonitor;
        FlowMonitorHelper flowHelper;
        flowMonitor = flowHelper.InstallAll();

       
        Ptr<PacketSink> Server_trace1 = StaticCast<PacketSink>(Server_app.Get(0));



  
        Simulator::Stop(Seconds(3000));
        Simulator::Schedule(Seconds(2), &PrintTotalRx, Server_trace1);
        Simulator::Run();
        Simulator::Destroy();
        
        std::cout << GREEN << "Simulation has finished" << RESET << std::endl;
        flowMonitor->SerializeToXmlFile("./sim_results/DelayXml.xml", false, true);
       
        // std::cout << MAGENTA << "INFO: " << RESET << "Sink: Total RX - " << Server_trace->GetTotalRx() << " bytes" << std::endl;
        return 0;
    }
