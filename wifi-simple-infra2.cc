
//test

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleInfra");

int main (int argc, char *argv[])
{
//==========================
time_t  timev;
time(&timev);
RngSeedManager::SetSeed(timev);
RngSeedManager::SetRun (7);

//==========================

  std::string phyMode ("DsssRate11Mbps");
  std::string rtsCts ("1500");
  //double rss = -80;  // -dBm

  double interval = 0.001; // was 1.0 second
  bool verbose = false;
  double rss = -80;  // -dBm
  uint32_t n=4;
  uint32_t maxPacketCount = 320;
  uint32_t MaxPacketSize = 1024;

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

  NodeContainer apContainer;
  NodeContainer staContainer;
  apContainer.Create (1);
  staContainer.Create (n);

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
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  // Setup the rest of the upper mac
  Ssid ssid = Ssid ("wifi-default");
  // setup sta.
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (false));
  NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, staContainer);
  NetDeviceContainer devices = staDevice;

  // setup ap.
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, apContainer.Get (0));
  devices.Add (apDevice);  


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
  mobility.Install (staContainer);

  InternetStackHelper internet;
  internet.Install (apContainer);
  internet.Install (staContainer);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer iap = ipv4.Assign (apDevice);
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  //port number, given in array
  uint16_t port[n];

   //std::cout << "Calculating throughput for " << n << " nodes\n";

   //single server and apps with multiple ports opened
   UdpServerHelper server;
   ApplicationContainer apps;
   for( uint16_t  a = 1; a <= n; a = a + 1 )
   {
       port[a]=8000+a-1;
       server = UdpServerHelper (port[a]);
       apps = server.Install (apContainer.Get (0));       
   }

  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0)); //was 10


  Time interPacketInterval = Seconds (interval);

  UdpClientHelper client;
  for( uint16_t  a = 1; a <= n; a = a + 1 )
  {
      port[a]=8000+a-1;
      client = UdpClientHelper (iap.GetAddress (0), port[a]);
      client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      client.SetAttribute ("Interval", TimeValue (interPacketInterval));
      client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
      apps = client.Install (staContainer.Get (a-1));
      apps.Start (Seconds (2.0));
      apps.Stop (Seconds (10.0)); //was 10         
  }

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  
  wifiPhy.EnablePcap ("wifi-simple-infra", apDevice);

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds(11.0)); //was 11

  Simulator::Run ();

  monitor->CheckForLostPackets ();
  double tot[n];
  double throughput;
  double psent;
  double preceived;

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
/*
	  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

          std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
          std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
*/
	  tot[i->first] =i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024;
	  //std::cout << "  Throughput: " << tot[i->first] <<"\n";

	  throughput=throughput+tot[i->first];
	  psent=psent+i->second.txBytes;
	  preceived=preceived+i->second.rxBytes;
     }

  //std::cout << n << "\t" << throughput/n << "\t" << rtsCts <<"\n";
  std::cout << n<<"\tTotal throughput: " << throughput <<"\n";
  //std::cout << "Average throughput: " << throughput/n <<"\n";
  //std::cout << "Packet loss: " << psent-preceived <<"\n";
  //std::cout << "Packet sent: " << psent <<"\n";
  //std::cout << "Packet received: " << preceived <<"\n";

  //monitor->SerializeToXmlFile("lab-1.flowmon", true, true);

  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
