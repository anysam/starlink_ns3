/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef LEO_SATELLITE_MOBILITY_H
#define LEO_SATELLITE_MOBILITY_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/mobility-model.h"

namespace ns3 {

class LeoSatelliteMobilityModel : public MobilityModel
{
public:

private:
  double m_latitude;
  double m_longitude;
  double m_altitude; //kilometers
  bool m_direction; //each plane will have opposited directions
  
  
}

}

#endif /* LEO_SATELLITE_MOBILITY_H */
