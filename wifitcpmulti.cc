#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/applications-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleInfra");

int main (int argc, char *argv[])
{
  double simulationTime = 10; //seconds
//==========================
//time_t  timev;
//time(&timev);
//RngSeedManager::SetSeed(timev);
//RngSeedManager::SetRun (7);

//==========================

  std::string phyMode ("DsssRate11Mbps");
  std::string rtsCts ("150");

  double interval = 0.001; // was 1.0 second
  bool verbose = false;
  double rss = -80;  // -dBm
  uint32_t n=10;
  uint32_t m=10;
  uint32_t maxPacketCount = 320;
  uint32_t MaxPacketSize = 1024;
  uint32_t payloadSize = 1024;
  CommandLine cmd;

  cmd.AddValue ("n", "number of nodes", n);
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  //cmd.AddValue ("rss", "received signal strength", rss);
  cmd.AddValue ("packetSize", "size of application packet sent", MaxPacketSize);
  cmd.AddValue ("packetCount", "size of application packet sent", maxPacketCount);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("rtsCts", "RTS/CTS threshold", rtsCts);

  cmd.Parse (argc, argv);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue (rtsCts));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", 
                      StringValue (phyMode));

  NodeContainer apContainer, apContainer2;
  NodeContainer staContainer, staContainer2;
  apContainer.Create (1);
  apContainer2.Create (1);
  staContainer.Create (n);
  staContainer2.Create (m);


  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (0) ); 
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  NqosWifiMacHelper wifiMac2 = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  // Setup the rest of the upper mac
  Ssid ssid = Ssid ("ssid1");
  Ssid ssid2 = Ssid ("ssid2");
  // setup sta.
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (true));
  wifiMac2.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid2),
                   "ActiveProbing", BooleanValue (true));
  NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, staContainer);
  NetDeviceContainer staDevice2 = wifi.Install (wifiPhy, wifiMac2, staContainer2);
  //NetDeviceContainer devices = staDevice;

  // setup ap.
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid2));
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, apContainer.Get (0));
  NetDeviceContainer apDevice2 = wifi.Install (wifiPhy, wifiMac, apContainer2.Get (0));
  //devices.Add (apDevice);  


  // Note that with FixedRssLossModel, the positions below are not 
  // used for received signal strength. 
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  for( uint16_t  a = 1; a <= n; a = a + 1 )
   {
  	positionAlloc->Add (Vector (5.0, 0.0, 0.0));
   }

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (apContainer);
  mobility.Install (apContainer2);
  mobility.Install (staContainer);
  mobility.Install (staContainer2);

  InternetStackHelper internet;
  internet.Install (apContainer);
  internet.Install (apContainer2);
  internet.Install (staContainer);
  internet.Install (staContainer2);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer iap = ipv4.Assign (apDevice);
  Ipv4InterfaceContainer i = ipv4.Assign (staDevice);

  Ipv4AddressHelper ipv42;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv42.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer iap2 = ipv42.Assign (apDevice2);
  Ipv4InterfaceContainer i2 = ipv42.Assign (staDevice2);

  //port number, given in array
  uint16_t port[n];

  ApplicationContainer apps[n], sinkApp[n];

	for( uint16_t  a = 0; a < n; a = a + 1 )
   	{
	  port[a]=8000+a;
//sink = ns3.PacketSinkHelper("ns3::UdpSocketFactory", ns3.InetSocketAddress (ipcontainer.GetAddress (1), port))
          Address apLocalAddress (InetSocketAddress(i.GetAddress (a), port[a]));
          PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress(iap.GetAddress (0), port[a]));
          sinkApp[a] = packetSinkHelper.Install (apContainer.Get (0));

          sinkApp[a].Start (Seconds (0.0));
          sinkApp[a].Stop (Seconds (simulationTime+1));

          OnOffHelper onoff ("ns3::TcpSocketFactory",InetSocketAddress(i.GetAddress (a), port[a]));

          onoff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
          onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
          onoff.SetAttribute ("PacketSize", UintegerValue (payloadSize));
          onoff.SetAttribute ("DataRate", StringValue ("5Mbps")); //bit/s

          AddressValue remoteAddress (InetSocketAddress (iap.GetAddress (0), port[a]));
          onoff.SetAttribute ("Remote", remoteAddress);

          apps[a].Add (onoff.Install (staContainer.Get (a)));

          apps[a].Start (Seconds (1.0));
          apps[a].Stop (Seconds (simulationTime+1));
	}



  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  wifiPhy.EnablePcap ("wifitcp", apDevice);
  wifiPhy.EnablePcap ("wifitcp3", staDevice);
  wifiPhy.EnablePcap ("wifitcp2", apDevice2);
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  Simulator::Stop (Seconds (simulationTime+1));
  Simulator::Run ();

  monitor->CheckForLostPackets ();
  double tot=0;
  double throughput[2*n]; //for every node, there are 2 flows in TCP
  double psent=0;
  double preceived=0;

  std::cout << n << "\t" << rtsCts <<"\t";
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
	  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
          std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
          std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
	  throughput[i->first] = i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024;
      	  std::cout << "  Throughput: " << throughput[i->first]  << " Mbps\n";

	if (t.destinationAddress=="10.1.1.1")
	 {
	  tot=tot+throughput[i->first];
	  psent=psent+i->second.txBytes;
	  preceived=preceived+i->second.rxBytes;
	 }
     }
/*
  std::cout << tot <<"\t";
  std::cout << tot/n <<"\t";
  std::cout << psent <<"\t";
  std::cout << preceived <<"\t";
  std::cout << preceived/psent << "\n";
*/
  //std::cout << n << "\t" << throughput/n << "\t" << rtsCts <<"\n";
  //std::cout << "Total throughput: " << tot <<"\n";
  //std::cout << "Average throughput: " << tot/n <<"\n";
  //std::cout << "Packet loss: " << psent-preceived <<"\n";
  //std::cout << "Packet sent: " << psent <<"\n";
  //std::cout << "Packet received: " << preceived <<"\n";
  //monitor->SerializeToXmlFile("lab-1.flowmon", true, true);

  Simulator::Destroy ();
  return 0;
}
