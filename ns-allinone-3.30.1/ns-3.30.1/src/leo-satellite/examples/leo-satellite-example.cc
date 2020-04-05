/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/leo-satellite-config.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LeoSatelliteExample");

int 
main (int argc, char *argv[])
{
  //bool verbose = true;

  CommandLine cmd;
  //cmd.AddValue ("verbose", "Tell application to log if true", verbose);

  cmd.Parse (argc,argv);

  LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

  LeoSatelliteConfig sat_network(10, 12, 2000);
  
  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install(sat_network.plane[0].Get(1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (sat_network.intraplaneInterfaces.GetAddress(1), 9);
  echoClient.SetAttribute("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
  echoClient.SetAttribute("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (sat_network.plane[0].Get(0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  PointToPointHelper p2p;
  p2p.EnablePcap("intra", sat_network.intra_plane_devices[0].Get(1), true);
  //CsmaHelper csma;
  //csma.EnablePcapAll ("ground");
  //csma.EnablePcap("ground", sat_network.ground_station_devices[0].Get(1), true);

  //Simulator::Stop(Seconds(10));
  Simulator:: Run ();
  //sat_network.UpdateLinks();

  //Simulator::Run ();
  Simulator::Destroy ();
  return 0; 
}


