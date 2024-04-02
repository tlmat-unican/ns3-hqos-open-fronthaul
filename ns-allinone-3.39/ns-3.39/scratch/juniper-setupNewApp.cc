
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
        RxTracerHelper(const std::string& name, const std::string& filename) {
            RxFile.open(filename + "RxFile" + name + ".log", std::fstream::out);
        }

        ~RxTracerHelper() {
            RxFile.close(); // Close the file in the destructor
        }

        void RxTracerWithAdresses(Ptr<const Packet> pkt, const Address & from) {
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
        TxTracerHelper(const std::string& typetx, int num, const std::string& filename) {
            if (typetx == "RU"){
                TxFileUser.open(filename + "TxFileUser" + std::to_string(num) + ".log", std::fstream::out);
                TxFileControl.open(filename + "TxFileControl" + std::to_string(num) + ".log", std::fstream::out);
            }else{
                TxFile.open(filename + "TxFile" + std::to_string(num) + ".log", std::fstream::out);
            }
            

       
        }

        ~TxTracerHelper() {
            TxFileUser.close(); // Close the file in the destructor
            TxFileControl.close();
        }

        void TxTracer(Ptr<const Packet> pkt, const Address & from, const Address & to) {

            if( (InetSocketAddress::ConvertFrom(to).GetPort()/1000)%10 == 8 ){
                TxFileUser << pkt->GetUid() << " " << Simulator::Now().GetSeconds() << " " << pkt->GetSize() <<std::endl;
            }else if((InetSocketAddress::ConvertFrom(to).GetPort()/1000)%10 == 9 ){
                TxFileControl << pkt->GetUid() << " " << Simulator::Now().GetSeconds() << " " << pkt->GetSize() <<std::endl;
            }else{
                TxFile << pkt->GetUid() << " " << Simulator::Now().GetSeconds() << " " << pkt->GetSize() <<std::endl;
            }
            
            
            // Don't close the file here; it will be automatically closed in the destructor
        }


    private:
        std::ofstream TxFileUser;
        std::ofstream TxFileControl;
        std::ofstream TxFile;
        int num;
        std::string filename;
        std::string typetx;
       
    };

    // Function to print total received bytes
    void PrintTotalRx(Ptr<PacketSink> serverSink) {
        std::cout << MAGENTA << "INFO: " << RESET << "Sink: Total RX - " << serverSink->GetTotalRx() << " bytes" << std::endl;
        Simulator::Schedule(Seconds(Simulator::Now().GetSeconds()+0.2), &PrintTotalRx, serverSink);
    }


    int
    main(int argc, char* argv[])
    {
        RngSeedManager::SetSeed(time(NULL));
        std::string JSONpath = "./scratch/scen.json";
        std::ifstream f(JSONpath);
        json data = json::parse(f);
        uint32_t netMTU = 8000; 
        bool enabletracing = true, enablepcap = false, enableRTtraffic = data.at("RT_Traffic"), enablehqos = true;
        int Routers = 10;
   
    

        // LogComponentEnable("OfhApplication", LOG_LEVEL_INFO);
        // LogComponentEnable("WrrQueueDisc", LOG_LEVEL_INFO);
        // LogComponentEnable("PrioQueueDscpDisc", LOG_LEVEL_INFO);
        // Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(pktsize));
        TxTracerHelper txTracerHelper1("RU", 1, "./sim_results/"), txTracerHelper2("RU", 2, "./sim_results/"), 
                        txTracerHelper3("RU", 3, "./sim_results/"),  
                        // RT flows
                        txTracerHelper4("RT", 4, "./sim_results/"), txTracerHelper5("RT", 5, "./sim_results/"), 
                        txTracerHelper6("RT", 6, "./sim_results/");

        RxTracerHelper rxTracerHelperUser1("User1", "./sim_results/"), rxTracerHelperControl1("Control1", "./sim_results/"),
                        rxTracerHelperUser2("User2", "./sim_results/"), rxTracerHelperControl2("Control2", "./sim_results/"),
                        rxTracerHelperUser3("User3", "./sim_results/"), rxTracerHelperControl3("Control3", "./sim_results/"),
                        // RT flows
                        rxTracerHelper4("4", "./sim_results/"), rxTracerHelper5("5", "./sim_results/"), rxTracerHelper6("6", "./sim_results/");
                   
   
        
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
            // tch1.SetRootQueueDisc("ns3::MarkerQueueDisc", "MarkingQueue", StringValue("8080 46 8081 8 8082 16 8083 24 8084 32"));
            tch1.SetRootQueueDisc("ns3::MarkerQueueDisc", "MarkingQueue", StringValue("8080 46 8081 46 8082 46 9090 46 9091 46 9092 46 9093 46 9094 46 10090 8 10091 16 10092 24"));
            // tch1.Install(R0R4.Get(0));
            tch1.Install(R4R8.Get(0));
            // tch1.Install(R4R9.Get(0));

            //// Policies
            TrafficControlHelper tch2;
            // Set up the root queue disc with PrioQueueDisc
            uint16_t rootHandle = tch2.SetRootQueueDisc("ns3::PrioQueueDscpDisc");
            // Get ClassIdList for the second-level queues
            TrafficControlHelper::ClassIdList cid = tch2.AddQueueDiscClasses(rootHandle, 2, "ns3::QueueDiscClass");
            tch2.AddChildQueueDisc(rootHandle, cid[0], "ns3::FifoQueueDisc");
            tch2.AddChildQueueDisc(rootHandle, cid[1], "ns3::WrrQueueDisc", "Quantum", StringValue("10 1 1"), "MapQueue", StringValue("8 0 16 1 24 2"));

            tch2.Install(R8Du.Get(0));
            // tch2.Install(R8RT12.Get(0));
            // tch2.Install(R8R9.Get(0));
            // tch2.Install(R9RT12.Get(0));
            // tch2.Install(R9Du.Get(1));
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
        // staticRoutingR4->AddNetworkRouteTo(Ipv4Address("128.192.89.0"), Ipv4Mask("255.255.255.0"), Spine_1.GetAddress(5,0), 1, 0);
        staticRoutingR4->AddNetworkRouteTo(Ipv4Address("10.0.88.0"), Ipv4Mask("255.255.255.252"), Spine_1.GetAddress(5,0), 1, 0);
        staticRoutingR4->AddNetworkRouteTo(Ipv4Address("10.0.99.0"), Ipv4Mask("255.255.255.252"), Spine_1.GetAddress(5,0), 1, 0);

        Ptr<Ipv4StaticRouting> staticRoutingR3 = ipv4RoutingHelper.GetStaticRouting (nodes.Get(0)->GetObject<Ipv4> ());
        staticRoutingR3->AddNetworkRouteTo(Ipv4Address("128.192.89.0"), Ipv4Mask("255.255.255.0"), Spine_1.GetAddress(7,0), 2, 0);
        // staticRoutingR3->AddNetworkRouteTo(Ipv4Address("10.0.88.0"), Ipv4Mask("255.255.255.252"), Spine_1.GetAddress(7,0), 2, 0);
        staticRoutingR3->AddNetworkRouteTo(Ipv4Address("10.0.99.0"), Ipv4Mask("255.255.255.252"), Spine_1.GetAddress(7,0), 2, 0);

        Ptr<Ipv4StaticRouting> staticRoutingR0 = ipv4RoutingHelper.GetStaticRouting (nodes.Get(1)->GetObject<Ipv4> ());
        // staticRoutingR0->AddNetworkRouteTo(Ipv4Address("128.192.89.0"), Ipv4Mask("255.255.255.0"), Spine_2.GetAddress(7,0), 4, 0);
        // staticRoutingR0->AddNetworkRouteTo(Ipv4Address("10.0.88.0"), Ipv4Mask("255.255.255.252"), Spine_2.GetAddress(7,0), 4, 0);
        staticRoutingR0->AddNetworkRouteTo(Ipv4Address("10.0.99.0"), Ipv4Mask("255.255.255.252"), Spine_2.GetAddress(7,0), 4, 0);
    
       
        Ptr<Ipv4StaticRouting> staticRoutingRTR3 = ipv4RoutingHelper.GetStaticRouting (nodes.Get(6)->GetObject<Ipv4> ());
        // staticRoutingRTR3->AddNetworkRouteTo(Ipv4Address("10.0.88.0"), Ipv4Mask("255.255.255.252"), Spine_1.GetAddress(9,0), 1, 0);
        staticRoutingRTR3->AddNetworkRouteTo(Ipv4Address("10.0.99.0"), Ipv4Mask("255.255.255.252"), Spine_1.GetAddress(9,0), 1, 0);


        Ptr<Ipv4StaticRouting> staticRoutingRTR0 = ipv4RoutingHelper.GetStaticRouting (nodes.Get(8)->GetObject<Ipv4> ());
        staticRoutingRTR0->AddNetworkRouteTo(Ipv4Address("10.0.99.0"), Ipv4Mask("255.255.255.252"), Spine_2.GetAddress(9,0), 1, 0);


        Ptr<Ipv4StaticRouting> staticRoutingR8 = ipv4RoutingHelper.GetStaticRouting (nodes.Get(3)->GetObject<Ipv4> ());
        staticRoutingR8->AddNetworkRouteTo(Ipv4Address("10.0.99.0"), Ipv4Mask("255.255.255.252"), Du_net.GetAddress(1,0), 5, 0);


        Ptr<Ipv4StaticRouting> staticRoutingDU = ipv4RoutingHelper.GetStaticRouting (nodes.Get(5)->GetObject<Ipv4> ());
        staticRoutingDU->AddNetworkRouteTo(Ipv4Address("10.0.99.0"), Ipv4Mask("255.255.255.252"), Du_net.GetAddress(2,0), 2, 0);

        Ptr<OutputStreamWrapper> allroutingtable = Create<OutputStreamWrapper>("./sim_results/routingtableallnodes",std::ios::out);
        ipv4RoutingHelper.PrintRoutingTableAllAt(Seconds(0), allroutingtable);
 
    
        /*******************************************************************
        ********************** RU node configuration ***********************
        ********************************************************************/

       ApplicationContainer Client_app, Server_app;
       int port1 = 8080; 
       int port2 = 9090;
       for (int i = 0; i < (int)data.at("NumRuflows"); i++){
            // Create a packet sink on the end of the chain
            InetSocketAddress Server_Address(InetSocketAddress("128.192.8.2", port1 + i));
            // Server_Address.SetTos(tos[i]);
            PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", Address(Server_Address));
            InetSocketAddress Server_Address2(InetSocketAddress("128.192.8.2", port2 + i));
            PacketSinkHelper packetSinkHelper2("ns3::UdpSocketFactory", Address(Server_Address2));  
            Server_app.Add(packetSinkHelper.Install(nodes.Get(5))); // Last node server
            Server_app.Add(packetSinkHelper2.Install(nodes.Get(5))); // Last node server
            // Create the Ofh APP to send TCP to the server
            OfhHelper clientHelper("ns3::UdpSocketFactory");
            clientHelper.SetAttribute("U-Plane",AddressValue(Server_Address));
            clientHelper.SetAttribute("U-OnTime", StringValue("ns3::ConstantRandomVariable[Constant="+ to_string(data.at("RuFeatures")[i]["UOnTime"])+"]"));
            clientHelper.SetAttribute("U-OffTime", StringValue("ns3::ConstantRandomVariable[Constant="+ to_string(data.at("RuFeatures")[i]["UOffTime"])+"]"));
            clientHelper.SetAttribute("U-DataRate", StringValue(data.at("RuFeatures")[i]["URate"]));
            clientHelper.SetAttribute("U-MaxBytes", UintegerValue(data.at("RuFeatures")[i]["UMaxBytes"]));
            clientHelper.SetAttribute("U-PacketSize", UintegerValue(data.at("RuFeatures")[i]["UPacketSize"]));
            clientHelper.SetAttribute("C-Plane",AddressValue(Server_Address2));
            clientHelper.SetAttribute("C-DataRate", StringValue(data.at("RuFeatures")[i]["CRate"]));
            clientHelper.SetAttribute("C-PacketSize", UintegerValue(data.at("RuFeatures")[i]["CPacketSize"]));
            clientHelper.SetAttribute("C-MaxBytes",UintegerValue(data.at("RuFeatures")[i]["CMaxBytes"]));
            
           
            Client_app.Add(clientHelper.Install(NodeContainer{nodes.Get(2)}));
        }

      if(enableRTtraffic){

            int portRT = 10090;
            for (int i = 0; i < (int)data.at("RT_flows"); i++){
                // Create a packet sink on the end of the chain
                InetSocketAddress Server_AddressRT1(InetSocketAddress("10.0.99.2", portRT + i));
                // Server_Address.SetTos(tos[i]);
                PacketSinkHelper packetSinkHelperRT1("ns3::UdpSocketFactory", Address(Server_AddressRT1));
                Server_app.Add(packetSinkHelperRT1.Install(nodes.Get(9))); // Last node server
                // Create the Ofh APP to send TCP to the server
                OnOffHelper clientHelper0("ns3::UdpSocketFactory", Address(Server_AddressRT1));
                clientHelper0.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant="+ to_string(data.at("RTFeatures")[i]["OnTime"])+"]"));
                clientHelper0.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant="+ to_string(data.at("RTFeatures")[i]["OffTime"])+"]"));
                clientHelper0.SetAttribute("DataRate", StringValue((data.at("RTFeatures")[i]["Rate"])));
                clientHelper0.SetAttribute("MaxBytes", UintegerValue(data.at("RTFeatures")[i]["MaxBytes"]));
                clientHelper0.SetAttribute("PacketSize", UintegerValue(data.at("RTFeatures")[i]["PacketSize"]));
                Client_app.Add(clientHelper0.Install(NodeContainer{nodes.Get(8)}));
            }


      }
       
        
    
        Server_app.Start(Seconds(0));
        Client_app.Start(Seconds(0));

        
        
        if (enabletracing){
            Config::ConnectWithoutContext("/NodeList/2/ApplicationList/0/$ns3::OfhApplication/TxWithAddresses",
                MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper1));
            Config::ConnectWithoutContext("/NodeList/2/ApplicationList/1/$ns3::OfhApplication/TxWithAddresses",
                MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper2));
            Config::ConnectWithoutContext("/NodeList/2/ApplicationList/2/$ns3::OfhApplication/TxWithAddresses",
                MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper3));
            // Config::ConnectWithoutContext("/NodeList/2/ApplicationList/0/$ns3::OfhApplication/TxWithAddresses",
            //     MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper4));
           
        
            Config::ConnectWithoutContext("/NodeList/5/ApplicationList/0/$ns3::PacketSink/Rx", 
                MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelperUser1));
            Config::ConnectWithoutContext("/NodeList/5/ApplicationList/1/$ns3::PacketSink/Rx", 
                MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelperControl1));

            Config::ConnectWithoutContext("/NodeList/5/ApplicationList/2/$ns3::PacketSink/Rx", 
                MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelperUser2));
            Config::ConnectWithoutContext("/NodeList/5/ApplicationList/3/$ns3::PacketSink/Rx", 
                MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelperControl2));
            
            Config::ConnectWithoutContext("/NodeList/5/ApplicationList/4/$ns3::PacketSink/Rx", 
                MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelperUser3));
            Config::ConnectWithoutContext("/NodeList/5/ApplicationList/5/$ns3::PacketSink/Rx", 
                MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelperControl3));


   

            if(enableRTtraffic){
                Config::ConnectWithoutContext("/NodeList/8/ApplicationList/0/$ns3::OnOffApplication/TxWithAddresses",
                    MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper4));
                Config::ConnectWithoutContext("/NodeList/8/ApplicationList/1/$ns3::OnOffApplication/TxWithAddresses",
                    MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper5));
                Config::ConnectWithoutContext("/NodeList/8/ApplicationList/2/$ns3::OnOffApplication/TxWithAddresses",
                    MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper6));

                Config::ConnectWithoutContext("/NodeList/9/ApplicationList/0/$ns3::PacketSink/Rx", 
                    MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelper4));
                Config::ConnectWithoutContext("/NodeList/9/ApplicationList/1/$ns3::PacketSink/Rx", 
                    MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelper5));
                Config::ConnectWithoutContext("/NodeList/9/ApplicationList/2/$ns3::PacketSink/Rx", 
                    MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelper6));
            }
           
        }

        if (enablepcap){
            p2p.EnablePcap("./sim_results/sched.pcap", nodes, true);
        }
        
        
        std::cout << GREEN << "Config. has finished" << RESET << std::endl;


        Ptr<FlowMonitor> flowMonitor;
        FlowMonitorHelper flowHelper;
        flowMonitor = flowHelper.InstallAll();

       
        Ptr<PacketSink> Server_trace1 = StaticCast<PacketSink>(Server_app.Get(2));



  
        Simulator::Stop(Seconds(3000));
        Simulator::Schedule(Seconds(0.1), &PrintTotalRx, Server_trace1);
        Simulator::Run();
        Simulator::Destroy();
        
        std::cout << GREEN << "Simulation has finished" << RESET << std::endl;
        flowMonitor->SerializeToXmlFile("./sim_results/DelayXml.xml", false, true);
       
        // std::cout << MAGENTA << "INFO: " << RESET << "Sink: Total RX - " << Server_trace->GetTotalRx() << " bytes" << std::endl;
        return 0;
    }
