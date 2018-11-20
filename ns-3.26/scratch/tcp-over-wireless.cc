/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/lognormal-shadowing-loss-model.h"

// Default Network Topology
//
// Number of wifi or csma nodes can be increased up to 250
//                          |
//                 Rank 0   |   Rank 1
// -------------------------|----------------------------
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpOverWireless");

int 
main (int argc, char *argv[])
{
  uint32_t nCsma = 3;
  uint32_t nWifi = 3;
  uint32_t payloadSize = 1472;                       /* Transport layer payload size in bytes. */
  std::string dataRate = "100Mbps";                  /* Application layer datarate. */

  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Application data ate", dataRate);

  cmd.Parse (argc,argv);

  //Configure TCP Options
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
  
  // Check for valid number of csma or wifi nodes
  // 250 should be enough, otherwise IP addresses 
  // soon become an issue
  if (nWifi > 250 || nCsma > 250)
    {
      std::cout << "Too many wifi or csma nodes, no more than 250 each." << std::endl;
      return 1;
    }

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = p2pNodes.Get (0);

  //Create propagation loss matrix
  Ptr<LogNormalPropagationLossModel> lossModel = CreateObject<LogNormalPropagationLossModel> ();
  lossModel->SetAttribute("Exponent", DoubleValue(2.5));
  lossModel->SetAttribute("RandomVariable", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=2.0]"));
  
  //Create & setup wifi channel
  Ptr<YansWifiChannel> wifiChannel = CreateObject <YansWifiChannel> ();
  wifiChannel->SetPropagationLossModel (lossModel);
  wifiChannel->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ());
  
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (wifiChannel);

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiInterfaces;
  wifiInterfaces = address.Assign (staDevices);
  address.Assign (apDevices);
  
  
  //Application
  
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer sinkApp = sinkHelper.Install (p2pNodes.Get (0));
  Ptr<PacketSink> sink = StaticCast<PacketSink> (sinkApp.Get (0));
  
  Ptr<OnOffApplication> server = CreateObject<OnOffApplication> ();
  server->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
  server->SetAttribute ("Remote", AddressValue (InetSocketAddress (p2pInterfaces.GetAddress (0), 9)));
  server->SetAttribute ("PacketSize", UintegerValue (payloadSize));
  server->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  server->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  server->SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  Ptr<Node> serverNode = csmaNodes.Get (nCsma);
  serverNode->AddApplication(server);
  
  sinkApp.Start (Seconds (0.0));
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (10.0));

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();


  Simulator::Run ();
  Simulator::Destroy ();
  
  //Print per flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
		std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) / 1000 / 1000  << " Mbps\n";
        std::cout << "  PDR   " << (i->second.rxBytes * 1.0) / (i->second.txBytes *1.0) << "\n";
        std::cout << "  Mean delay:   " << i->second.delaySum.GetSeconds () / i->second.rxPackets << "\n";
    }

  return 0;
}
