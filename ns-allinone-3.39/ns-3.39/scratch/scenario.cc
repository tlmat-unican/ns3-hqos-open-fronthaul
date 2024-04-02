
/*
 * Copyright (c) 2014 ResiliNets, ITTC, University of Kansas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Truc Anh N Nguyen <trucanh524@gmail.com>
 * Modified by:   Pasquale Imputato <p.imputato@gmail.com>
 *
 */



/*
 * This is a basic example that compares CoDel and PfifoFast queues using a simple, single-flow
 * topology:
 *
 * source -------------------------- router ------------------------ sink
 *          100 Mb/s, 0.1 ms                      5 Mb/s, 5ms
 *                                                 bottleneck
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
#include "json.hpp"
#include "scenario-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>


using namespace ns3;
using json = nlohmann::json;


// NS_LOG_COMPONENT_DEFINE("Scenario");

// /**
//  * Function called when Congestion Window is changed.
//  *
//  * \param stream Output stream.
//  * \param oldval Old value.
//  * \param newval New value.
//  */
// static void
// CwndTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
// {
//     *stream->GetStream() << oldval << " " << newval << std::endl;
// }

// /**
//  * Function to enable the Congestion window tracing.
//  *
//  * Note that you can not hook to the trace before the socket is created.
//  *
//  * \param cwndTrFileName Name of the output file.
//  */
// static void
// TraceCwnd(std::string cwndTrFileName)
// {
//     AsciiTraceHelper ascii;
//     if (cwndTrFileName.empty())
//     {
//         NS_LOG_DEBUG("No trace file for cwnd provided");
//         return;
//     }
//     else
//     {
//         Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(cwndTrFileName);
//         Config::ConnectWithoutContext(
//             "/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
//             MakeBoundCallback(&CwndTracer, stream));
//     }
// }

// std::ofstream TxFile;
// std::ofstream RxFile;
// std::ofstream CwndFile;

// static uint32_t contTX = 0;
// static uint32_t contRX = 0;
// uint32_t pktSize = 1480;
// void TxTracer(Ptr<const Packet> pkt)
// {
//   TxFile << contTX << " " << Simulator::Now().GetSeconds() << " " << pkt->GetSize() << std::endl;
//   contTX++;
// }

// void RxTracer(Ptr<const Packet> pkt, const Address &)
// {
//   auto rxBytes = pkt->GetSize();
//   if (rxBytes < 1460)
//   {
//     std::cout << "Pkt with size: " << rxBytes << std::endl;
//   }
//   while (rxBytes)
//   {
//     RxFile << contRX << " " << Simulator::Now().GetSeconds() << " " << pkt->GetSize() << std::endl;
//     rxBytes -= pktSize;
//     contRX++;
//   }
// }





int
main(int argc, char* argv[])
{
    std::string JSONpath = "./scratch/scen.json";
               
    float startTime = 0.1F;
    float simDuration = 60; // in seconds

    bool isPcapEnabled = true;
    std::string pcapFileName = "sched.pcap";
    std::string cwndTrFileName = "sched.tr";
    std::string outcome_folder = "/scratch/";
    bool logging = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("startTime", "Simulation start time", startTime);
    cmd.AddValue("simDuration", "Simulation duration in seconds", simDuration);
    cmd.AddValue("isPcapEnabled", "Flag to enable/disable pcap", isPcapEnabled);
    cmd.AddValue("pcapFileName", "Name of pcap file", pcapFileName);
    cmd.AddValue("cwndTrFileName", "Name of cwnd trace file", cwndTrFileName);
    cmd.AddValue("logging", "Flag to enable/disable logging", logging);
    cmd.Parse(argc, argv);

   

    std::ifstream f(JSONpath);
    json data = json::parse(f);

    // std::string name = "TCP"; 
    // RxFile.open(outcome_folder + "RxFile" + name + ".log", std::fstream::out);           // ofstream
    // TxFile.open(outcome_folder + "TxFile" + name + ".log", std::fstream::out);           // ofstream



    float stopTime = startTime + simDuration;
    LogComponentEnable("Scenario", LOG_LEVEL_INFO);
    LogComponentEnable("MarkerQueueDisc", LOG_LEVEL_INFO);
    if (logging)
    {
        LogComponentEnable("CoDelPfifoFastBasicTest", LOG_LEVEL_ALL);
        LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
        LogComponentEnable("PfifoFastQueueDisc", LOG_LEVEL_ALL);
        LogComponentEnable("CoDelQueueDisc", LOG_LEVEL_ALL);
    }

    // Enable checksum
    if (isPcapEnabled)
    {
        GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));
    }

    // Devices queue configuration
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize",
                       QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, 0)));


    NodeContainer nodes;
    int auxnodes;
    PointToPointHelper pointToPoint;
    std::vector<NetDeviceContainer> Devs;
    ManageMiddleLinks(data, pointToPoint, Devs, nodes, auxnodes); // To handle middle links and create them

    
    InternetStackHelper stack;
    for (int i = 0; i < auxnodes; i++)
    {
        stack.Install(nodes.Get(i));
    }

    // CREATING - TRAFFIC CONTROL  ----------------------------------------------------------------
    // Creating policies in each node
    InstallTrafficControl(data,Devs);
    NS_LOG_INFO(MAGENTA << "INFO: " << RESET << "Internet Stack >> finished");
    // CREATING - NETWORKS ------------------------------------------------------------------------
    // Creating middle networks - Links
    std::vector<Ipv4InterfaceContainer> ifacesP2p;
    SettingupMiddleNetworks(ifacesP2p, Devs, data);
    // CREATING - APPS ----------------------------------------------------------------------------
    // Creating apps
    ApplicationContainer app, sinkApps;
    InstallApps(app, sinkApps, data, ifacesP2p, nodes); // To generate background traffic

    Simulator::Stop(Seconds(stopTime));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
 