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

  LeoSatelliteConfig sat_network(9, 4, 2000);
  
  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install(sat_network.ground_stations.Get(1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (2000.0));

  UdpEchoClientHelper echoClient (sat_network.ground_station_interfaces[1].GetAddress(0), 9);
  echoClient.SetAttribute("MaxPackets", UintegerValue (20));
  echoClient.SetAttribute("Interval", TimeValue(Seconds(100.0)));
  echoClient.SetAttribute("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (sat_network.ground_stations.Get(0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (2000.0));

  for(uint32_t i=0; i<19; i++)
  {
    Simulator::Stop(Seconds(100));
    Simulator::Run();
    sat_network.UpdateLinks();
  }

  Simulator::Stop(Seconds(100));
  Simulator::Run();


  /*PointToPointHelper p2p;
  p2p.EnablePcapAll("intra");
  CsmaHelper csma;
  csma.EnablePcapAll("csma");*/

  //Ipv4GlobalRoutingHelper routes;
  //Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);

  //routes.PrintRoutingTableAt(Seconds(0), sat_network.plane[0].Get(2), routingStream);
  //routes.PrintRoutingTableAllAt(Seconds(0), routingStream);
  //routes.PrintRoutingTableAt(Seconds(6), sat_network.plane[0].Get(0), routingStream);
  //Simulator::Stop(Seconds(5));
  //Simulator::Run();
  //sat_network.UpdateLinks();
  //csma.EnablePcap("ground", sat_network.ground_station_devices[0].Get(0), true);

  //Simulator::Stop(Seconds(10));
  //Simulator:: Run ();
  //sat_network.UpdateLinks();

  //Simulator::Run ();

  Simulator::Destroy ();
  return 0; 
}


