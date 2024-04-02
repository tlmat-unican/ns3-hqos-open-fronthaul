#ifndef SCENARIO_HELPER_H
#define SCENARIO_HELPER_H



#include <cassert>
#include <string>
#include <fstream>
#include <map>
#include <tuple>
#include <string>
#include <iostream>
#include <list>
#include <vector>
#include <iomanip>
#include <exception>


#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"
#include "ns3/traffic-control-module.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace ns3;

#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define MAGENTA "\x1b[35m"
#define BG_ORANGE "\x1B[48;2;255;128;0m"
#define RESET "\x1b[0m"

NS_LOG_COMPONENT_DEFINE("Scenario");
void CreateMiddleLinks(PointToPointHelper pointToPoint, std::vector<NetDeviceContainer> &Devs, json data, NodeContainer nodes)
{
  for (int i = 0; i < (int)data.at("NumLinks"); i++)
  {
    pointToPoint.SetDeviceAttribute("DataRate", StringValue(to_string(data.at("Links")[i]["DataRate"]) + "Mbps")); // BW
    pointToPoint.SetChannelAttribute("Delay", StringValue(to_string(data.at("Links")[i]["Delay"]) + "ms"));        // delay
    pointToPoint.SetDeviceAttribute("Mtu", UintegerValue(data.at("Links")[i]["MTU"]));
    // pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(to_string(data.at("Links")[i]["DropTailQueue"]) + "p")); // FIXME: 1000000000000p, 15p, 7p
    // pointToPoint.SetDeviceAttribute("Mode", StringValue(data.at("Links")[i]["Mode"]));
    pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", ns3::QueueSizeValue(QueueSize(QueueSizeUnit::BYTES, (data.at("Links")[i]["DropTailQueue"]))));
    NetDeviceContainer p2pDev = pointToPoint.Install(NodeContainer{nodes.Get(i), nodes.Get(i + 1)});
    Devs.push_back(p2pDev);
    if (data.at("Links")[i]["ErrorRate"] != 0)
    {
      Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
      em->SetAttribute("ErrorRate", DoubleValue(data.at("Links")[i]["ErrorRate"]));
      em->SetAttribute("ErrorUnit", EnumValue(2)); // ERROR_UNIT_PACKET
      Devs.at(i).Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));
    }
    // cout << "The number of PDPdev "<< i+1 <<" created is: " << Devs.at(i).GetN() << "\n";
  }
  NS_LOG_INFO(MAGENTA << "INFO: " << RESET << "Middle Links created");
}




void ManageMiddleLinks(json data, PointToPointHelper pointToPoint, std::vector<NetDeviceContainer> &Devs, NodeContainer &nodes, int &auxnodes)
{

    auxnodes = 1 + (int)data.at("NumLinks");
    nodes.Create(auxnodes);
    CreateMiddleLinks(pointToPoint, Devs, data, nodes);

    NS_LOG_INFO(MAGENTA << "INFO: " << RESET << "Manage Midlle Links finished");
}



void SettingupMiddleNetworks(std::vector<Ipv4InterfaceContainer> &ifacesP2p, std::vector<NetDeviceContainer> Devs, json data)
{
  for (int i = 0; i < (int)data.at("NumLinks"); i++)
  {
    Ipv4AddressHelper addressesP2p;
    std::string str = "11." + std::to_string(i + 1) + ".0.0";
    const char *adr = str.c_str();
    NS_LOG_INFO(MAGENTA << "INFO: " << RESET << "Creating P2P Network " << i + 1 << ": " << str);
    addressesP2p.SetBase(adr, "255.255.255.0");
    Ipv4InterfaceContainer ifacP2p = addressesP2p.Assign(Devs.at(i)); // Assign fuction returns a Ipv4InterfaceConatiner
    ifacesP2p.push_back(ifacP2p);

    NS_LOG_INFO(MAGENTA << "INFO: " << RESET << "Ifaces P2P are " << ifacesP2p.at(i).GetN());
  }
}


void InstallApps(ApplicationContainer &app, ApplicationContainer &sinkApps, json data,
                 std::vector<Ipv4InterfaceContainer> &ifacesP2p, NodeContainer nodes)
{
    double start_time = 0;
   
    // Set up sink applications first
    for (int i = 0; i < (int)data.at("NumLinks"); i++)
    {
        if (data.at("Links")[i]["APP"])
        {
          for (int j = 0; j < (int)data.at("Links")[i]["numApps"]; j++){
            NS_LOG_INFO(MAGENTA << "INFO: " << RESET << "Set APP" << j <<" Rx at node" << i);
            uint16_t port = data.at("Links")[i]["AppFeatures"][j]["RemotePort"];
            std::string protocol = data.at("Links")[i]["AppFeatures"][j]["Protocol"];
            Address sinkAddress = InetSocketAddress((ifacesP2p.at(i).GetAddress(data.at("Links")[i]["AppFeatures"][j]["NodeDst"])), port);
            if (protocol == "TCP")
            {
                PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkAddress);
                sinkApps.Add(sinkHelper.Install(nodes.Get(data.at("Links")[i]["AppFeatures"][j]["NodeDst"])));
            }
            else if (protocol == "UDP")
            {
                PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", sinkAddress);
                sinkApps.Add(sinkHelper.Install(nodes.Get(data.at("Links")[i]["AppFeatures"][j]["NodeDst"])));
            }
          }
        }
    }

    // Start sink applications
    sinkApps.Start(Seconds(start_time));
    NS_LOG_INFO(MAGENTA << "INFO: " << RESET << "Setting sinks finished");
    // Set up source applications
    for (int i = 0; i < (int)data.at("NumLinks"); i++)
    {
        if (data.at("Links")[i]["APP"])
        {
          for (int j = 0; j < (int)data.at("Links")[i]["numApps"]; j++){
             NS_LOG_INFO(MAGENTA << "INFO: " << RESET << "Set APP" << j <<" Tx at node" << i);
            uint16_t port = data.at("Links")[i]["AppFeatures"][j]["RemotePort"];
            std::string protocol = data.at("Links")[i]["AppFeatures"][j]["Protocol"];

            Address sinkAddress = InetSocketAddress(ifacesP2p.at(i).GetAddress(data.at("Links")[i]["AppFeatures"][j]["NodeDst"]), port);

            if (data.at("Links")[i]["AppFeatures"][j]["TypeofApp"] == "Bulk")
            {
                NS_LOG_INFO(MAGENTA << "INFO: " << RESET << "Set a Bulk Tx at node" << i);
                BulkSendHelper ftp(protocol == "TCP" ? "ns3::TcpSocketFactory" : "ns3::UdpSocketFactory", ifacesP2p.at(i).GetAddress(data.at("Links")[i]["AppFeatures"][j]["NodeSrc"]));
                ftp.SetAttribute("Remote", AddressValue(sinkAddress));
                ftp.SetAttribute("SendSize", UintegerValue(data.at("Links")[i]["AppFeatures"][j]["SendSize"]));
                ftp.SetAttribute("MaxBytes", UintegerValue(data.at("Links")[i]["AppFeatures"][j]["Max_bytes"]));

                app.Add(ftp.Install(NodeContainer{nodes.Get(data.at("Links")[i]["AppFeatures"][j]["NodeSrc"])}));
            }
            else if (data.at("Links")[i]["AppFeatures"][j]["TypeofApp"] == "ONOFF")
            {
                NS_LOG_INFO(MAGENTA << "INFO: " << RESET << "Set a ONOFF Tx at node" << i);
                OnOffHelper appUdp(protocol == "TCP" ? "ns3::TcpSocketFactory" : "ns3::UdpSocketFactory", ifacesP2p.at(i).GetAddress(data.at("Links")[i]["AppFeatures"][j]["NodeSrc"]));
                appUdp.SetAttribute("Remote", AddressValue(sinkAddress));
                appUdp.SetAttribute("DataRate", DataRateValue(DataRate(to_string(data.at("Links")[i]["AppFeatures"][j]["DataRate"]) + "Mbps")));
                appUdp.SetAttribute("PacketSize", UintegerValue(data.at("Links")[i]["AppFeatures"][j]["PktSize"]));
                appUdp.SetAttribute("OnTime", UintegerValue(data.at("Links")[i]["AppFeatures"][j]["OnTime"]));
                appUdp.SetAttribute("OffTime", UintegerValue(data.at("Links")[i]["AppFeatures"][j]["OffTime"]));

                app.Add(appUdp.Install(NodeContainer{nodes.Get(data.at("Links")[i]["AppFeatures"][j]["NodeSrc"])}));
            }
          }
        }
    }

    // Start source applications
    app.Start(Seconds(start_time));
}


void InstallTrafficControl(json data, std::vector<NetDeviceContainer> Devs) {

  int link_i = 0;
  TrafficControlHelper tch;

  uint16_t handle = tch.SetRootQueueDisc("ns3::MarkerQueueDisc");
  // TrafficControlHelper::ClassIdList cid = tch.AddQueueDiscClasses(handle, 2, "ns3::QueueDiscClass");
  // tch.AddChildQueueDisc(handle, cid[0], "ns3::RedQueueDisc");
  // tch.AddChildQueueDisc(handle, cid[1], "ns3::RedQueueDisc");

  // Iterate through Links in the JSON
  for (const auto& link : data["Links"]) {
      // Check if 'Policies' is true for this link
      if (link["Policies"].get<bool>()) {
        tch.Install(Devs.at(link_i));
      }
      link_i++;
  }
}





#endif