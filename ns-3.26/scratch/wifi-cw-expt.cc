/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 IITP RAS
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
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

/*
 * Classical hidden terminal problem and its RTS/CTS solution.
 *
 * Topology: [node 0] <-- -50 dB --> [node 1] <-- -50 dB --> [node 2]
 * 
 * This example illustrates the use of 
 *  - Wifi in ad-hoc mode
 *  - Matrix propagation loss model
 *  - Use of OnOffApplication to generate CBR stream 
 *  - IP flow monitor
 */
#include "ns3/core-module.h"
#include "ns3/propagation-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"

#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/gnuplot.h"
#include <cmath>

using namespace ns3;

/// Run single 10 seconds experiment with enabled or disabled RTS/CTS mechanism
double experiment (uint32_t nWifi, double cwMin = 64, double cwMax = 256)
{
  // 1. Create 3 nodes 
  NodeContainer nodes;
  nodes.Create (3);

  // 2. Place nodes somehow, this is required by every wireless simulation
  for (size_t i = 0; i < 3; ++i)
    {
      nodes.Get (i)->AggregateObject (CreateObject<ConstantPositionMobilityModel> ());
    }

  // 3. Create propagation loss matrix
  Ptr<MatrixPropagationLossModel> lossModel = CreateObject<MatrixPropagationLossModel> ();
  lossModel->SetDefaultLoss (50); // set default loss to 200 dB (no link)
  lossModel->SetLoss (nodes.Get (0)->GetObject<MobilityModel>(), nodes.Get (1)->GetObject<MobilityModel>(), 50); // set symmetric loss 0 <-> 1 to 50 dB
  lossModel->SetLoss (nodes.Get (2)->GetObject<MobilityModel>(), nodes.Get (1)->GetObject<MobilityModel>(), 50); // set symmetric loss 2 <-> 1 to 50 dB

  // 4. Create & setup wifi channel
  Ptr<YansWifiChannel> wifiChannel = CreateObject <YansWifiChannel> ();
  wifiChannel->SetPropagationLossModel (lossModel);
  wifiChannel->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ());

  // 5. Install wireless devices
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
                                "DataMode",StringValue ("DsssRate2Mbps"), 
                                "ControlMode",StringValue ("DsssRate1Mbps"));
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel);
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac"); // use ad-hoc MAC
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);

  for(uint32_t i = 0; i < nWifi; i++)
  {
    Ptr<Node> node = nodes.Get(i); // Get station from node container 
    Ptr<NetDevice> dev = node->GetDevice(0);
    Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
    Ptr<WifiMac> mac = wifi_dev->GetMac();
	
    PointerValue ptr;
    mac->GetAttribute("DcaTxop", ptr);
	
    Ptr<DcaTxop> dca = ptr.Get<DcaTxop>();
    dca->SetMinCw(cwMin);
    dca->SetMaxCw(cwMax);
  }
  // uncomment the following to have athstats output
  // AthstatsHelper athstats;
  // athstats.EnableAthstats(enableCtsRts ? "rtscts-athstats-node" : "basic-athstats-node" , nodes);

  // uncomment the following to have pcap output
  // wifiPhy.EnablePcap (enableCtsRts ? "rtscts-pcap-node" : "basic-pcap-node" , nodes);


  // 6. Install TCP/IP stack & assign IP addresses
  InternetStackHelper internet;
  internet.Install (nodes);
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.0.0.0");
  ipv4.Assign (devices);

  // 7. Install applications: two CBR streams each saturating the channel 
  ApplicationContainer cbrApps;
  uint16_t cbrPort = 12345;
  OnOffHelper onOffHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address ("10.0.0.2"), cbrPort));
  onOffHelper.SetAttribute ("PacketSize", UintegerValue (1400));
  onOffHelper.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

  // flow 1:  node 0 -> node 1
  onOffHelper.SetAttribute ("DataRate", StringValue ("3000000bps"));
  onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (1.000000)));
  cbrApps.Add (onOffHelper.Install (nodes.Get (0))); 

  // flow 2:  node 2 -> node 1
  /** \internal
   * The slightly different start times and data rates are a workaround
   * for \bugid{388} and \bugid{912}
   */
  onOffHelper.SetAttribute ("DataRate", StringValue ("3001100bps"));
  onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (1.001)));
  cbrApps.Add (onOffHelper.Install (nodes.Get (2))); 

  /** \internal
   * We also use separate UDP applications that will send a single
   * packet before the CBR flows start. 
   * This is a workaround for the lack of perfect ARP, see \bugid{187}
   */
  uint16_t  echoPort = 9;
  UdpEchoClientHelper echoClientHelper (Ipv4Address ("10.0.0.2"), echoPort);
  echoClientHelper.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClientHelper.SetAttribute ("Interval", TimeValue (Seconds (0.1)));
  echoClientHelper.SetAttribute ("PacketSize", UintegerValue (10));
  ApplicationContainer pingApps;

  // again using different start times to workaround Bug 388 and Bug 912
  echoClientHelper.SetAttribute ("StartTime", TimeValue (Seconds (0.001)));
  pingApps.Add (echoClientHelper.Install (nodes.Get (0))); 
  echoClientHelper.SetAttribute ("StartTime", TimeValue (Seconds (0.006)));
  pingApps.Add (echoClientHelper.Install (nodes.Get (2)));




  // 8. Install FlowMonitor on all nodes
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  // 9. Run simulation for 10 seconds
  Simulator::Stop (Seconds (10));
  Simulator::Run ();

  double avgTput = 0;
  // 10. Print per flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      // first 2 FlowIds are for ECHO apps, we don't want to display them
      //
      // Duration for throughput measurement is 9.0 seconds, since 
      //   StartTime of the OnOffApplication is at about "second 1"
      // and 
      //   Simulator::Stops at "second 10".
      if (i->first > 2)
        {
          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
          std::cout << "Flow " << i->first - 2 << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
          std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
          std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
          std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
          std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
          std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
		  avgTput += i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) / 1000 / 1000;
        }
    }
	avgTput = avgTput / stats.size();

  // 11. Cleanup
  Simulator::Destroy ();
  
  return avgTput;
}

int main (int argc, char **argv)
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
    	
  for(uint32_t cwExpMax = 6; cwExpMax <= 8; cwExpMax++){
    double cwMax = pow( 2.0, (double)cwExpMax);
    Gnuplot2dDataset dataset;
    dataset.SetStyle (Gnuplot2dDataset::LINES);
    for(uint32_t cwExpMin = 4; cwExpMin <= 6; cwExpMin++){
        double cwMin = pow( 2.0, (double)cwExpMin);
        double avgTput = experiment(nWifi, cwMin, cwMax);
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
