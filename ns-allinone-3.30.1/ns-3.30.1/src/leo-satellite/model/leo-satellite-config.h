/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * LEO Satellite Constellation Config
 * Creates and maintains all satellites and links within a satellite communication network
 *
 * ENSC 427: Communication Networks
 * Spring 2020
 * Team 11
 */
#ifndef LEO_SATELLITE_CONFIG_H
#define LEO_SATELLITE_CONFIG_H

#include "ns3/vector.h"
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/leo-satellite-mobility.h"
#include <vector>
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"

namespace ns3 {

class LeoSatelliteConfig : public Object
{
public:
  /**
   * Register this type with the TypeId system.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  LeoSatelliteConfig (uint32_t num_planes, uint32_t num_satellites_per_plane, double altitude);

  virtual ~LeoSatelliteConfig () = 0;
  
  void UpdateLinks (); //update the intersatellite links
private:
  uint32_t num_planes;
  uint32_t num_satellites_per_plane;
  double m_altitude;

  std::vector<NodeContainer> plane; //node container for each plane
  std::vector<NetDeviceContainer> intra_plane_devices; //contains net devices for all P2P links for all planes
  std::vector<NetDeviceContainer> inter_plane_devices; //the size of this vector should be the number of nodes after configured
  std::vector<Ptr<CsmaChannel>> inter_plane_channels; //the size of this vector should be the number of nodes after configured
  std::vector<uint32_t> inter_plane_channel_tracker; //this will have the node from the adjacent plane that is currently connected
  
};
  
}

#endif /* LEO_SATELLITE_CONFIG_H */
