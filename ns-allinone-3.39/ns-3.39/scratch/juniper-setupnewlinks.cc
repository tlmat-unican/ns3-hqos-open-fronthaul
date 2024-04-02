
    /*
    * This is a basic example that compares CoDel and PfifoFast queues using a simple, single-flow
    * topology:
    *
    * source_{1-N} --------------------------Router_marking--------------------------QoS_router------------------------sink_{1-N}
    *                   inf Mb/s, 0 ms                          inf Mb/s, 0 ms                       inf Mb/s, 0ms
    *                                                                                                
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
            RxFile << pkt->GetUid() << " " << Simulator::Now().GetFemtoSeconds() << " " << pkt->GetSize() << std::endl;
           
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
                TxFileUser << pkt->GetUid() << " " << Simulator::Now().GetFemtoSeconds() << " " << pkt->GetSize() <<std::endl;
            }else if((InetSocketAddress::ConvertFrom(to).GetPort()/1000)%10 == 9 ){
                TxFileControl << pkt->GetUid() << " " << Simulator::Now().GetFemtoSeconds() << " " << pkt->GetSize() <<std::endl;
            }else{
                TxFile << pkt->GetUid() << " " << Simulator::Now().GetFemtoSeconds() << " " << pkt->GetSize() <<std::endl;
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
        std::cout << MAGENTA << "INFO: " << RESET << "Sink: Total RX - " << serverSink->GetTotalRx() << " bytes in t = " << Simulator::Now().GetSeconds() << std::endl;
        Simulator::Schedule(Seconds(Simulator::Now().GetSeconds()+0.01), &PrintTotalRx, serverSink);
    }


    int
    main(int argc, char* argv[])
    {
        RngSeedManager::SetSeed(time(NULL));
        std::string JSONpath = "./scratch/scen_ex.json";
        std::ifstream f(JSONpath);
        json data = json::parse(f);
        uint32_t netMTU = 1500; 
        bool enabletracing = true, enablepcap = false, enableRTtraffic = data.at("RT_Traffic"), enablehqos = data.at("EnableHQoS");
        int Routers = 10;

        std::string simFolder = data.at("FolderName");
        std::string resultsPathname = "./sim_results/" + simFolder + "/";
        int num_RU = data.at("NumRuflows");
        int type_enB = data.at("RT_nodes");
        int num_flows_per_node = data.at("RT_flows");
        
        CommandLine cmd(__FILE__);
        cmd.AddValue("json-path", "Configuration file name", JSONpath);
        cmd.Parse(argc, argv);
    
        // LogComponentEnable("OfhApplication", LOG_LEVEL_INFO);
        // LogComponentEnable("WrrQueueDisc", LOG_LEVEL_INFO);
        // LogComponentEnable("PrioQueueDscpDisc", LOG_LEVEL_INFO);
        // Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(pktsize));
        
        std::vector<std::unique_ptr<RxTracerHelper>> rxTracersRU;
        std::vector<std::unique_ptr<TxTracerHelper>> txTracersRU;


        for (int i = 1; i <= num_RU; ++i) {
            txTracersRU.push_back(std::make_unique<TxTracerHelper>("RU", i, resultsPathname));
            rxTracersRU.push_back(std::make_unique<RxTracerHelper>("User" + std::to_string(i), resultsPathname));
            rxTracersRU.push_back(std::make_unique<RxTracerHelper>("Control" + std::to_string(i), resultsPathname));
        }

        std::vector<std::unique_ptr<RxTracerHelper>> rxTracers;
        std::vector<std::unique_ptr<TxTracerHelper>> txTracers;
        
        if(enableRTtraffic){
            // Loop to create instances of RxTracerHelper
            for (int i = 0; i < type_enB; ++i) {
                for (int j = 0; j < num_flows_per_node; ++j) {
                    txTracers.push_back(std::make_unique<TxTracerHelper>("RT", i * num_flows_per_node + j, resultsPathname));
                    rxTracers.push_back(std::make_unique<RxTracerHelper>(std::to_string(i * num_flows_per_node + j), resultsPathname));
                }
            }
        }



        NodeContainer nodes;
        nodes.Create(Routers);

        /************************************************
        ************ Creating links features ************
        *************************************************/
        PointToPointHelper AccessenB;
        AccessenB.SetDeviceAttribute("DataRate", StringValue("100Gbps"));
        AccessenB.SetChannelAttribute("Delay", StringValue("0ms"));
        AccessenB.SetDeviceAttribute("Mtu", UintegerValue(netMTU));

        PointToPointHelper p2p;
        p2p.SetDeviceAttribute("DataRate", StringValue("100Gbps"));
        p2p.SetChannelAttribute("Delay", StringValue("0ms"));
        p2p.SetDeviceAttribute("Mtu", UintegerValue(netMTU));

        PointToPointHelper midp2p;
        midp2p.SetDeviceAttribute("DataRate", StringValue(to_string(data.at("MidLinkCap"))+"Gbps"));
        // midp2p.SetDeviceAttribute("DataRate", StringValue("1kbps"));
        midp2p.SetChannelAttribute("Delay", StringValue("0ms"));
        midp2p.SetDeviceAttribute("Mtu", UintegerValue(netMTU));

       
        // Consider R4 as a marker node from a RU
        NetDeviceContainer enB0R1 = AccessenB.Install(NodeContainer{nodes.Get(0), nodes.Get(4)});
        NetDeviceContainer enB1R1 = AccessenB.Install(NodeContainer{nodes.Get(1), nodes.Get(4)}); 
        NetDeviceContainer enB2R1 = AccessenB.Install(NodeContainer{nodes.Get(2), nodes.Get(4)}); 

        NetDeviceContainer RuR1 = p2p.Install(NodeContainer{nodes.Get(3), nodes.Get(4)}); 
       
        NetDeviceContainer R1R2 = midp2p.Install(NodeContainer{nodes.Get(4), nodes.Get(5)}); 
       
        NetDeviceContainer R2Du = p2p.Install(NodeContainer{nodes.Get(5), nodes.Get(6)}); 
        NetDeviceContainer R2RT0 = AccessenB.Install(NodeContainer{nodes.Get(5), nodes.Get(7)}); 
        NetDeviceContainer R2RT1 = AccessenB.Install(NodeContainer{nodes.Get(5), nodes.Get(8)}); 
        NetDeviceContainer R2RT2 = AccessenB.Install(NodeContainer{nodes.Get(5), nodes.Get(9)}); 
    

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
            tch1.SetRootQueueDisc("ns3::MarkerQueueDisc", "MarkingQueue", StringValue(data.at("Marking_Port")));
            
            tch1.Install(enB0R1.Get(0));
            tch1.Install(enB1R1.Get(0));
            tch1.Install(enB2R1.Get(0));
            tch1.Install(RuR1.Get(0));
            // tch1.Install(R4R9.Get(0));

            //// Policies
            TrafficControlHelper tch2;
            // Set up the root queue disc with PrioQueueDisc
            uint16_t rootHandle = tch2.SetRootQueueDisc("ns3::PrioQueueDscpDisc");
            // Get ClassIdList for the second-level queues
            TrafficControlHelper::ClassIdList cid = tch2.AddQueueDiscClasses(rootHandle, 2, "ns3::QueueDiscClass");
            tch2.AddChildQueueDisc(rootHandle, cid[0], "ns3::FifoQueueDisc");
            std::string qsd_type = data.at("QSD");
            qsd_type.erase(std::remove(qsd_type.begin(), qsd_type.end(), '"'), qsd_type.end()); // Remove double quotes
            tch2.AddChildQueueDisc(rootHandle, cid[1], "ns3::"+ qsd_type + "QueueDisc", "Quantum", StringValue(data.at("Weights")), "MapQueue", StringValue(data.at("MapQueue")));
            // tch2.AddChildQueueDisc(rootHandle, cid[1], "ns3::WfqQueueDisc", "Quantum", StringValue(data.at("Weights")), "MapQueue", StringValue(data.at()));
            tch2.Install(R1R2.Get(0));

        }


        /*******************************************************************
        ************** Creating networks from spine leaf nodes *************
        ********************************************************************/
        Ipv4InterfaceContainer SmallTree;
        Ipv4AddressHelper Spine1address;
        Spine1address.SetBase("10.0.10.0", "255.255.255.252");
        SmallTree.Add(Spine1address.Assign(enB0R1)); // Assign fuction returns a Ipv4InterfaceConatiner
        Spine1address.SetBase("10.0.11.0", "255.255.255.252");
        SmallTree.Add(Spine1address.Assign(enB1R1));
        Spine1address.SetBase("10.0.21.0", "255.255.255.252");
        SmallTree.Add(Spine1address.Assign(enB2R1));
        Spine1address.SetBase("10.0.1.0", "255.255.255.252");
        SmallTree.Add(Spine1address.Assign(RuR1));
        Spine1address.SetBase("10.0.45.0", "255.255.255.252");
        SmallTree.Add(Spine1address.Assign(R1R2));
        Spine1address.SetBase("10.0.5.0", "255.255.255.252");
        SmallTree.Add(Spine1address.Assign(R2Du));
        Spine1address.SetBase("10.0.50.0", "255.255.255.252");
        SmallTree.Add(Spine1address.Assign(R2RT0));
        Spine1address.SetBase("10.0.51.0", "255.255.255.252");
        SmallTree.Add(Spine1address.Assign(R2RT1));
        Spine1address.SetBase("10.0.52.0", "255.255.255.252");
        SmallTree.Add(Spine1address.Assign(R2RT2));
        
 


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
       double time_ia = double(1)/((double(data.at("URatenum"))*double(1e9)/double(8))/double(data.at("RuFeatures")[0]["UPacketSize"]));
       double sep_app = time_ia/double(data.at("NumRuflows"));
       // std::cout << sep_app << std::endl;
       for (int i = 0; i < num_RU; i++){
            // Create a packet sink on the end of the chain
            InetSocketAddress Server_Address(InetSocketAddress("10.0.5.2", port1 + i));
            // Server_Address.SetTos(tos[i]);
            PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", Address(Server_Address));
            InetSocketAddress Server_Address2(InetSocketAddress("10.0.5.2", port2 + i));
            PacketSinkHelper packetSinkHelper2("ns3::UdpSocketFactory", Address(Server_Address2));  
            Server_app.Add(packetSinkHelper.Install(nodes.Get(6))); // Last node server
            Server_app.Add(packetSinkHelper2.Install(nodes.Get(6))); // Last node server
            // Create the Ofh APP to send TCP to the server
            OfhHelper clientHelper("ns3::UdpSocketFactory");
            clientHelper.SetAttribute("U-Plane",AddressValue(Server_Address));
            if(data.at("Poisson")){
                double time_on = double(1)/(double(data.at("RuFeatures")[i]["URatenum"])/(double(8)*double(data.at("RuFeatures")[i]["UPacketSize"])));
                std::cout << time_on << std::endl;
                clientHelper.SetAttribute("U-OnTime",  StringValue("ns3::ConstantRandomVariable[Constant="+ std::to_string(time_on) +"]"));
                clientHelper.SetAttribute("U-OffTime", StringValue("ns3::ExponentialRandomVariable[Mean="+ std::to_string(time_on)+"]"));
            }else{
                clientHelper.SetAttribute("U-OnTime", StringValue("ns3::ConstantRandomVariable[Constant="+ to_string(data.at("RuFeatures")[i]["UOnTime"])+"]"));
                clientHelper.SetAttribute("U-OffTime", StringValue("ns3::ConstantRandomVariable[Constant="+ to_string(data.at("RuFeatures")[i]["UOffTime"])+"]"));
            }
            
            clientHelper.SetAttribute("U-DataRate", StringValue(data.at("RuFeatures")[i]["URate"]));
            clientHelper.SetAttribute("U-MaxBytes", UintegerValue(data.at("RuFeatures")[i]["UMaxBytes"]));
            clientHelper.SetAttribute("U-PacketSize", UintegerValue(data.at("RuFeatures")[i]["UPacketSize"]));
            clientHelper.SetAttribute("C-Plane", AddressValue(Server_Address2));
            clientHelper.SetAttribute("C-DataRate", StringValue(data.at("RuFeatures")[i]["CRate"]));
            clientHelper.SetAttribute("C-PacketSize", UintegerValue(data.at("RuFeatures")[i]["CPacketSize"]));
            clientHelper.SetAttribute("C-MaxBytes", UintegerValue(data.at("RuFeatures")[i]["CMaxBytes"]));
            clientHelper.SetAttribute("StartTime", TimeValue(NanoSeconds(sep_app*double(i))));
           
            Client_app.Add(clientHelper.Install(NodeContainer{nodes.Get(3)}));
        }
       
      if(enableRTtraffic){

            int portRT = 10800;
            for (int i = 0; i < type_enB; i++){
                // Create a packet sink on the end of the chain
                std::string str = "10.0.5" + std::to_string(i) + ".2";
                const char *adr = str.c_str();
                for (int j = 0; j < num_flows_per_node; j++){
                    InetSocketAddress Server_AddressRT1(InetSocketAddress(adr, portRT + j));
                    // Server_Address.SetTos(tos[i]);
                    PacketSinkHelper packetSinkHelperRT1("ns3::UdpSocketFactory", Address(Server_AddressRT1));
                    Server_app.Add(packetSinkHelperRT1.Install(nodes.Get(7 + i))); // Last node server
                    // Create the Ofh APP to send TCP to the server
                    OnOffHelper clientHelper0("ns3::UdpSocketFactory", Address(Server_AddressRT1));
                    clientHelper0.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant="+ to_string(data.at("RTFeatures")[i]["OnTime"])+"]"));
                    clientHelper0.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant="+ to_string(data.at("RTFeatures")[i]["OffTime"])+"]"));
                    clientHelper0.SetAttribute("DataRate", StringValue((data.at("RTFeatures")[i]["Rate"])));
                    clientHelper0.SetAttribute("MaxBytes", UintegerValue(data.at("RTFeatures")[i]["MaxBytes"]));
                    clientHelper0.SetAttribute("PacketSize", UintegerValue(data.at("RTFeatures")[i]["PacketSize"][j]));
                    // clientHelper0.SetAttribute("StartTime",TimeValue(NanoSeconds(i)));
                    Client_app.Add(clientHelper0.Install(NodeContainer{nodes.Get(i)}));
                }
                portRT = portRT + 100;
        
                    
            }


      }
       
   
        Server_app.Start(Seconds(0));
        Client_app.Start(Seconds(0));  

        
        if (enabletracing){
        
            for (int i = 0; i < num_RU; ++i) {
                    // Construct the callback paths
                    std::string txCallbackPath = "/NodeList/3/ApplicationList/" + std::to_string(i) + "/$ns3::OfhApplication/TxWithAddresses";
                    Config::ConnectWithoutContext(txCallbackPath, MakeCallback(&TxTracerHelper::TxTracer, txTracersRU[i].get()));
                for (int j = 0; j < 2; ++j) {
                    std::string rxCallbackPath = "/NodeList/6/ApplicationList/" + std::to_string(i * 2 + j) + "/$ns3::PacketSink/Rx";
                    // Connect the RxTracerHelper callback
                    Config::ConnectWithoutContext(rxCallbackPath, MakeCallback(&RxTracerHelper::RxTracerWithAdresses, rxTracersRU[i * 2 + j].get()));
                }
            }
   

            if(enableRTtraffic){
                for (int i = 0; i < type_enB; ++i) {
                    for (int j = 0; j < num_flows_per_node; ++j) {
                        // Construct the callback paths
                        std::string txCallbackPath = "/NodeList/" + std::to_string(i) + "/ApplicationList/" + std::to_string(j) + "/$ns3::OnOffApplication/TxWithAddresses";
                        std::string rxCallbackPath = "/NodeList/" + std::to_string(7+i) + "/ApplicationList/" + std::to_string(j) + "/$ns3::PacketSink/Rx";
                        // Connect the TxTracerHelper callback
                        Config::ConnectWithoutContext(txCallbackPath, MakeCallback(&TxTracerHelper::TxTracer, txTracers[i * num_flows_per_node + j].get()));
                        // Connect the RxTracerHelper callback
                        Config::ConnectWithoutContext(rxCallbackPath, MakeCallback(&RxTracerHelper::RxTracerWithAdresses, rxTracers[i * num_flows_per_node + j].get()));
                    }
                }

            }
           
        }

        if (enablepcap){
            p2p.EnablePcap("./sim_results/sched.pcap", nodes, true);
        }
        
        
        std::cout << GREEN << "Config. has finished" << RESET << std::endl;


        Ptr<FlowMonitor> flowMonitor;
        FlowMonitorHelper flowHelper;
        flowMonitor = flowHelper.InstallAll();

       
        Ptr<PacketSink> Server_trace1 = StaticCast<PacketSink>(Server_app.Get(0));



  
        Simulator::Stop(Seconds(data.at("Seconds_sim")));
        Simulator::Schedule(Seconds(0.001), &PrintTotalRx, Server_trace1);
        Simulator::Run();
        Simulator::Destroy();
        
        std::cout << GREEN << "Simulation has finished" <<  RESET << std::endl;
        flowMonitor->SerializeToXmlFile("./sim_results/DelayXml.xml", false, true);
       
        // std::cout << MAGENTA << "INFO: " << RESET << "Sink: Total RX - " << Server_trace->GetTotalRx() << " bytes" << std::endl;
        return 0;
    }
