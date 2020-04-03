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

LeoSatelliteConfig::~LeoSatelliteConfig ()
{
}

TypeId LeoSatelliteConfig::GetInstanceTypeId (void) const
{
  TypeId tid = this->GetTypeId();
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
    }

  //assigning nodes to e/ plane's node container as necessary
  for (uint32_t i=0; i<num_planes; i++)
  {
     NodeContainer temp_plane;
     if (i==0)
     {
       for(uint32_t j=0; j<num_satellites_per_plane/2; j++)
       {
         Vector pos = temp.Get(j)->GetObject<MobilityModel> ()->GetPosition();
         std::cout << Simulator::Now().GetSeconds() << ": plane # "<< i << " node # " <<j<< ": x = " << pos.x << ", y = " << pos.y << ", z = " << pos.z << std::endl;
         temp_plane.Add(temp.Get(j));
       }
       for(uint32_t j=num_satellites_per_plane/2; j> 0; j--)
       {
         Vector pos = temp.Get(total_num_satellites/2 + j - 1)->GetObject<MobilityModel> ()->GetPosition();
         std::cout << Simulator::Now().GetSeconds() << ": plane # "<< i << " node # " <<num_satellites_per_plane - j<< ": x = " << pos.x << ", y = " << pos.y << ", z = " << pos.z << std::endl;
         temp_plane.Add(temp.Get(total_num_satellites/2 + j - 1));
       }
     }
     else
     {
       for (uint32_t j=0; j<num_satellites_per_plane/2; j++)
       {
         Vector pos = temp.Get(i*num_satellites_per_plane/2 + j)->GetObject<MobilityModel> ()->GetPosition();
         std::cout << Simulator::Now().GetSeconds() << ": plane # "<< i << " node # " <<j<< ": x = " << pos.x << ", y = " << pos.y << ", z = " << pos.z << std::endl;
         temp_plane.Add(temp.Get(i*num_satellites_per_plane/2 + j));
       }
       for (uint32_t j=num_satellites_per_plane/2; j>0; j--)
       {
         Vector pos = temp.Get(total_num_satellites- i*num_satellites_per_plane/2 + j - 1)->GetObject<MobilityModel> ()->GetPosition();
         std::cout << Simulator::Now().GetSeconds() << ": plane # "<< i << " node # " <<num_satellites_per_plane - j<< ": x = " << pos.x << ", y = " << pos.y << ", z = " << pos.z << std::endl;
         temp_plane.Add(temp.Get(total_num_satellites- i*num_satellites_per_plane/2 + j - 1));
       }
     }
     this->plane.push_back(temp_plane);
  }

  //setting up all intraplane links
  Vector nodeAPosition = this->plane[0].Get(0)->GetObject<MobilityModel>()->GetPosition();
  Vector nodeBPosition = this->plane[0].Get(1)->GetObject<MobilityModel>()->GetPosition();
  double distance = CalculateDistance(nodeAPosition, nodeBPosition);
  double delay = (distance * 1000)/speed_of_light; //should get delay in seconds
  PointToPointHelper intraplane_link_helper;
  intraplane_link_helper.SetDeviceAttribute ("DataRate", StringValue ("5Mbps")); //TODO: update this attribute
  intraplane_link_helper.SetChannelAttribute ("Delay", TimeValue(Seconds (delay)));

  std::cout<<"Setting up intra-plane links with distance of "<<distance<<" km and delay of "<<delay<<" seconds."<<std::endl;

  for (uint32_t i=0; i<num_planes; i++)
  {
    for (uint32_t j=0; j<num_satellites_per_plane; j++)
    {
      this->intra_plane_devices.push_back(intraplane_link_helper.Install(plane[i].Get(j), plane[i].Get((j+1)%num_satellites_per_plane)));
      std::cout<<"Plane "<<i<<": channel between node "<<j<<" and node "<<(j+1)%num_satellites_per_plane<<std::endl;
    }
  }

  //setting up interplane links
  std::cout<<"Setting up inter-plane links"<<std::endl;
  for (uint32_t i=0; i<num_planes; i++)
  {
    for (uint32_t j=0; j<num_satellites_per_plane; j++)
    {
      Vector nodeAPos = this->plane[i].Get(j)->GetObject<MobilityModel>()->GetPosition();
      Vector nodeBPos = this->plane[(i+1)%num_planes].Get(j)->GetObject<MobilityModel>()->GetPosition();
      double distance = CalculateDistance(nodeAPos, nodeBPos);
      double delay = (distance*1000)/speed_of_light;
      CsmaHelper interplane_link_helper;
      interplane_link_helper.SetChannelAttribute("DataRate", StringValue ("5Mbps")); //TODO: update values
      interplane_link_helper.SetChannelAttribute("Delay", TimeValue(Seconds(delay)));

      std::cout<<"Channel open between plane "<<i<<" satellite "<<j<<" and plane "<<(i+1)%num_planes<<" satellite "<<j<< " with distance "<<distance<< "km and delay of "<<delay<<" seconds"<<std::endl;

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
  std::cout<<std::endl<<std::endl<<std::endl<<"Updating Links"<<std::endl;
  for (uint32_t i=0; i<this->num_planes; i++)
  {
    Vector refSatPos;
    uint32_t refSat;
    //find reference satellite (closest to equator)
    for (uint32_t j=0; j<this->num_satellites_per_plane; j++)
    {
      Vector pos = this->plane[i].Get(j)->GetObject<MobilityModel>()->GetPosition();
      if ((std::abs(pos.x) < std::abs(refSatPos.x)) || j == 0)
      {
        refSatPos = pos;
        refSat = j;
      }
    }

    //find the closest adjacent satellite to the reference satellite
    uint32_t closestAdjSat;
    double closestAdjSatDist = 0;
    for (uint32_t j=0; j<this->num_satellites_per_plane; j++)
    {
      Vector pos = this->plane[(i+1)%num_planes].Get(j)->GetObject<MobilityModel>()->GetPosition();
      double temp_dist = CalculateDistance(refSatPos,pos);
      if((temp_dist < closestAdjSatDist) || (j==0))
      {
        closestAdjSatDist = temp_dist;
        closestAdjSat = j;
      }
    }

    //calculate the reference increment factor for adjacent satellites in a plane
    uint32_t ref_incr;
    (refSat <= closestAdjSat) ? (ref_incr = closestAdjSat - refSat) : (ref_incr = this->num_satellites_per_plane - refSat + closestAdjSat);

    //update all adjacent satellites for this plane
    for (uint32_t j=0; j<this->num_satellites_per_plane; j++)
    {
      uint32_t access_idx = i*(this->num_satellites_per_plane) + j;
      uint32_t currAdjNodeID = this->inter_plane_channel_tracker[access_idx];
      uint32_t nextAdjNodeID = (j + ref_incr)%(this->num_satellites_per_plane);
      double nextAdjNodeDist;

      Vector constNodePos = this->plane[i].Get(j)->GetObject<MobilityModel>()->GetPosition();
      Vector nextAdjNodePos = this->plane[(i+1)%(this->num_planes)].Get(nextAdjNodeID)->GetObject<MobilityModel>()->GetPosition();

      nextAdjNodeDist = CalculateDistance(constNodePos, nextAdjNodePos);

      if(currAdjNodeID == nextAdjNodeID)
      {
        double new_delay = (nextAdjNodeDist*1000)/speed_of_light;
        this->inter_plane_channels[access_idx]->SetAttribute("Delay", TimeValue(Seconds(new_delay)));
        std::cout<<"Channel updated between plane "<<i<<" satellite "<<j<<" and plane "<<(i+1)%num_planes<<" satellite "<<nextAdjNodeID<< " with distance "<<nextAdjNodeDist<< "km and delay of "<<new_delay<<" seconds"<<std::endl;
      }
      else
      {
        this->inter_plane_channels[access_idx]->Detach(this->inter_plane_devices[access_idx].Get(currAdjNodeID+1)->GetObject<CsmaNetDevice> ());
        this->inter_plane_channels[access_idx]->Reattach(this->inter_plane_devices[access_idx].Get(nextAdjNodeID+1)->GetObject<CsmaNetDevice> ());
        this->inter_plane_channel_tracker[access_idx] = nextAdjNodeID;
        double new_delay = (nextAdjNodeDist*1000)/speed_of_light;
        this->inter_plane_channels[access_idx]->SetAttribute("Delay", TimeValue(Seconds(new_delay)));
        std::cout<<"New channel between plane "<<i<<" satellite "<<j<<" and plane "<<(i+1)%num_planes<<" satellite "<<nextAdjNodeID<< " with distance "<<nextAdjNodeDist<< "km and delay of "<<new_delay<<" seconds"<<std::endl;
      }
    }
  }
}


}

