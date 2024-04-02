#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("setup1");

std::ofstream TxFile;
std::ofstream RxFile;
uint32_t contTX = 0;
uint32_t contRX = 0;


void TxTracer(Ptr<const Packet> pkt)
{
  TxFile << contTX << " " << Simulator::Now().GetSeconds() << " " << pkt->GetSize() << std::endl;
  contTX++;
}

void RxTracer(Ptr<const Packet> pkt, const Address& from)
{
 
  RxFile << contRX << " " << Simulator::Now().GetSeconds() << " "<< pkt->GetSize() << std::endl;
  contRX++;
}

int main(int argc, char *argv[])
{
  uint32_t pktSize = 1458; 
  std::string folder = "./sim_results/";
  RxFile.open(folder + "RxFile.log");
  TxFile.open(folder + "TxFile.log");

  LogComponentEnable("setup1", LOG_LEVEL_INFO);
  LogComponentEnable("MarkerQueueDisc", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create(4); // 1 source, 1 router marking, 1 QoS router, 1 sink

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
  pointToPoint.SetChannelAttribute("Delay", StringValue("0ms"));
  pointToPoint.SetDeviceAttribute("Mtu", UintegerValue(10000));
  // pointToPoint.SetDeviceAttribute("Mtu", UintegerValue(1460));
  pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", ns3::QueueSizeValue(QueueSize(QueueSizeUnit::BYTES, (0))));
  // pointToPointBottleneck.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  // pointToPointBottleneck.SetChannelAttribute("Delay", StringValue("5ms"));
  // pointToPointBottleneck.SetDeviceAttribute("Mtu", UintegerValue(1460));

  NetDeviceContainer devicesSourceRouter = pointToPoint.Install(NodeContainer(nodes.Get(0), nodes.Get(1)));
  NetDeviceContainer devicesRouterQoS = pointToPoint.Install(NodeContainer(nodes.Get(1), nodes.Get(2)));
  NetDeviceContainer devicesQoSSink = pointToPoint.Install(NodeContainer(nodes.Get(2), nodes.Get(3)));

  InternetStackHelper stack;
  stack.Install(nodes);

  Ipv4AddressHelper addressSourceRouter, addressRouterQoS, addressQoSSink;
  addressSourceRouter.SetBase("10.1.1.0", "255.255.255.0");
  addressRouterQoS.SetBase("10.1.2.0", "255.255.255.0");
  addressQoSSink.SetBase("10.1.3.0", "255.255.255.0");

  Ipv4InterfaceContainer interfacesSourceRouter = addressSourceRouter.Assign(devicesSourceRouter);
  Ipv4InterfaceContainer interfacesRouterQoS = addressRouterQoS.Assign(devicesRouterQoS);
  Ipv4InterfaceContainer interfacesQoSSink = addressQoSSink.Assign(devicesQoSSink);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  ApplicationContainer apps, sinkApps;
  ApplicationContainer Client_app, Server_app;
  // Create a packet sink on the end of the chain
  Address Server_Address(InetSocketAddress(interfacesQoSSink.GetAddress(1,0), 50000));
  PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", Address(Server_Address));
  Server_app.Add(packetSinkHelper.Install(nodes.Get(3))); // Last node server
  Server_app.Start(Seconds(0));

  // Create the OnOff APP to send TCP to the server
  OnOffHelper clientHelper("ns3::UdpSocketFactory", Address(Server_Address));
  // Config::SetDefault("ns3::UdpSocket::SegmentSize", UintegerValue(pktSize));
  clientHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelper.SetAttribute("DataRate", StringValue("3Mbps"));
  // clientHelper.SetAttribute("MaxBytes", UintegerValue(100e6));
  clientHelper.SetAttribute("PacketSize", UintegerValue(pktSize));
  Client_app.Add(clientHelper.Install(NodeContainer{nodes.Get(0)}));
  Client_app.Start(Seconds(0));
  NS_LOG_INFO("Simulation starts here!");

  // pointToPoint.EnablePcapAll("p2p.pcap");
  Config::ConnectWithoutContext("/NodeList/0/ApplicationList/*/$ns3::OnOffApplication/Tx", MakeCallback(&TxTracer));
  Config::ConnectWithoutContext("/NodeList/3/ApplicationList/0/$ns3::PacketSink/Rx", MakeCallback(&RxTracer));
  Simulator::Stop(Seconds(3));
  Simulator::Run();
  Simulator::Destroy();
  NS_LOG_INFO("Simulation finishes");
  RxFile.close();
  TxFile.close();

  return 0;
}

