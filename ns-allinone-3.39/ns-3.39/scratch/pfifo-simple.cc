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
 *          100 Mb/s, 0.1 ms       pfifofast       5 Mb/s, 5ms
 *                                 or codel        bottleneck
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
#include "ns3/traffic-control-module.h"
#include "ns3/udp-header.h"

#include <fstream>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Pruebas");

/**
 * Function called when Congestion Window is changed.
 *
 * \param stream Output stream.
 * \param oldval Old value.
 * \param newval New value.
 */
static void
CwndTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
    *stream->GetStream() << oldval << " " << newval << std::endl;
}

/**
 * Function to enable the Congestion window tracing.
 *
 * Note that you can not hook to the trace before the socket is created.
 *
 * \param cwndTrFileName Name of the output file.
 */
static void
TraceCwnd(std::string cwndTrFileName)
{
    AsciiTraceHelper ascii;
    if (cwndTrFileName.empty())
    {
        NS_LOG_DEBUG("No trace file for cwnd provided");
        return;
    }
    else
    {
        Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(cwndTrFileName);
        Config::ConnectWithoutContext(
            "/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
            MakeBoundCallback(&CwndTracer, stream));
    }
}

std::ofstream TxFile;
std::ofstream RxFile;
std::ofstream CwndFile;

static uint32_t contTX = 0;
static uint32_t contRX = 0;
uint32_t pktSize = 1480;
void TxTracer(Ptr<const Packet> pkt)
{
  TxFile << contTX << " " << Simulator::Now().GetSeconds() << " " << pkt->GetSize() << std::endl;
  contTX++;
}

void RxTracer(Ptr<const Packet> pkt, const Address &)
{
  auto rxBytes = pkt->GetSize();
  if (rxBytes < 1460)
  {
    std::cout << "Pkt with size: " << rxBytes << std::endl;
  }
  while (rxBytes)
  {
    RxFile << contRX << " " << Simulator::Now().GetSeconds() << " " << pkt->GetSize() << std::endl;
    rxBytes -= pktSize;
    contRX++;
  }
}
















int
main(int argc, char* argv[])
{
    
    std::string bottleneckBandwidth = "5Mbps";
    std::string bottleneckDelay = "5ms";
    std::string accessBandwidth = "100Mbps";
    std::string accessDelay = "0.1ms";

    std::string queueDiscType = "Sched"; // PfifoFast or CoDel
    uint32_t queueDiscSize = 1000;           // in packets
    uint32_t queueSize = 10;                 // in packets
    uint32_t pktSize = 1458;                 // in bytes. 1458 to prevent fragments
    float startTime = 0.1F;
    float simDuration = 60; // in seconds

    bool isPcapEnabled = true;
    std::string pcapFileName = "sched.pcap";
    std::string cwndTrFileName = "sched.tr";
    std::string outcome_folder = "/scratch/";
    bool logging = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("bottleneckBandwidth", "Bottleneck bandwidth", bottleneckBandwidth);
    cmd.AddValue("bottleneckDelay", "Bottleneck delay", bottleneckDelay);
    cmd.AddValue("accessBandwidth", "Access link bandwidth", accessBandwidth);
    cmd.AddValue("accessDelay", "Access link delay", accessDelay);
    cmd.AddValue("queueDiscType", "Bottleneck queue disc type: PfifoFast, CoDel", queueDiscType);
    cmd.AddValue("queueDiscSize", "Bottleneck queue disc size in packets", queueDiscSize);
    cmd.AddValue("queueSize", "Devices queue size in packets", queueSize);
    cmd.AddValue("pktSize", "Packet size in bytes", pktSize);
    cmd.AddValue("startTime", "Simulation start time", startTime);
    cmd.AddValue("simDuration", "Simulation duration in seconds", simDuration);
    cmd.AddValue("isPcapEnabled", "Flag to enable/disable pcap", isPcapEnabled);
    cmd.AddValue("pcapFileName", "Name of pcap file", pcapFileName);
    cmd.AddValue("cwndTrFileName", "Name of cwnd trace file", cwndTrFileName);
    cmd.AddValue("logging", "Flag to enable/disable logging", logging);
    cmd.Parse(argc, argv);

    std::string name = "TCP"; 
    RxFile.open(outcome_folder + "RxFile" + name + ".log", std::fstream::out);           // ofstream
    TxFile.open(outcome_folder + "TxFile" + name + ".log", std::fstream::out);           // ofstream



    float stopTime = startTime + simDuration;
    LogComponentEnable("SchedQueueDisc", LOG_LEVEL_ALL);
    // LogComponentEnable("TrafficControlLayer", LOG_LEVEL_ALL);
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
                       QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, queueSize)));

    // Create gateway, source, and sink
    NodeContainer gateway;
    gateway.Create(1);
    NodeContainer source;
    source.Create(1);
    NodeContainer sink;
    sink.Create(1);

    // Create and configure access link and bottleneck link
    PointToPointHelper accessLink;
    accessLink.SetDeviceAttribute("DataRate", StringValue(accessBandwidth));
    accessLink.SetChannelAttribute("Delay", StringValue(accessDelay));

    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue(bottleneckBandwidth));
    bottleneckLink.SetChannelAttribute("Delay", StringValue(bottleneckDelay));

    InternetStackHelper stack;
    stack.InstallAll();

    // Access link traffic control configuration
    TrafficControlHelper tch;
    uint16_t handle = tch.SetRootQueueDisc("ns3::PfifoFastQueueDisc");
    tch.AddInternalQueues(handle, 2,"ns3::PieQueueDisc");
    // tch.AddChildQueueDisc(handle, cid[0], "ns3::FqCoDelFlow");
    // tch.AddChildQueueDisc(handle, cid[1], "ns3::FqCobaltQueueDisc");
      



    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");

    // Configure the source and sink net devices
    // and the channels between the source/sink and the gateway
    Ipv4InterfaceContainer sinkInterface;

    NetDeviceContainer devicesAccessLink;
    NetDeviceContainer devicesBottleneckLink;

    devicesAccessLink = accessLink.Install(source.Get(0), gateway.Get(0));
    tch.Install(devicesAccessLink);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces = address.Assign(devicesAccessLink);

    devicesBottleneckLink = bottleneckLink.Install(gateway.Get(0), sink.Get(0));
    address.NewNetwork();

   
    interfaces = address.Assign(devicesBottleneckLink);

    sinkInterface.Add(interfaces.Get(1));

    NS_LOG_INFO("Initialize Global Routing.");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // APP - 1 (TCP)
    uint16_t port = 50000;
    Address sinkLocalAddress1(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper sinkHelper1("ns3::TcpSocketFactory", sinkLocalAddress1);

    // APP - 2 (UDP)
    uint16_t port2 = 50001;
    Address sinkLocalAddress2(InetSocketAddress(Ipv4Address::GetAny(), port2));
    PacketSinkHelper sinkHelper2("ns3::UdpSocketFactory", sinkLocalAddress2);

    // Configure application - 1 (TCP)
    AddressValue remoteAddress1(InetSocketAddress(sinkInterface.GetAddress(0, 0), port));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(pktSize));

    BulkSendHelper ftp("ns3::TcpSocketFactory", Address());
    ftp.SetAttribute("Remote", remoteAddress1);
    ftp.SetAttribute("SendSize", UintegerValue(pktSize));
    ftp.SetAttribute("MaxBytes", UintegerValue(0));

    // Configure application - 2 (UDP)
    AddressValue remoteAddress2(InetSocketAddress(sinkInterface.GetAddress(0, 0), port2));

    OnOffHelper appUdp("ns3::UdpSocketFactory", Address());
    appUdp.SetAttribute("Remote", remoteAddress2);
    appUdp.SetAttribute("DataRate", DataRateValue(DataRate("10Mbps")));  // Set UDP data rate
    appUdp.SetAttribute("PacketSize", UintegerValue(pktSize));        // Set UDP packet size

    // Start app-source - 1 (TCP)
    ApplicationContainer sourceAppTcp = ftp.Install(source.Get(0));
    sourceAppTcp.Start(Seconds(0));
    sourceAppTcp.Stop(Seconds(stopTime - 3));

    // Start app-source - 2 (UDP)
    ApplicationContainer sourceAppUdp = appUdp.Install(source.Get(0));
    sourceAppUdp.Start(Seconds(0));
    sourceAppUdp.Stop(Seconds(stopTime));

    // Start app-sink (both TCP and UDP)
    sinkHelper1.SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));
    sinkHelper2.SetAttribute("Protocol", TypeIdValue(UdpSocketFactory::GetTypeId()));
    ApplicationContainer sinkApp = sinkHelper1.Install(sink);
    sinkApp.Add(sinkHelper2.Install(sink));
    sinkApp.Start(Seconds(0));
    sinkApp.Stop(Seconds(stopTime));
    Simulator::Schedule(Seconds(0.00001), &TraceCwnd, cwndTrFileName);



    if (isPcapEnabled)
    {
        accessLink.EnablePcap(pcapFileName, nodes.Get(2), true);
    }

    Simulator::Stop(Seconds(stopTime));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
