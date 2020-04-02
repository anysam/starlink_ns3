/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * LEO Satellite Constellation Config
 * Creates and maintains all satellites and links within a satellite communication network
 *
 * ENSC 427: Communication Networks
 * Spring 2020
 * Team 11
 */

#include "leo-satellite-config.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LeoSatelliteConfig);
NS_LOG_COMPONENT_DEFINE ("LeoSatelliteConfig");

double speed_of_light = 299792458; //in m/s

//typeid
TypeId LeoSatelliteConfig::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LeoSatelliteConfig")
  .SetParent<Object> ()
  .SetGroupName("LeoSatellite")
  ;
  return tid;
}

//constructor
LeoSatelliteConfig::LeoSatelliteConfig (uint32_t num_planes, uint32_t num_satellites_per_plane, double altitude)
{
  this->num_planes = num_planes;
  this->num_satellites_per_plane = num_satellites_per_plane;
  this->m_altitude = altitude;

  uint32_t total_num_satellites = num_planes*num_satellites_per_plane;
  NodeContainer temp;
  temp.Create(total_num_satellites);

  //assign mobility model to all satellites
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::LeoSatelliteMobilityModel",
                             "NPerPlane", IntegerValue (num_satellites_per_plane),
                             "NumberofPlanes", IntegerValue (num_planes),
                             "Altitude", DoubleValue(altitude),
                             "Time", DoubleValue(Simulator::Now().GetSeconds()));
  mobility.Install(temp);
  
  for (NodeContainer::Iterator j = temp.Begin ();
       j != temp.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      Vector null = Vector(0.0, 0.0, 0.0);
      position->SetPosition(null); // needs to be done to initialize
      NS_ASSERT (position != 0);

      // Printing initial position
      Vector pos = position->GetPosition ();
      std::cout << Simulator::Now().GetSeconds() << ": x = " << pos.x << ", y = " << pos.y << ", z = " << pos.z << std::endl;
    }

  //assigning nodes to e/ plane's node container as necessary
  for (uint32_t i=0; i<num_planes; i++)
  {
     NodeContainer temp_plane;
     for (uint32_t j=0; j<num_satellites_per_plane; j++)
     {
       temp_plane.Add(temp.Get(j));
     }
     this->plane.push_back(temp_plane);
  }

  //setting up all intraplane links
  Vector nodeAPosition = this->plane[0].Get(0)->GetObject<MobilityModel>()->GetPosition();
  Vector nodeBPosition = this->plane[0].Get(1)->GetObject<MobilityModel>()->GetPosition();
  double distance = CalculateDistance(nodeAPosition, nodeBPosition);
  double delay = speed_of_light/(distance * 1000); //should get delay in seconds
  PointToPointHelper intraplane_link_helper;
  intraplane_link_helper.SetDeviceAttribute ("DataRate", StringValue ("5Mbps")); //TODO: update this attribute
  intraplane_link_helper.SetChannelAttribute ("Delay", TimeValue(Seconds (delay)));

  for (uint32_t i=0; i<num_planes; i++)
  {
    for (uint32_t j=0; j<num_satellites_per_plane; j++)
    {
      if (j < num_satellites_per_plane - 1)
      {
        this->intra_plane_devices.push_back(intraplane_link_helper.Install(plane[i].Get(j), plane[i].Get(j+1)));
      }
      else
      {
        this->intra_plane_devices.push_back(intraplane_link_helper.Install(plane[i].Get(j), plane[i].Get(0)));
      }
    }
  }

  //setting up interplane links
  for (uint32_t i=0; i<num_planes; i++)
  {
    for (uint32_t j=0; j<num_satellites_per_plane; j++)
    {
      Vector nodeAPos = this->plane[i].Get(j)->GetObject<MobilityModel>()->GetPosition();
      Vector nodeBPos = this->plane[(i+1)%num_planes].Get(j)->GetObject<MobilityModel>()->GetPosition();
      double distance = CalculateDistance(nodeAPos, nodeBPos);
      double delay = speed_of_light/(distance*1000);
      CsmaHelper interplane_link_helper;
      interplane_link_helper.SetChannelAttribute("DataRate", StringValue ("5Mbps")); //TODO: update values
      interplane_link_helper.SetChannelAttribute("Delay", TimeValue(Seconds(delay)));

      NodeContainer temp_node_container;
      temp_node_container.Add(this->plane[i].Get(j));
      temp_node_container.Add(this->plane[(i+1)%num_planes]);
      NetDeviceContainer temp_netdevice_container;
      temp_netdevice_container = interplane_link_helper.Install(temp_node_container);
      Ptr<CsmaChannel> csma_channel;
      Ptr<Channel> channel;
      channel = temp_netdevice_container.Get(0)->GetChannel();
      csma_channel = channel->GetObject<CsmaChannel> ();
      
      for (uint32_t k=0; k<num_satellites_per_plane; k++)
      {
        if (j != k)
        {
          csma_channel->Detach(temp_netdevice_container.Get(k+1)->GetObject<CsmaNetDevice> ());
        }
      }
        
      this->inter_plane_devices.push_back(temp_netdevice_container);
      this->inter_plane_channels.push_back(csma_channel);
      this->inter_plane_channel_tracker.push_back(j);
    }
  }
  //TODO: configure ground stations and channels
  //TODO: set IP addresses
}

void LeoSatelliteConfig::UpdateLinks()
{
  for (uint32_t i=0; i<this->num_planes; i++)
  {
    for (uint32_t j=0; j<this->num_satellites_per_plane; j++)
    {
      uint32_t access_idx = i*(this->num_satellites_per_plane) + j;
      uint32_t currAdjNodeID = this->inter_plane_channel_tracker[access_idx];
      uint32_t nextAdjNodeID1, nextAdjNodeID2;
      double currAdjNodeDist, nextAdjNodeDist1, nextAdjNodeDist2;

      Vector constNodePos = this->plane[i].Get(j)->GetObject<MobilityModel>()->GetPosition();

      if (currAdjNodeID == 0)
      {
        nextAdjNodeID1 = 1;
        nextAdjNodeID2 = this->num_satellites_per_plane - 1;
      }
      else if (currAdjNodeID == this->num_satellites_per_plane - 1)
      {
        nextAdjNodeID1 = 0;
        nextAdjNodeID2 = currAdjNodeID - 1;
      }
      else
      {
        nextAdjNodeID1 = currAdjNodeID + 1;
        nextAdjNodeID2 = currAdjNodeID - 1;
      }

      Vector currAdjNodePos = this->plane[(i+1)%num_planes].Get(currAdjNodeID)->GetObject<MobilityModel>()->GetPosition();
      Vector nextAdjNodePos1 = this->plane[(i+1)%num_planes].Get(nextAdjNodeID1)->GetObject<MobilityModel>()->GetPosition();
      Vector nextAdjNodePos2 = this->plane[(i+1)%num_planes].Get(nextAdjNodeID2)->GetObject<MobilityModel>()->GetPosition();

      currAdjNodeDist = CalculateDistance(constNodePos, currAdjNodePos);
      nextAdjNodeDist1 = CalculateDistance(constNodePos, nextAdjNodePos1);
      nextAdjNodeDist2 = CalculateDistance(constNodePos, nextAdjNodePos2);

      if((currAdjNodeDist < nextAdjNodeDist1) && (currAdjNodeDist < nextAdjNodeDist2))
      {
        double new_delay = speed_of_light/(currAdjNodeDist*1000);
        this->inter_plane_channels[access_idx]->SetAttribute("Delay", TimeValue(Seconds(new_delay)));
      }
      else if (nextAdjNodeDist1 < nextAdjNodeDist2)
      {
        this->inter_plane_channels[access_idx]->Detach(this->inter_plane_devices[access_idx].Get(currAdjNodeID+1)->GetObject<CsmaNetDevice> ());
        this->inter_plane_channels[access_idx]->Reattach(this->inter_plane_devices[access_idx].Get(nextAdjNodeID1+1)->GetObject<CsmaNetDevice> ());
        this->inter_plane_channel_tracker[access_idx] = nextAdjNodeID1;
        double new_delay = speed_of_light/(nextAdjNodeDist1*1000);
        this->inter_plane_channels[access_idx]->SetAttribute("Delay", TimeValue(Seconds(new_delay)));
      }
      else
      {
        this->inter_plane_channels[access_idx]->Detach(this->inter_plane_devices[access_idx].Get(currAdjNodeID+1)->GetObject<CsmaNetDevice> ());
        this->inter_plane_channels[access_idx]->Reattach(this->inter_plane_devices[access_idx].Get(nextAdjNodeID2+1)->GetObject<CsmaNetDevice> ());
        this->inter_plane_channel_tracker[access_idx] = nextAdjNodeID2;
        double new_delay = speed_of_light/(nextAdjNodeDist2*1000);
        this->inter_plane_channels[access_idx]->SetAttribute("Delay", TimeValue(Seconds(new_delay)));
      }    
    }
  }
}


}

