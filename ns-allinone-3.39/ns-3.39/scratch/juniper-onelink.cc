
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
            RxFile << pkt->GetUid() << " " << Simulator::Now().GetNanoSeconds() << " " << pkt->GetSize() << std::endl;
           
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
            TxFile.close();
        }

        void TxTracer(Ptr<const Packet> pkt, const Address & from, const Address & to) {

            if( (InetSocketAddress::ConvertFrom(to).GetPort()/1000)%10 == 8 ){
                TxFileUser << pkt->GetUid() << " " << Simulator::Now().GetNanoSeconds() << " " << pkt->GetSize() <<std::endl;
            }else if((InetSocketAddress::ConvertFrom(to).GetPort()/1000)%10 == 9 ){
                TxFileControl << pkt->GetUid() << " " << Simulator::Now().GetNanoSeconds() << " " << pkt->GetSize() <<std::endl;
            }else{
                TxFile << pkt->GetUid() << " " << Simulator::Now().GetNanoSeconds() << " " << pkt->GetSize() <<std::endl;
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
        Simulator::Schedule(Seconds(Simulator::Now().GetNanoSeconds()+0.2), &PrintTotalRx, serverSink);
    }


    int
    main(int argc, char* argv[])
    {
        RngSeedManager::SetSeed(time(NULL));
        std::string JSONpath = "./scratch/scen1link.json";
        std::ifstream f(JSONpath);
        json data = json::parse(f);
        uint32_t netMTU = 8000; 
        bool enabletracing = true, enablepcap = false;
        int Routers = 2;

        std::string simFolder = data.at("FolderName");
        std::string resultsPathname = "./sim_results/" + simFolder + "/";
        // LogComponentEnable("OfhApplication", LOG_LEVEL_INFO);
        // LogComponentEnable("WrrQueueDisc", LOG_LEVEL_INFO);
        // LogComponentEnable("PrioQueueDscpDisc", LOG_LEVEL_INFO);
        // Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(pktsize));
        TxTracerHelper txTracerHelper1("RU", 1, resultsPathname), txTracerHelper2("RU", 2, resultsPathname), txTracerHelper3("RU", 3, resultsPathname);
                
        RxTracerHelper rxTracerHelperUser1("User1", resultsPathname), rxTracerHelperControl1("Control1", resultsPathname),
                        rxTracerHelperUser2("User2", resultsPathname), rxTracerHelperControl2("Control2", resultsPathname),
                        rxTracerHelperUser3("User3", resultsPathname), rxTracerHelperControl3("Control3", resultsPathname);


        NodeContainer nodes;
        nodes.Create(Routers);

    
        /************************************************
        ************ Creating links features ************
        *************************************************/
     
        PointToPointHelper p2p;
        p2p.SetDeviceAttribute("DataRate", StringValue(data.at("MidLinkCap")));
        p2p.SetChannelAttribute("Delay", StringValue("0ms"));
        p2p.SetDeviceAttribute("Mtu", UintegerValue(netMTU));


        NetDeviceContainer Node1Node2 = p2p.Install(NodeContainer{nodes.Get(0), nodes.Get(1)}); 
    

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

        


        /*******************************************************************
        ************** Creating networks from spine leaf nodes *************
        ********************************************************************/
        Ipv4InterfaceContainer SmallTree;
        Ipv4AddressHelper Spine1address;
        Spine1address.SetBase("10.0.10.0", "255.255.255.252");
        SmallTree.Add(Spine1address.Assign(Node1Node2)); // Assign fuction returns a Ipv4InterfaceConatiner
      
 


        /*********************************************************************
        ********************** Populate Routing Tables ***********************
        **********************************************************************/
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();
       
    
        /*******************************************************************
        ********************** RU node configuration ***********************
        ********************************************************************/
       ApplicationContainer Client_app, Server_app;
       int port1 = 8080; 
       int port2 = 9090;
       double time_ia = double(1)/((double(data.at("URatenum"))/double(8))/double(data.at("RuFeatures")[0]["UPacketSize"]));
       double sep_app = time_ia/double(data.at("NumRuflows"));
       for (int i = 0; i < (int)data.at("NumRuflows"); i++){
            // Create a packet sink on the end of the chain
            InetSocketAddress Server_Address(InetSocketAddress("10.0.10.2", port1 + i));
            // Server_Address.SetTos(tos[i]);
            PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", Address(Server_Address));
            InetSocketAddress Server_Address2(InetSocketAddress("10.0.10.2", port2 + i));
            PacketSinkHelper packetSinkHelper2("ns3::UdpSocketFactory", Address(Server_Address2));  
            Server_app.Add(packetSinkHelper.Install(nodes.Get(1))); // Last node server
            Server_app.Add(packetSinkHelper2.Install(nodes.Get(1))); // Last node server
            // Create the Ofh APP to send TCP to the server
            OfhHelper clientHelper("ns3::UdpSocketFactory");
            clientHelper.SetAttribute("U-Plane",AddressValue(Server_Address));
            clientHelper.SetAttribute("U-OnTime", StringValue("ns3::ConstantRandomVariable[Constant="+ to_string(data.at("RuFeatures")[i]["UOnTime"])+"]"));
            clientHelper.SetAttribute("U-OffTime", StringValue("ns3::ConstantRandomVariable[Constant="+ to_string(data.at("RuFeatures")[i]["UOffTime"])+"]"));
            clientHelper.SetAttribute("U-DataRate", StringValue(data.at("RuFeatures")[i]["URate"]));
            clientHelper.SetAttribute("U-MaxBytes", UintegerValue(data.at("RuFeatures")[i]["UMaxBytes"]));
            clientHelper.SetAttribute("U-PacketSize", UintegerValue(data.at("RuFeatures")[i]["UPacketSize"]));
            clientHelper.SetAttribute("C-Plane", AddressValue(Server_Address2));
            clientHelper.SetAttribute("C-DataRate", StringValue(data.at("RuFeatures")[i]["CRate"]));
            clientHelper.SetAttribute("C-PacketSize", UintegerValue(data.at("RuFeatures")[i]["CPacketSize"]));
            clientHelper.SetAttribute("C-MaxBytes",UintegerValue(data.at("RuFeatures")[i]["CMaxBytes"]));
            
            clientHelper.SetAttribute("StartTime", TimeValue(Seconds(sep_app*double(i))));

            Client_app.Add(clientHelper.Install(NodeContainer{nodes.Get(0)}));
        }
       
    
        
    
        Server_app.Start(Seconds(0));
        Client_app.Start(Seconds(0));
     
        
        
        if (enabletracing){
            Config::ConnectWithoutContext("/NodeList/0/ApplicationList/0/$ns3::OfhApplication/TxWithAddresses",
                MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper1));
            // Config::ConnectWithoutContext("/NodeList/0/ApplicationList/1/$ns3::OfhApplication/TxWithAddresses",
            //     MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper2));
            // Config::ConnectWithoutContext("/NodeList/0/ApplicationList/2/$ns3::OfhApplication/TxWithAddresses",
            //     MakeCallback(&TxTracerHelper::TxTracer, &txTracerHelper3));
      
        
            Config::ConnectWithoutContext("/NodeList/1/ApplicationList/0/$ns3::PacketSink/Rx", 
                MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelperUser1));
            Config::ConnectWithoutContext("/NodeList/1/ApplicationList/1/$ns3::PacketSink/Rx", 
                MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelperControl1));

            // Config::ConnectWithoutContext("/NodeList/1/ApplicationList/2/$ns3::PacketSink/Rx", 
            //     MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelperUser2));
            // Config::ConnectWithoutContext("/NodeList/1/ApplicationList/3/$ns3::PacketSink/Rx", 
            //     MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelperControl2));
            
            // Config::ConnectWithoutContext("/NodeList/1/ApplicationList/4/$ns3::PacketSink/Rx", 
            //     MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelperUser3));
            // Config::ConnectWithoutContext("/NodeList/1/ApplicationList/5/$ns3::PacketSink/Rx", 
            //     MakeCallback(&RxTracerHelper::RxTracerWithAdresses, &rxTracerHelperControl3));


   

            
           
        }

        if (enablepcap){
            p2p.EnablePcap("./sim_results/sched.pcap", nodes, true);
        }
        
        
        std::cout << GREEN << "Config. has finished" << RESET << std::endl;


        Ptr<FlowMonitor> flowMonitor;
        FlowMonitorHelper flowHelper;
        flowMonitor = flowHelper.InstallAll();

       
        Ptr<PacketSink> Server_trace1 = StaticCast<PacketSink>(Server_app.Get(0));



  
        Simulator::Stop(Seconds(9000));
        Simulator::Schedule(Seconds(0.1), &PrintTotalRx, Server_trace1);
        Simulator::Run();
        Simulator::Destroy();
        
        std::cout << GREEN << "Simulation has finished" << RESET << std::endl;
        flowMonitor->SerializeToXmlFile("./sim_results/DelayXml.xml", false, true);
       
        // std::cout << MAGENTA << "INFO: " << RESET << "Sink: Total RX - " << Server_trace->GetTotalRx() << " bytes" << std::endl;
        return 0;
    }
