/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/arp-cache.h"
#include "ns3/progress-bar.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DcfStudy");

uint32_t RcvPktCount = 0;

void
ReceiveTrace (Ptr<const Packet> pkt, const Address & addr) {
    RcvPktCount++;
}

int main (int argc, char *argv[]) {
    Time simTime = Seconds(20);
    std::string phyMode("OfdmRate6Mbps");
    uint32_t packetSize = 2000; // bytes
    uint32_t numPackets = -1;
    uint32_t numOfStations = 1;
    Time clientStart = Seconds(2);
    Time serverStart = Seconds(1);
    Time packetInterval = MicroSeconds(100);
    bool verbose = false;
    std::string outFileName = "result.txt";
    bool isEDCA = false;

    CommandLine cmd;

    cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);
    cmd.AddValue("packetSize", "size of application packet sent", packetSize);
    cmd.AddValue("numPackets", "number of packets generated", numPackets);
    cmd.AddValue("interval", "interval (seconds) between packets", packetInterval);
    cmd.AddValue("verbose", "turn on all WifiNetDevice log components", verbose);
    cmd.AddValue("simTime", "Simulation time", simTime);
    cmd.AddValue("numOfStations", "Number of stations", numOfStations);
    cmd.AddValue("outFileName", "Out file name", outFileName);
    cmd.AddValue("isEDCA", "DCF/EDCA switch", isEDCA);

    cmd.Parse(argc, argv);

    // disable fragmentation for frames below 2200 bytes
    Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2200"));
    // turn off RTS/CTS for frames below 2200 bytes
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
    // turn off fragmentation at IP layer
    Config::SetDefault("ns3::WifiNetDevice::Mtu", StringValue("2200"));

    NodeContainer serverStation;
    serverStation.Create(1);
    NodeContainer clientStations;
    clientStations.Create(numOfStations);

    NodeContainer allStations;
    allStations.Add(serverStation);
    allStations.Add(clientStations);

    // The below set of helpers will help us to put together the wifi NICs we want
    WifiHelper wifi;
    if (verbose) {
        wifi.EnableLogComponents();  // Turn on all Wifi logging
    }
    wifi.SetStandard(WIFI_PHY_STANDARD_80211a);

    //wifi.EnableLogComponents();

    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiMacHelper wifiMac;
    // Set adhoc mode and select DCF/EDCA
    wifiMac.SetType("ns3::AdhocWifiMac", "QosSupported", BooleanValue(isEDCA));
    // disable rate control
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue(phyMode));
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, allStations);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(allStations);

    InternetStackHelper internet;
    internet.Install(allStations);

    Ipv4AddressHelper ipv4;
    NS_LOG_INFO("Assign IP Addresses.");
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    ArpCache::PopulateArpCache();
    // Install applications
    // Install server on station 0
    PacketSinkHelper server("ns3::UdpSocketFactory", InetSocketAddress(interfaces.GetAddress(0), 9));

    ApplicationContainer serverApps = server.Install(serverStation);
    serverApps.Start(serverStart);
    serverApps.Stop(simTime);
    serverApps.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&ReceiveTrace));

    // Install clients
    UdpEchoClientHelper echoClient(interfaces.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue (numPackets));
    echoClient.SetAttribute("Interval", TimeValue (packetInterval));
    echoClient.SetAttribute("PacketSize", UintegerValue (packetSize));

    ApplicationContainer clientApps = echoClient.Install(clientStations);
    clientApps.Start(clientStart);
    clientApps.Stop(simTime);

    // Tracing
    if (verbose) {
        wifiPhy.EnablePcap ("wifi", devices);
    }

    ProgressBar pg(simTime);

    Simulator::Stop(simTime);
    Simulator::Run();
    Simulator::Destroy();

    // Print simulation results in the file
    //std::ofstream outStream;
   // outStream.open(outFileName.c_str(), std::ios::out);
    //if (!outStream.is_open()) {
    //    NS_FATAL_ERROR("Cannot open file " << outFileName);
   // }

    double throuhgput = 8 * RcvPktCount * packetSize / ((simTime - clientStart).GetSeconds() * 1e6);
    //outStream << numOfStations << "\t" << throuhgput << std::endl;

    std::cout << numOfStations << "\t" << throuhgput << std::endl;

    //outStream.close ();

    return 0;
}

