/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 TEI of Western Macedonia, Greece
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
 * Author: Dimitrios J. Vergados <djvergad@gmail.com>
 */

// Network topology
//
//       n0 ----------- n1
//            500 Kbps
//             5 ms
//
#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/dash-module.h"

#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/lte-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("108062586_project3");

int
main (int argc, char *argv[])
{
  bool tracing = false;
  uint32_t maxBytes = 100;
  
  uint32_t users = 1;
  uint16_t numberOfEnbs = 1;

  double target_dt = 35.0;
  double stopTime = 100.0;
  std::string linkRate = "500Kbps";
  std::string delay = "5ms";
  std::string algorithm = "ns3::DashClient";
  uint32_t bufferSpace = 30000000;
  std::string window = "10s";
  double enbTxPowerDbm = 46.0;

  double distance = 100.0;
  double yForUe = 10.0;
  double speed = 20;       // m/s

  /*  LogComponentEnable("MpegPlayer", LOG_LEVEL_ALL);*/
  /* LogComponentEnable ("DashServer", LOG_LEVEL_ALL);
  LogComponentEnable ("DashClient", LOG_LEVEL_ALL);*/

  //
  // Allow the user to override any of the defaults at
  // run-time, via command-line arguments
  //
  CommandLine cmd;
  cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
  cmd.AddValue ("maxBytes", "Total number of bytes for application to send", maxBytes);
  cmd.AddValue ("users", "The number of concurrent videos", users);
  cmd.AddValue ("targetDt", "The target time difference between receiving and playing a frame.",
                target_dt);
  cmd.AddValue ("stopTime", "The time when the clients will stop requesting segments", stopTime);
  cmd.AddValue ("linkRate",
                "The bitrate of the link connecting the clients to the server (e.g. 500kbps)",
                linkRate);
  cmd.AddValue ("delay", "The delay of the link connecting the clients to the server (e.g. 5ms)",
                delay);
  cmd.AddValue ("algorithms",
                "The adaptation algorithms used. It can be a comma seperated list of"
                "protocolos, such as 'ns3::FdashClient,ns3::OsmpClient'."
                "You may find the list of available algorithms in src/dash/model/algorithms",
                algorithm);
  cmd.AddValue ("bufferSpace", "The space in bytes that is used for buffering the video",
                bufferSpace);
  cmd.AddValue ("window", "The window for measuring the average throughput (Time).", window);
  cmd.Parse (argc, argv);


/////////////////////////////////////////////////////////////////////////////
//                      LTE topology                                       //

/////////////////////////////////////////////////////////////////////////////

//create lte
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

  lteHelper->SetHandoverAlgorithmType ("ns3::A2A4RsrqHandoverAlgorithm");
  lteHelper->SetHandoverAlgorithmAttribute ("ServingCellThreshold",
                                            UintegerValue (30));
  lteHelper->SetHandoverAlgorithmAttribute ("NeighbourCellOffset",
                                            UintegerValue (1));
  // create epc
  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet ( RemoteHost <----> epc )
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  pointToPoint.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  pointToPoint.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = pointToPoint.Install (pgw, remoteHost);

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  // Routing of the Internet Host (towards the LTE network)
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  // interface 0 is localhost, 1 is the p2p device
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create (numberOfEnbs);
  ueNodes.Create (users);

  // Install Mobility Model in eNB
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfEnbs; i++)
    {
      Vector enbPosition (distance * (i + 1), distance, 0);
      enbPositionAlloc->Add (enbPosition);
    }
  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator (enbPositionAlloc);
  enbMobility.Install (enbNodes);

  // Install Mobility Model in UE
  MobilityHelper ueMobility;
  ueMobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  ueMobility.Install (ueNodes);
  for (uint16_t i = 0; i < users; i++)
    {
      // lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(0));
      ueNodes.Get (i)->GetObject<MobilityModel> ()->SetPosition (Vector (0 + (5*i), yForUe, 0));
      ueNodes.Get (i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));
    }
  // ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0, yForUe, 0));
  // ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));

// -------------------- 1207 version
//   Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  
// /*
//   for (uint16_t i = 0; i < numberOfEnbs; i++)
//     {
//       Vector enbPosition (-150+(distance * (i + 1)), distance, 100); //distance=100
//       enbPositionAlloc->Add (enbPosition);
//     }
// */
//   Vector enbPosition (-150+(distance * 1), distance, 100); //distance=100
//   enbPositionAlloc->Add (enbPosition);
//   MobilityHelper enbMobility;
//   enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
//   enbMobility.SetPositionAllocator (enbPositionAlloc);
//   enbMobility.Install (enbNodes);

//   // Install Mobility Model in UE
//   MobilityHelper ueMobility;
//   ueMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
//   //enbMobility.SetPositionAllocator (enbPositionAlloc);
//   ueMobility.Install (ueNodes);
  
//   for (uint16_t i = 0; i < users; i++)
//   {
//      ueNodes.Get(i)->GetObject<MobilityModel> ()->SetPosition (Vector (-150+(yForUe*i), 100+(0.5*i), 0));
//      //ueNodes.Get(i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));
//   }


  // Install LTE Devices in eNB and UEs
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (enbTxPowerDbm));
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIfaces;
  ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  
  for (uint16_t i = 0; i < users; i++)
    {
      lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(0));
    }

  //-------------------- for example two nodes
  /*
  //
  // Explicitly create the nodes required by the topology (shown above).
  //
  NS_LOG_INFO ("Create nodes.");
  NodeContainer nodes;
  nodes.Create (2);

  NS_LOG_INFO ("Create channels.");

  //
  // Explicitly create the point-to-point link required by the topology (shown above).
  //
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (linkRate));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (delay));
  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  //
  // Install the internet stack on the nodes
  //
  InternetStackHelper internet;
  internet.Install (nodes);
  

  //
  // We've got the "hardware" in place.  Now we need to add IP addresses.
  //

  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);
*/

  NS_LOG_INFO ("Create Applications.");

  std::vector<std::string> algorithms;
  std::stringstream ss (algorithm);
  std::string proto;
  uint32_t protoNum = 0; // The number of algorithms
  while (std::getline (ss, proto, ',') && protoNum++ < users)
    {
      algorithms.push_back (proto);
    }

  uint16_t port = 80; // well-known echo port number
  std::vector<DashClientHelper> clients;
  std::vector<ApplicationContainer> clientApps;

  for (uint32_t user = 0; user < users; user++)
    {
      //--------------------------for LTE module

      // Ptr<Node> ue = ueNodes.Get (user);
      // Set the default gateway for the UE
      // Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNodes.Get (user)->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

      DashClientHelper client ("ns3::TcpSocketFactory", InetSocketAddress (remoteHostAddr, port),
                               algorithms[user % protoNum]);
      //client.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
      client.SetAttribute ("VideoId", UintegerValue (user + 1)); // VideoId should be positive
      client.SetAttribute ("TargetDt", TimeValue (Seconds (target_dt)));
      client.SetAttribute ("window", TimeValue (Time (window)));
      client.SetAttribute ("bufferSpace", UintegerValue (bufferSpace));

      ApplicationContainer clientApp = client.Install (ueNodes.Get (user));
      clientApp.Start (Seconds (0.25));
      clientApp.Stop (Seconds (stopTime));

      clients.push_back (client);
      clientApps.push_back (clientApp);

      //--------------------------for example two nodes

      // DashClientHelper client ("ns3::TcpSocketFactory", InetSocketAddress (i.GetAddress (1), port),
      //                          algorithms[user % protoNum]);
      // //client.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
      // client.SetAttribute ("VideoId", UintegerValue (user + 1)); // VideoId should be positive
      // client.SetAttribute ("TargetDt", TimeValue (Seconds (target_dt)));
      // client.SetAttribute ("window", TimeValue (Time (window)));
      // client.SetAttribute ("bufferSpace", UintegerValue (bufferSpace));

      // ApplicationContainer clientApp = client.Install (nodes.Get (0));
      // clientApp.Start (Seconds (0.25));
      // clientApp.Stop (Seconds (stopTime));

      // clients.push_back (client);
      // clientApps.push_back (clientApp);

      //------------1207 version---------------

      // DashClientHelper client ("ns3::TcpSocketFactory", InetSocketAddress (remoteHostAddr, port),
      //                          algorithms[user % protoNum]);
      // //client.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
      // client.SetAttribute ("VideoId", UintegerValue (user + 1)); // VideoId should be positive
      // client.SetAttribute ("TargetDt", TimeValue (Seconds (target_dt)));
      // client.SetAttribute ("window", TimeValue (Time (window)));
      // client.SetAttribute ("bufferSpace", UintegerValue (bufferSpace));

      // //ApplicationContainer clientApp = client.Install (nodes.Get (0));
      // ApplicationContainer clientApp = client.Install (ueNodes.Get (user));
      // clientApp.Start (Seconds (0.25));
      // clientApp.Stop (Seconds (stopTime));

      // clients.push_back (client);
      // clientApps.push_back (clientApp);
    }

  //--------------------------for LTE module

  DashServerHelper server ("ns3::TcpSocketFactory",
                           InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer serverApps = server.Install (remoteHost);
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (stopTime + 5.0));

  //--------------------------for example two nodes

  // DashServerHelper server ("ns3::TcpSocketFactory",
  //                          InetSocketAddress (Ipv4Address::GetAny (), port));
  // ApplicationContainer serverApps = server.Install (nodes.Get (1));
  // serverApps.Start (Seconds (0.0));
  // serverApps.Stop (Seconds (stopTime + 5.0));

  //
  // Set up tracing if enabled
  //
  if (tracing)
    {
      AsciiTraceHelper ascii;
      pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("dash-send.tr"));
      pointToPoint.EnablePcapAll ("dash-send", false);
    }

  //
  // Now, do the actual simulation.
  //
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop(Seconds(30.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  uint32_t k;
  for (k = 0; k < users; k++)
    {
      Ptr<DashClient> app = DynamicCast<DashClient> (clientApps[k].Get (0));
      std::cout << algorithms[k % protoNum] << "-Node: " << k;
      app->GetStats ();
    }

  return 0;
}
