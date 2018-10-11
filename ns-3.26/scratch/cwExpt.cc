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
#include "ns3/gnuplot.h"
#include <cmath>

// Default Network Topology
//
// ------------------
//   Wifi 10.1.1.0
//                 AP
//  *    *    *    *
//  |    |    |    |
//  n1   n2   n3   n0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

double run_expt(uint32_t nWifi, double cwMin = 16, double cwMax = 64){
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode;
  wifiApNode.Create(1);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  for(uint32_t i = 0; i < nWifi; i++)
  {
    Ptr<Node> node = wifiStaNodes.Get(i); // Get station from node container 
    Ptr<NetDevice> dev = node->GetDevice(0);
    Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
    Ptr<WifiMac> mac = wifi_dev->GetMac();
	
    PointerValue ptr;
    mac->GetAttribute("DcaTxop", ptr);
	
    Ptr<DcaTxop> dca = ptr.Get<DcaTxop>();
    dca->SetMinCw(cwMin);
    dca->SetMaxCw(cwMax);
  }

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
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiInterfaces;
  wifiInterfaces = address.Assign (staDevices);
  address.Assign (apDevices);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (wifiStaNodes.Get(0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (wifiInterfaces.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (30));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (4096));

  ApplicationContainer clientApps = 
    echoClient.Install (wifiStaNodes.Get (nWifi - 1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));
  
  UdpEchoClientHelper echoClient2 (wifiInterfaces.GetAddress (0), 9);
  echoClient2.SetAttribute ("MaxPackets", UintegerValue (30));
  echoClient2.SetAttribute ("Interval", TimeValue (Seconds (0.0)));
  echoClient2.SetAttribute ("PacketSize", UintegerValue (4096));

  ApplicationContainer clientApps2 = 
    echoClient.Install (wifiStaNodes.Get (nWifi - 2));
  clientApps2.Start (Seconds (2.01));
  clientApps2.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

/* Adding a flow monitor to compute throughput */  
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  
  Simulator::Stop (Seconds (10.0));

  Simulator::Run ();
  
  double avgTput = 0;
  
/* Reading from the flow monitor */
  monitor->CheckForLostPackets();
  
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
  
  for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i ){
     Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
     std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
     std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
     std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
     std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
     std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) / 1000 / 1000  << " Mbps\n";
     
     avgTput += i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) / 1000 / 1000;
  }
  
  avgTput = avgTput / stats.size();
  
  Simulator::Destroy ();
  
  return avgTput;
}

int 
main (int argc, char *argv[])
{
  uint32_t nWifi = 3;

  CommandLine cmd;
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);

  cmd.Parse (argc,argv);
  
  std::ofstream plotFile ("cwExpt.plt");
  GnuplotCollection gnuplots ("cwExpt.pdf");
  Gnuplot plot;
  plot.AppendExtra ("set xlabel 'cwMin'");
  plot.AppendExtra ("set ylabel 'Throughput (Mbps)'");
  plot.AppendExtra ("set key outside");
    	
  for(uint32_t cwExpMax = 4; cwExpMax <= 6; cwExpMax++){
    double cwMax = pow( 2.0, (double)cwExpMax);
    Gnuplot2dDataset dataset;
    dataset.SetStyle (Gnuplot2dDataset::LINES);
    for(uint32_t cwExpMin = 2; cwExpMin <= 4; cwExpMin++){
        double cwMin = pow( 2.0, (double)cwExpMin);
        double avgTput = run_expt(nWifi, cwMin, cwMax);
        dataset.Add(cwMin, avgTput);
  
    }
    std::ostringstream os;
    os << "cwMax : " << cwMax ;
    dataset.SetTitle (os.str ());
    plot.AddDataset(dataset);
  }

  plot.SetTitle ("Varying CW sizes");
  gnuplots.AddPlot (plot);
  gnuplots.GenerateOutput (plotFile);
  plotFile.close();
  
  return 0;
}
