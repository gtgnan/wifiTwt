/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 SEBASTIEN DERONNE
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
 * Author: Sebastien Deronne <sebastien.deronne@gmail.com>
 * Modified by Shyam K Venkateswaran <vshyamkrishnan@gmail.com>
 */


#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/energy-source.h"
#include <unordered_map>
#include <iomanip>

#define FOLDER_PATH "scratch/"

// This simulation is modified from examples/wireless/wifi-he-network.cc

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wifi6Net");

uint32_t loopIndex = 10001;
uint32_t randSeed = 7000;      // Seed for Random Number Generator

// Power Scheme ----------------------------------
// Select before run
// std::string useCase = "noPs";   // No power saving

// std::string useCase = "twt";    // Use TWT
std::string useCase = "noTwt";    // Use TWT


std::string scenario = "CAM";    // used for logging and processing. Get from command line argument

// Power Scheme ----------------------------------


// Simulation configuration ----------------------

std::string currentLoopIndex_string;
uint32_t flowMonStartTime_s = 10;   // Start time for flow monitor in seconds
Time keepTrackOfEnergyFrom_S = Seconds (10); // Start time for current logging in seconds
// std::string currentLoopIndex_string = "010001";
// Simulation configuration ----------------------



// Network configuration -------------------------
std::size_t nStations {1};              // number of TWT/PS stations
double simulationTime {10 + 15};        //seconds  - 10 seconds for setup
double roomLength = 10.0;               // 2D room - side length in meters
uint32_t p2pLinkDelay {0};              //p2p link delay in ms -- RTT will be double of this
Time beaconInterval = MicroSeconds(102400);  // 102.4 ms as beacon interval
std::string dlAckSeqType;
// std::string dlAckSeqType = "NO-OFDMA";   // DL Ack Sequence Type
uint32_t bsrLife_ms = 10;                // in ms - BSR life time
// uint32_t packetsPerSecond = 50;        // UL Packets per second per station
uint32_t ampduLimitBytes = 20000;       //50,000 at 143 Mbps and 5/6 coding takes 3.4 ms
// Network configuration -------------------------


// Transport Protocol -----------------------------
// std::string protocol = "udp";    // Use UDP - DO NOT USE for video
std::string protocol = "tcp";    
uint32_t payloadSize = 1500;                       /* Transport layer payload size in bytes. */
// Transport Protocol -----------------------------




// Video traffic parameters
uint8_t videoQuality = 1; // Video quality, bv1 to bv6
double videoFrameInterval = 0.0333; // for 30 fps
double videoWeibullScale = 6950; // Will change for each video quality 
//BV1 - 6950, BV2 - 13900, BV3 - 20850, BV4 - 27800, BV5 - 34750, BV6 - 54210
double videoWeibullShape = 0.8099;
bool videoHasJitter = false;
double videoGammaShape = 0.2463;
double videoGammaScale = 60.227;

// Video Traffic direction
bool enableVideoUplink = true;
bool enableVideoDownlink = false;

// Traffic direction
bool enableUplink = true;

// Logging ---------------------------------------
bool enablePcap {true};            // Enable pcap traces if true
bool enableStateLogs = false;      // Enable state logs if true
bool enableFlowMon = true;          // Enable flow monitor if true
bool recordApPhyState = false;     // If true, AP PHY states are also logged to the stateLog file (as StaId=0)
// Logging ---------------------------------------


// TWT params

bool twtUplinkOutsideSp = false;
bool twtAnnounced = false;
bool twtWakeForBeacons = true;
// uint64_t maxMuSta = 1;

// TWT Timing parameters
// Time firstTwtSpOffsetFromBeacon = MilliSeconds (3);    // Offset from beacon for first TWT SP
Time firstTwtSpOffsetFromBeacon = MilliSeconds (2);    // Offset from beacon for first TWT SP
double twtWakeIntervalMultiplier = 0.25;    // K, where twtWakeInterval = BI * K; K is double
double twtNominalWakeDurationDivider = 16;    // K, where twtNomimalWakeDuration = BI / K; K is double
double nextStaTwtSpOffsetDivider = 16;   // K, where nextStaTwtSpOffset = BI / K; K is double
uint8_t staCountModulusForTwt = 1;      // Modulus for STA count for TWT SP start - TWT SP start Offset is added only after each 'staCountModulusForTwt' STAs
uint8_t maxOffsetModulusMultiplier = 2;      // Offsets added = providedOffset % (maxOffsetModulusMultiplier * BI)


Time firstTwtSpStart = (83 * beaconInterval);
bool twtTriggerBased = false; // Set it to false for contention-based TWT
uint64_t maxMuSta = 1;      // Maximum number of STAs the TWT scheduler will assign for a DATA MU exchange. For BSRP, max possible is used

//minTwtWakeDuration is the TWT SP duration, twtPeriod determines the periodicity, twtSpStartIncrement determines the offset for TWT allocation for subsequent STAs

// Config 1 - equal slot lengths for all levels
// For 16 STAs
// u_int8_t twtStartModulusMultiplier = 8;
// Time twtSpStartIncrement = beaconInterval/16.0;    // TWT SP will start 'twtSpStartIncrement' after the previous allocation of TWT SP
// Time twtPeriod = twtSpStartIncrement * twtStartModulusMultiplier;
// Time minTwtWakeDuration = beaconInterval/16.0;
// Time twtStartModulus = twtSpStartIncrement * twtStartModulusMultiplier;



// Config 2 - unequal slots per level - Level 1 has 2 slots of 50 ms each with half the stations
// Period is always bcn interval
// For 16 STAs
// u_int8_t twtStartModulusMultiplier = 2;
// Time twtSpStartIncrement = beaconInterval/twtStartModulusMultiplier;    // TWT SP will start 'twtSpStartIncrement' after the previous allocation of TWT SP
// Time twtPeriod = beaconInterval;
// Time minTwtWakeDuration = beaconInterval/twtStartModulusMultiplier;
// Time twtStartModulus = beaconInterval;
// bool twtTriggerBased = false; // Set it to false for contention-based TWT
// uint64_t maxMuSta = 1;      // Maximum number of STAs the TWT scheduler will assign for an MU exchange



// App timing to start only

Time AppStartMin = (45 * beaconInterval) + MilliSeconds (3);    
Time AppStartMax = (75 * beaconInterval);                       



// TI device model
std::unordered_map<std::string, double>TI_currentModel_mA = {
  {"IDLE", 50}, {"CCA_BUSY", 50}, {"RX", 66}, {"TX", 232}, {"SLEEP", 0.12}
};
double *timeElapsedForSta_ns_TI, *awakeTimeElapsedForSta_ns_TI, *current_mA_TimesTime_ns_ForSta_TI;    // This is only after keepTrackOfEnergyFrom_S


// Server Apps
ApplicationContainer serverApp;

uint64_t *totalRxBytes;
uint64_t *rxBytesAt10Seconds;
double *throughput;

// tcp parameters
uint32_t delACKTimer_ms = 0;                     /* TCP delayed ACK timer in ms   */ 

// -************************************************************************

// To schedule starting of STA wifi PHYs - to avoid assoc req sent at same time
// Refer https://groups.google.com/g/ns-3-users/c/bWaK9QvB3OQ/m/uHPB3t9DBAAJ
void RandomWifiStart (Ptr<WifiPhy> phy)
{
  phy->ResumeFromOff();
}

//-**************************************************************************
// Parse context strings of the form "/NodeList/x/DeviceList/x/..." to extract the NodeId integer
uint32_t
ContextToNodeId (std::string context)
{
  std::string sub = context.substr (10);
  uint32_t pos = sub.find ("/Device");
  return atoi (sub.substr (0, pos).c_str ());
}

//-**************************************************************************
//-**************************************************************************
// PHY state tracing 
//template <int node>
void PhyStateTrace (std::string context, Time start, Time duration, WifiPhyState state)
{
  std::stringstream ss;
  // ss <<FOLDER_PATH<< "log"<<std::setfill('0') <<std::setw(3)<<node <<currentLoopIndex_string<<".statelog";
  // ss <<FOLDER_PATH<< "log"<<std::setfill('0') <<std::setw(3)<<ContextToNodeId (context)<<"sta" <<currentLoopIndex_string<<".statelog";
  ss <<FOLDER_PATH<< "log"<<currentLoopIndex_string<<".statelog";

  static std::fstream f (ss.str ().c_str (), std::ios::out);

  // f << Simulator::Now ().GetSeconds () << "    state=" << state << " start=" << start << " duration=" << duration << std::endl;
  // Do not use spaces. Use '='  and ';'
  if (Simulator::Now ().GetSeconds()>9)
  {
    f << "state=" <<state<< ";startTime_ns="<<start.GetNanoSeconds()<<";duration_ns=" << duration.GetNanoSeconds()<<";nextStateShouldStartAt_ns="<<(start + duration).GetNanoSeconds()<<";staId="<<ContextToNodeId (context)<< std::endl;
  }
  

}

//-*************************************************************************
//-**************************************************************************
// PHY state tracing 
//template <int node>
void PhyStateTrace_inPlace (std::string context, Time start, Time duration, WifiPhyState state)
{
  // std::stringstream ss;
  // ss <<FOLDER_PATH<< "log"<<std::setfill('0') <<std::setw(3)<<node <<currentLoopIndex_string<<".statelog";
  // ss <<FOLDER_PATH<< "log"<<std::setfill('0') <<std::setw(3)<<ContextToNodeId (context)<<"sta" <<currentLoopIndex_string<<".statelog";
  // ss <<FOLDER_PATH<< "log"<<currentLoopIndex_string<<".statelog";

  // static std::fstream f (ss.str ().c_str (), std::ios::out);

  // f << Simulator::Now ().GetSeconds () << "    state=" << state << " start=" << start << " duration=" << duration << std::endl;
  // Do not use spaces. Use '='  and ';'
  if (Simulator::Now ().GetMicroSeconds()>keepTrackOfEnergyFrom_S.GetMicroSeconds())
  {
    std::ostringstream oss;
    oss << state;
    uint32_t nodeId = ContextToNodeId (context);
    // std::cout << "state=" <<state<< ";startTime_ns="<<start.GetNanoSeconds()<<";duration_ns=" << duration.GetNanoSeconds()<<";nextStateShouldStartAt_ns="<<(start + duration).GetNanoSeconds()<<";staId="<<ContextToNodeId (context)<< std::endl;
    timeElapsedForSta_ns_TI[nodeId - 1] += duration.GetNanoSeconds();
    if (oss.str() != "SLEEP")
    {
      // Add to awake time for this STA
      awakeTimeElapsedForSta_ns_TI[nodeId - 1] += duration.GetNanoSeconds();
    }
    // std::cout<<"\nCurrent value (mA)= "<<TI_currentModel_mA[oss.str()]<<"\n";
    current_mA_TimesTime_ns_ForSta_TI[nodeId - 1] += TI_currentModel_mA[oss.str()] * duration.GetNanoSeconds();
  }
  

}

//-*************************************************************************
//-**************************************************************************
// PHY Tx state tracing - check log file
void PhyTxStateTraceAll (std::string context, Time start, Time duration, WifiPhyState state)
{
  std::stringstream ss;
  ss <<FOLDER_PATH<<"log"<<currentLoopIndex_string<<".txlog";

  static std::fstream f (ss.str ().c_str (), std::ios::out);

  // f << Simulator::Now ().GetSeconds () << "    state=" << state << " start=" << start << " duration=" << duration << std::endl;
  // Do not use spaces. Use '='  and ';'
  if (state == WifiPhyState::TX)
  {
    f<<"startTime_ns="<<start.GetNanoSeconds()<<";duration_ns=" << duration.GetNanoSeconds()<< std::endl;
  }
  
}


void
SetLinkStatusForApAndSta (Ptr<Node> wifiApNode, Ptr<Node> wifiStaNode, bool enable)
{    
    // Set link status for AP and STA to enable/disable
    Time now = Simulator::Now();
    std::cout<<wifiApNode<<"\n";
    Ptr<WifiNetDevice> apDevice = DynamicCast<WifiNetDevice>(wifiApNode->GetDevice(1));
    std::cout<<"Got Ap device" << apDevice<<"\n";
    Ptr<WifiMac> apMac = apDevice->GetMac();
    std::cout<<"Got Ap Mac";
    
    Ptr<WifiNetDevice> staDevice = DynamicCast<WifiNetDevice>(wifiStaNode->GetDevice(0));
    Ptr<WifiMac> staMac = staDevice->GetMac();
    std::cout<<"Got Sta Mac device";
    // Mac48Address staMacAddress = staMac->GetAddress();

    // Assert that just one link is available at AP and STA
    NS_ABORT_MSG_IF(apMac->GetNLinks() != 1, "More than one link at AP");
    NS_ABORT_MSG_IF(staMac->GetNLinks() != 1, "More than one link at STA");

    // Get link ID for STA
    std::set<uint8_t> linkId = staMac->GetLinkIds();
    std::cout<<"Got Sta linkIds";
    // Get the first element in set
    uint8_t linkIdForSta = *linkId.begin();

    std::set<uint8_t> blockedLinks;
    std::set<uint8_t> unblockedLinks;

    if (enable)
    {
        unblockedLinks.insert(linkIdForSta);
    }
    else
    {
        blockedLinks.insert(linkIdForSta);
    }
    // Print blocked and unblocked links
    std::cout<< "At t="<<now.As(Time::S)<<", Links to be blocked: " ;
    for (auto it = blockedLinks.begin(); it != blockedLinks.end(); ++it)
    {
        std::cout<< "Link " << +(*it);
    }
    std::cout<< "At t="<<now.As(Time::S)<<", Links to be unblocked: " ;
    for (auto it = unblockedLinks.begin(); it != unblockedLinks.end(); ++it)
    {
        std::cout<< "Link " << +(*it);
    }

    staMac->BlockUnicastTxOnLinks (WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK, apMac->GetAddress(), blockedLinks);
    staMac->UnblockUnicastTxOnLinks (WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK, apMac->GetAddress(), unblockedLinks);
    // Blocking at AP side
    apMac->BlockUnicastTxOnLinks (WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK, staMac->GetAddress(), blockedLinks);
    apMac->UnblockUnicastTxOnLinks (WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK, staMac->GetAddress(), unblockedLinks);

    // for (NodeContainer::Iterator i = wifiStaNodes.Begin(); i != wifiStaNodes.End(); ++i)
    // {
    //     Ptr<Node> StaNode = *i;
    //     Ptr<WifiNetDevice> staDevice = DynamicCast<WifiNetDevice>(StaNode->GetDevice(0));
    //     Ptr<WifiMac> staMac = staDevice->GetMac();
    //     Mac48Address staMacAddress = staMac->GetAddress();

    //     // Find which linkId this STA is assigned to
    //     // uint8_t linkId = 99;
    //     uint8_t linkId = GetCurrentAssignedLinkForSta (staMacAddress);
    //     // for (auto it = currentLinkToStaMap.begin(); it != currentLinkToStaMap.end(); ++it)
    //     // {
    //     //     if (it->second.find(staMacAddress) != it->second.end())
    //     //     {
    //     //         linkId = it->first;
    //     //         break;
    //     //     }
    //     // }

    //     NS_ABORT_MSG_IF(linkId == 99, "linkId not found in currentLinkToStaMap for STA MAC=" << staMacAddress);   
        
    //     // Assert that this linkId is within GetNLinks()
    //     NS_ASSERT_MSG(linkId < staMac->GetNLinks(), "linkId must be within GetNLinks()");
        
    //     // NS_LOG_INFO ("At t="<<now.As(Time::S)<<", Blocking all but link " << +linkId << " for " << staMacAddress) ;

    //     // Create an empty set of blocked links
    //     std::set<uint8_t> blockedLinks;
    //     std::set<uint8_t> unblockedLinks;
    //     unblockedLinks.insert(linkId);
    //     for (uint8_t i = 0; i < staMac->GetNLinks(); i++)
    //     {
    //         if (i != linkId)
    //         {
    //             blockedLinks.insert(i);
    //         }
    //     }
    //     // NS_LOG_INFO ("t="<<Simulator::Now().As(Time::S)<<", Links to be blocked: " );
    //     // for (auto it = blockedLinks.begin(); it != blockedLinks.end(); ++it)
    //     // {
    //     //     NS_LOG_INFO ("Link " << +(*it));
    //     // }
    //     staMac->BlockUnicastTxOnLinks (WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK, apMac->GetAddress(), blockedLinks);
    //     staMac->UnblockUnicastTxOnLinks (WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK, apMac->GetAddress(), unblockedLinks);
    //     // Blocking at AP side
    //     apMac->BlockUnicastTxOnLinks (WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK, staMac->GetAddress(), blockedLinks);
    //     apMac->UnblockUnicastTxOnLinks (WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK, staMac->GetAddress(), unblockedLinks);
    // }
}

//-*************************************************************************
//-*************************************************************************

// void 
// initiateTwtAtSta (Ptr<StaWifiMac> staMac, Mac48Address apMacAddress)
// // ,  Time twtPeriod, Time minTwtWakeDuration, bool twtUplinkOutsideSp, bool twtWakeForBeacons, bool twtAnnounced, bool twtTriggerBased)
// // initiateTwtAtSta (Ptr<StaWifiMac> staMac,  Time twtPeriod, Time minTwtWakeDuration, bool twtUplinkOutsideSp, bool twtWakeForBeacons, bool twtAnnounced)
// {

//   // staMac->SetUpTwt (twtPeriod, minTwtWakeDuration, twtUplinkOutsideSp, twtWakeForBeacons, twtAnnounced, twtTriggerBased);
//   // staMac->CreateTwtAgreement(0, apMacAddress, true, true, true, twtTriggerBased, true, 0, twtPeriod, minTwtWakeDuration, NanoSeconds(1));
//   // staMac->SetUpTwt (twtPeriod, minTwtWakeDuration, twtUplinkOutsideSp, twtWakeForBeacons, twtAnnounced, twtTriggerBased);
//   return;
// }

// void 

// initiateTwtAtAp (Ptr<ApWifiMac> apMac, Mac48Address staMacAddress, Time twtWakeInterval, Time nominalWakeDuration, Time nextTwt)

// {
//   /**
//    * @brief Set TWT schedule at the AP - TWT SP will be scheduled at nextTwt after next beacon generation
//    * 
//    */
  
//   apMac->SetTwtSchedule (0, staMacAddress, false, true, true, twtTriggerBased, true, 0, twtWakeInterval, nominalWakeDuration, nextTwt);
//   return;
// }



//-*************************************************************************

//-*******************************************************************
// void
// changeStaPSM (Ptr<StaWifiMac> staMac, bool PSMenable)
// {
//   // This function puts the specific PHY to sleep
// //   staMac->SetPowerSaveMode(PSMenable);
// }
//-*******************************************************************



//-*******************************************************************
void
setRxBytesAt10Seconds ()
{
  // This function records the Rx bytes at each server App at 10 seconds
  // It has to be scheduled at 10 seconds
  if (protocol == "tcp")
  {
    for (uint32_t i = 0; i < serverApp.GetN (); i++)
      {
        rxBytesAt10Seconds[i] = DynamicCast<PacketSink> (serverApp.Get (i))->GetTotalRx ();
        // std::cout<<"At TCP Server "<<i<<" Rx Bytes at t=10s is recorded as: "<<rxBytesAt10Seconds[i]<<"\n";
      }
  }
  else
  {
    NS_ABORT_MSG("UDP not implemented");
  }
}
//-*******************************************************************



int main (int argc, char *argv[])
{



  CommandLine cmd (__FILE__);
  cmd.AddValue ("loopIndex", "The index of current interation. Integer between 0 and 999999", loopIndex);
  cmd.AddValue ("randSeed", "Random seed to initialize position and app start times - unit32", randSeed);
  cmd.AddValue ("useCase", "twt or noTwt",useCase);
  cmd.AddValue ("scenario", "Used for Logging and grouping for batch processing. example: CAM, dedicatedTwt1, sharedMU4, etc",scenario);
  cmd.AddValue ("twtTriggerBased", "TB if true; non-TB if false",twtTriggerBased);
  cmd.AddValue ("protocol", "Transport protocol: tcp only",protocol);
  cmd.AddValue ("nStations", "Number of non-AP HE stations", nStations);
  // cmd.AddValue ("packetsPerSecond", "packetsPerSecond: Int > 0",packetsPerSecond);
  cmd.AddValue ("simulationTime", "Simulation time in seconds- first 10 seconds is for setup", simulationTime);
  // cmd.AddValue ("frequency", "Whether working in the 2.4, 5 or 6 GHz band (other values gets rejected)", frequency);
  // cmd.AddValue ("gi", "Guard Interval in ns - 800, 1600 or 3200 ns", gi);
  // cmd.AddValue ("bandwidth", "Bandwtdth of operation in MHz - 20 or 40 MHz for the 2.4 GHz band. 20, 40, 80, 160 MHz for 5 or 6 GHz band (other values gets rejected)", frequency);
  // cmd.AddValue ("distance", "Distance in meters between the station and the access point", distance);
  cmd.AddValue ("p2pLinkDelay", "Delay of P2P link between Ap and App server in ms", p2pLinkDelay);
  cmd.AddValue ("enableUplink", "Uplink enabled if true", enableUplink);
  
  
  // cmd.AddValue ("useRts", "Enable/disable RTS/CTS", useRts);
  // cmd.AddValue ("useExtendedBlockAck", "Enable/disable use of extended BACK", useExtendedBlockAck);
  
  // cmd.AddValue ("dlAckType", "Ack sequence type for DL OFDMA (NO-OFDMA, ACK-SU-FORMAT, MU-BAR, AGGR-MU-BAR)",
                // dlAckSeqType);
  // cmd.AddValue ("mcs", "A specific MCS (0-11)", mcs);
  
  cmd.AddValue ("maxMuSta", "Max number of STAs the AP can trigger in one MU_UL with Basic TF", maxMuSta);
  cmd.AddValue ("enablePcap", "PCAP recorded if true", enablePcap);
  cmd.AddValue ("twtWakeIntervalMultiplier", "double K, where wakeInterval = BI * K", twtWakeIntervalMultiplier);
  cmd.AddValue ("twtNominalWakeDurationDivider", "Double K; Nominal wake duration = BeaconInterval/K", twtNominalWakeDurationDivider);
  cmd.AddValue ("nextStaTwtSpOffsetDivider", "Double K; Offset for next STA TWT start = BeaconInterval/K", nextStaTwtSpOffsetDivider);
  cmd.AddValue ("staCountModulusForTwt", "Int K; TWT SP start Offset is added only after each staCountModulusForTwt STAs", staCountModulusForTwt);
  // Video app params
  cmd.AddValue ("enableVideoUplink", "Uplink enabled if true", enableVideoUplink);
  cmd.AddValue ("enableVideoDownlink", "Downlink enabled if true", enableVideoDownlink);
  cmd.AddValue ("videoQuality", "integer: 1 through 6 for bv1 through bv6", videoQuality);
  
  cmd.Parse (argc,argv);



  std::cout<<"\n\nSimulation with Index "<<loopIndex<<" Started**********************************************************\n\n\n";

  // energyAtTimeStart = new double[nStations];
  // energyAtTimeEnd = new double[nStations];
  //*******************************************************
  // Logging if necessary
  
// LogComponentEnable ("WifiHelper", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("StaWifiMac", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("ApWifiMac", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("RegularWifiMac", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("WifiMacQueue", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("WifiMacQueueItem", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("WifiRemoteStationManager", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("WifiPhy", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("FrameExchangeManager", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("QosFrameExchangeManager", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("VhtFrameExchangeManager", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("HtFrameExchangeManager", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("PhyEntity", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("WifiPhyStateHelper", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("TwtRrMultiUserScheduler", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("MultiUserScheduler", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("WifiDefaultAckManager", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("WifiAckManager", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("HeFrameExchangeManager", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("ChannelAccessManager", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("QosTxop", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("Txop", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("BlockAckManager", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("BlockAckAgreement", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("OriginatorBlockAckAgreement", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("BlockAckWindow", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("MacTxMiddle", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("WifiPhyStateHelper", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("BasicEnergySource", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("PacketSink", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("VoiPApplication", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("SeqTsHeader", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("UdpServer", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("UdpClient", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("UdpSocket", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("TcpHeader", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("TcpSocketBase", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("TcpSocket", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
  //*******************************************************
  
  rxBytesAt10Seconds = new uint64_t[nStations];
  totalRxBytes = new uint64_t[nStations];
  throughput = new double[nStations];

  // Energy model init
  timeElapsedForSta_ns_TI = new double[nStations];
  awakeTimeElapsedForSta_ns_TI = new double[nStations];
  current_mA_TimesTime_ns_ForSta_TI = new double[nStations];
  for (std::size_t i = 0; i <nStations; i++)
  {
    timeElapsedForSta_ns_TI[i] = 0;
    awakeTimeElapsedForSta_ns_TI[i] = 0;
    current_mA_TimesTime_ns_ForSta_TI[i] = 0;
  }

  bool enableTwt = false;

  if (useCase == "twt")
  {
    enableTwt = true;

  }
  else if (useCase == "noTwt")
  {
    enableTwt = false;

  }
  else
  {
    std::cout<<"Use Case invalid. Exiting";
    return 0;
  }
  


  // if non trigger based but maxMuSTA is not 1, exit program
  if (twtTriggerBased == false && maxMuSta != 1)
  {
    std::cout<<"Error. Non trigger based but maxMuSTA is not 1. Exiting.";
    return 0;
  }
  // create switch case for video quality and set weibull parameters accordingly

  switch (videoQuality)
  {
  //BV1 - 6950, BV2 - 13900, BV3 - 20850, BV4 - 27800, BV5 - 34750, BV6 - 54210
  case 1:
    videoWeibullScale = 6950;
    break;
  case 2:
    videoWeibullScale = 13900;
    break;
  case 3:
    videoWeibullScale = 20850;
    break;
  case 4:
    videoWeibullScale = 27800;
    break;
  case 5:
    videoWeibullScale = 34750;
    break;
  case 6:
    videoWeibullScale = 54210;
    break;
  default:
    std::cout<<"Error. Video quality invalid. Exiting.";
    return 0;
  }


  bool useRts {false};
//   bool useExtendedBlockAck {false};


  // double distance {5.0}; //meters
  double frequency {2.4}; //whether 2.4, 5 or 6 GHz
  int channelWidth {20};  // Channel width in MHz (can be 20, 40 MHz. 80, or 160 MHz also for 5 or 6 GHz band)
  
  // std::string dlAckSeqType {"NO-OFDMA"};    // USe for PSM and no PSM
  // std::string dlAckSeqType {"ACK-SU-FORMAT"};
  // std::string dlAckSeqType {"MU-BAR"};
  
  // if (enableTwt)
  if (enableTwt && twtTriggerBased)
  {
    dlAckSeqType = "AGGR-MU-BAR";   // Use this for MU TWT - only for trigger based
  }
  else
  {
      dlAckSeqType = "NO-OFDMA";    // For PSM and no PS
  }
  // dlAckSeqType = "ACK-SU-FORMAT";
  
  
  // int mcs {-1}; // -1 indicates an unset value
  int mcs {7}; // -1 indicates an unset value - can be in [0, 11]
  int gi {800}; // Guard interval in nanoseconds (can be 800, 1600 or 3200 ns)


  std::ostringstream oss;
  oss << "HeMcs" << mcs;


  
//   uint32_t staMaxMissedBeacon = 10000;                 // Set the max missed beacons for STA before attempt for reassociation
//   Time AdvanceWakeupPS = MicroSeconds (10);

  std::string phyModel {"Spectrum"};
  bool enableAsciiTrace {false};  





  std::stringstream indexStringTemp;
  indexStringTemp << std::setfill('0') << std::setw(6) << loopIndex;
  currentLoopIndex_string = indexStringTemp.str();


  // Set Arp request retry limit to 10
  Config::SetDefault ("ns3::ArpCache::MaxRetries", UintegerValue (10));
  Config::SetDefault ("ns3::WifiMacQueue::MaxDelay", TimeValue (MilliSeconds (2000)));    // Default is 500 ms - changed because frames timed out and TCP connection kept dropping. 
  Config::SetDefault ("ns3::WifiMacQueue::MaxSize", QueueSizeValue (QueueSize ("1000p")));    // Default is 500p
  // Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue ("HtMcs7"));

  // To set QoS queue size in all QoS data frames by all STAs
  Config::SetDefault ("ns3::QosFrameExchangeManager::SetQueueSize",
                    BooleanValue (true));
  if (useRts)
    {
      Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
    }

  if (dlAckSeqType == "ACK-SU-FORMAT")
    {
      Config::SetDefault ("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                          EnumValue (WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE));
    }
  else if (dlAckSeqType == "MU-BAR")
    {
      Config::SetDefault ("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                          EnumValue (WifiAcknowledgment::DL_MU_TF_MU_BAR));
    }
  else if (dlAckSeqType == "AGGR-MU-BAR")
    {
      Config::SetDefault ("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                          EnumValue (WifiAcknowledgment::DL_MU_AGGREGATE_TF));
    }
  else if (dlAckSeqType != "NO-OFDMA")
    {
      NS_ABORT_MSG ("Invalid DL ack sequence type (must be NO-OFDMA, ACK-SU-FORMAT, MU-BAR or AGGR-MU-BAR)");
    }

  if (phyModel != "Yans" && phyModel != "Spectrum")
    {
      NS_ABORT_MSG ("Invalid PHY model (must be Yans or Spectrum)");
    }
  if (dlAckSeqType != "NO-OFDMA")
    {
      // SpectrumWifiPhy is required for OFDMA
      phyModel = "Spectrum";
    }


  // std::cout << "MCS value" << "\t\t" << "Channel width" << "\t\t" << "GI" << "\t\t\t" << "Throughput" << '\n';


  // TCP parameters
  std::string tcpVariant = std::string ("ns3::TcpNewReno");
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcpVariant)));


  /* Configure TCP Options */
  Config::SetDefault ("ns3::TcpSocket::DelAckTimeout", TimeValue (MilliSeconds (delACKTimer_ms)));
  // Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (Seconds (1)));    //"TCP retransmission timeout when opening connection (seconds) - default 3 seconds"
  Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (MilliSeconds(200)));    //"TCP retransmission timeout when opening connection (seconds) - default 3 seconds"
  Config::SetDefault ("ns3::TcpSocket::DataRetries", UintegerValue (20)); 
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
  // Creating nodes

  NodeContainer wifiApNodes;
  wifiApNodes.Create (1);
  Ptr<Node> ApNode = wifiApNodes.Get (0);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nStations);
  
  // For TCP traffic only - p2p nodes setup ⬇️
  NodeContainer p2pServerNodes;   // Only the remote p2p server node - not including AP
  p2pServerNodes.Create (1);
  Ptr<Node> p2pServerNode = p2pServerNodes.Get (0);   // The remote P2P node

  NodeContainer p2pConnectedNodes;  // remote server and AP node 
  p2pConnectedNodes.Add (ApNode);
  p2pConnectedNodes.Add (p2pServerNode);

  PointToPointHelper pointToPoint;
  std::stringstream delayString;
  delayString<<p2pLinkDelay <<"ms";
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1000Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (delayString.str()));
  
  NetDeviceContainer p2pdevices;
  p2pdevices = pointToPoint.Install (p2pConnectedNodes);
  // For TCP traffic only - p2p nodes setup ⬆️

  



  // Printing Node IDs
  // std::cout<<"Node IDs:\n";
  // std::cout<<"\tP2P node0: "<<unsigned(P2PNodes.Get(0)->GetId())<<std::endl;
  // std::cout<<"\tAP: "<<unsigned(wifiApNodes.Get(0)->GetId()) <<std::endl;
  //  std::cout<<"\tTWT STA is: "<<unsigned(wifiStaNodes.Get(0)->GetId()) <<std::endl;

    NetDeviceContainer apDevice, staDevices;
    WifiMacHelper mac;
    WifiHelper wifi;
    StringValue ctrlRate;
    auto nonHtRefRateMbps = HePhy::GetNonHtReferenceRate(mcs) / 1e6;    
    std::string channelStr ("{0, " + std::to_string (channelWidth) + ", ");
    std::ostringstream ossDataMode;
    ossDataMode << "HeMcs" << mcs;

    if (frequency == 6)
    {
        wifi.SetStandard(WIFI_STANDARD_80211ax);
        ctrlRate = StringValue(ossDataMode.str());
        channelStr += "BAND_6GHZ, 0}";
        Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss",
                            DoubleValue(48));
    }
    else if (frequency == 5)
    {
        wifi.SetStandard(WIFI_STANDARD_80211ax);
        std::ostringstream ossControlMode;
        ossControlMode << "OfdmRate" << nonHtRefRateMbps << "Mbps";
        ctrlRate = StringValue(ossControlMode.str());
        channelStr += "BAND_5GHZ, 0}";
    }
    else if (frequency == 2.4)
    {
        wifi.SetStandard(WIFI_STANDARD_80211ax);
        std::ostringstream ossControlMode;
        ossControlMode << "ErpOfdmRate" << nonHtRefRateMbps << "Mbps";
        ctrlRate = StringValue(ossControlMode.str());
        channelStr += "BAND_2_4GHZ, 0}";
        Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss",
                            DoubleValue(40));
    }
    else
    {
        std::cout << "Wrong frequency value!" << std::endl;
        return 0;
    }

  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                      UintegerValue (65535));
  
  // wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss.str ()), "ControlMode", StringValue (oss.str ()));
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("HeMcs7"), "ControlMode", StringValue ("HeMcs0"));
  // wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss.str ()), "ControlMode", StringValue (oss.str ()));
  // wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss.str ()), "ControlMode", StringValue (oss.str ()));
  // wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager", "RtsCtsThreshold", UintegerValue (65535));
  Ssid ssid = Ssid ("ns3-80211ax");

  //To enable ASCII traces
  AsciiTraceHelper ascii;
  std::stringstream ss;
  ss <<FOLDER_PATH<< "log"<<currentLoopIndex_string<<".asciitr" ;
  
  // if (phyModel == "Spectrum")
  //   {
  /*
  * SingleModelSpectrumChannel cannot be used with 802.11ax because two
  * spectrum models are required: one with 78.125 kHz bands for HE PPDUs
  * and one with 312.5 kHz bands for, e.g., non-HT PPDUs (for more details,
  * see issue #408 (CLOSED))
  */
  Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel> ();
  SpectrumWifiPhyHelper phy;
  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
  phy.SetChannel (spectrumChannel);


  mac.SetType ("ns3::StaWifiMac",
              "BE_BlockAckThreshold", UintegerValue (1),  // If AMPDU is used, Block Acks will always be used regardless of this value
              "Ssid", SsidValue (ssid));

    phy.Set ("ChannelSettings", StringValue (channelStr));
  staDevices = wifi.Install (phy, mac, wifiStaNodes);


  // if (dlAckSeqType != "NO-OFDMA")
  //   {
  //     mac.SetMultiUserScheduler ("ns3::RrMultiUserScheduler",
  //                               "EnableUlOfdma", BooleanValue (true),
  //                               "EnableBsrp", BooleanValue (false));
  //   }
  if (dlAckSeqType != "NO-OFDMA")   // My scheduler
    {
      // std::cout<<"\nTwtRrMultiUserScheduler is selected\n";
      mac.SetMultiUserScheduler ("ns3::TwtRrMultiUserScheduler",
                                "EnableUlOfdma", BooleanValue (true),
                                // "EnableBsrp", BooleanValue (true),
                                "EnableBsrp", BooleanValue (twtTriggerBased),
                                "NStations", UintegerValue (maxMuSta));
    }

  mac.SetType ("ns3::ApWifiMac",
              "EnableBeaconJitter", BooleanValue (false),
              "BE_BlockAckThreshold", UintegerValue (1),
                // "BE_MaxAmsduSize", UintegerValue (7000),
              //  "BE_MaxAmpduSize", UintegerValue (30000),
              //  "BE_MaxAmpduSize", UintegerValue (64*1500),  // 64 MPDUs of 1500 bytes each
                "BE_MaxAmpduSize", UintegerValue (ampduLimitBytes),  
                "BsrLifetime", TimeValue (MilliSeconds (bsrLife_ms)),
              "Ssid", SsidValue (ssid));
  // }




  apDevice = wifi.Install (phy, mac, wifiApNodes);



  if (enableAsciiTrace)
    {
      phy.EnableAsciiAll (ascii.CreateFileStream (ss.str()));
    }
  if (enablePcap)
    {
      std::stringstream ss1, ss2, ss4;
      phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
      ss1<<FOLDER_PATH<<"pcap_AP";
      phy.EnablePcap (ss1.str(), apDevice);
            // Record PCAP for all STAs
      // for (uint32_t i = 0; i < nStations; i++)
      // {
      //   ss2<<FOLDER_PATH<<"pcap_STA"<<i;
      //   phy.EnablePcap (ss2.str(), staDevices.Get(i));
      //   ss2.str("");
      // }
      // ss2<<FOLDER_PATH<<"pcap_STA0";
      // phy.EnablePcap (ss2.str(), staDevices.Get(0));
      // ss2<<FOLDER_PATH<<"pcap_p2p";
      // phy.EnablePcap (ss2.str(), p2pdevices.Get(0));
    }
      

  RngSeedManager::SetSeed (randSeed);
  RngSeedManager::SetRun (1);
  int64_t streamNumber = 100;
  streamNumber += wifi.AssignStreams (apDevice, streamNumber);
  streamNumber += wifi.AssignStreams (staDevices, streamNumber);

  // Set guard interval and MPDU buffer size
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/GuardInterval", TimeValue (NanoSeconds (gi)));


  // Set max missed beacons to avoid dis-association
//   for (u_int32_t ii = 0; ii < wifiStaNodes.GetN() ; ii++)
//   {
    
//     std::stringstream nodeIndexStringTemp, maxBcnStr, advWakeStr, apsdStr;
//     nodeIndexStringTemp << wifiStaNodes.Get(ii)->GetId();
//     // maxBcnStr = "/NodeList/" + std::string(ii+1) + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/MaxMissedBeacons";
//     maxBcnStr << "/NodeList/" << nodeIndexStringTemp.str() << "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/MaxMissedBeacons";
//     advWakeStr << "/NodeList/" << nodeIndexStringTemp.str() << "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/AdvanceWakeupPS";

//     Config::Set (maxBcnStr.str(), UintegerValue(staMaxMissedBeacon));
//     Config::Set (advWakeStr.str(), TimeValue(AdvanceWakeupPS));

//   }

  // Config::Set ("/NodeList/1/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/MaxMissedBeacons", UintegerValue(staMaxMissedBeacon));
  // Config::Set ("/NodeList/1/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/AdvanceWakeupPS", TimeValue(AdvanceWakeupPS));

  // ----------------------------------------------------------------------------

  // Setting up TWT
  Ptr<WifiNetDevice> apWifiDevice = apDevice.Get(0)->GetObject<WifiNetDevice> ();    //This returns the pointer to the object - works for all functions from WifiNetDevice
  Ptr<WifiMac> apMacTemp = apWifiDevice->GetMac ();
  Ptr<ApWifiMac> apMac = DynamicCast<ApWifiMac> (apMacTemp);
  // Mac48Address apMacAddress = apMac->GetAddress();
  // std::cout<<"Ap MAC:"<<apMac<<"\n";
  if (enableTwt)
  {
    for (u_int32_t ii = 0; ii < nStations ; ii++)
    {
      // Setting up TWT for Sta Mac
      Ptr<WifiNetDevice> device = staDevices.Get(ii)->GetObject<WifiNetDevice> ();    //This returns the pointer to the object - works for all functions from WifiNetDevice
      Ptr<WifiMac> staMacTemp = device->GetMac ();
      Ptr<StaWifiMac> staMac = DynamicCast<StaWifiMac> (staMacTemp);
      
      Time delta = firstTwtSpOffsetFromBeacon + int(ii/staCountModulusForTwt)*beaconInterval/nextStaTwtSpOffsetDivider;
      delta = MicroSeconds( delta.GetMicroSeconds()% (maxOffsetModulusMultiplier*beaconInterval.GetMicroSeconds()) );   
      // To ensure that offsets are not too large. That may cause frame drops before the first SP begins. Above line ensures that the first scheduled SP is no further than k*BI from the next beacon in the future
      Time scheduleTwtAgreement = (firstTwtSpStart + MilliSeconds(2) + ii*MicroSeconds(100));
      // Time scheduleTwtAgreement = (firstTwtSpStart);
      Time twtWakeInterval = twtWakeIntervalMultiplier*beaconInterval;
      Time twtNominalWakeDuration = beaconInterval/twtNominalWakeDurationDivider;

      // TWT at AP MAC
      // Mac48Address staMacAddress = staMac->GetAddress();
      std::cout<<"\nTWT agreement for STA:"<<staMac->GetAddress()<<"\nAction frame acheduled at t = "<<scheduleTwtAgreement.GetSeconds() << "s;\nTWT SP starts at "<<delta.GetMicroSeconds()/1000.0<<" ms after next beacon;\nTWT Wake Interval = "<< twtWakeInterval.GetMicroSeconds()/1000.0<<" ms;\nTWT Nominal Wake Duration = "<< twtNominalWakeDuration.GetMicroSeconds()/1000.0<<" ms;\n\n\n";

      // Simulator::Schedule (scheduleTwtAgreement, &initiateTwtAtAp, apMac,staMac->GetAddress(), twtWakeInterval, twtNominalWakeDuration, delta);

    }
  
  }

    Simulator::Schedule (Seconds(15), &SetLinkStatusForApAndSta, ApNode, wifiStaNodes.Get(0), false);
  // ---------------------------------------------
  // - enabling PHY of wifiStaNodes at random times - to avoid assoc req at same time
  for (uint32_t i=0 ; i < wifiStaNodes.GetN(); i++)
  {
    Ptr<Node> n = wifiStaNodes.Get (i);
    Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (n->GetDevice (0)); //assuming only one device
    Ptr<WifiPhy> phy = wifi_dev->GetPhy();
    phy->SetOffMode (); //turn all of them off
    
    //schedule random time when they switch their radio ON.
    Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable> ();
    // uint32_t start_time = random->GetInteger (0, 5000);
    uint32_t start_time = random->GetInteger (0, 2500);
    Simulator::Schedule (MilliSeconds (start_time), &RandomWifiStart, phy);
  }
  // ---------------------------------------------

  // mobility // ---------------------------------------------
  // Room dimension in meters - creating uniform random number generator
  double minX = -1.0 * roomLength/2;
  double maxX = 1.0 * roomLength/2;
  double minY = -1.0 * roomLength/2;
  double maxY = 1.0 * roomLength/2;

  //  STA positions
  double currentX, currentY;  
  Ptr<UniformRandomVariable> xCoordinateRand = CreateObject<UniformRandomVariable> ();
  Ptr<UniformRandomVariable> yCoordinateRand = CreateObject<UniformRandomVariable> ();

  xCoordinateRand->SetAttribute ("Min", DoubleValue (minX));
  xCoordinateRand->SetAttribute ("Max", DoubleValue (maxX));
  yCoordinateRand->SetAttribute ("Min", DoubleValue (minY));
  yCoordinateRand->SetAttribute ("Max", DoubleValue (maxY));
  
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  positionAlloc->Add (Vector (0.0, 0.0, 0.0));    // AP node
  // positionAlloc->Add (Vector (distance, 0.0, 0.0)); // STA nodes
  for (uint32_t ii = 0; ii < nStations ; ii++)
  {
    currentX = xCoordinateRand->GetValue ();
    currentY = yCoordinateRand->GetValue ();
    // std::cout<<"\tTWT STA "<<ii<<" : ["<< currentX<<", "<<currentY <<", 0.0 ];\n";
    positionAlloc->Add (Vector (currentX, currentY, 0.0));
  }
  


  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNodes);
  mobility.Install (wifiStaNodes);


  /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (wifiApNodes);
  stack.Install (wifiStaNodes);
  stack.Install (p2pServerNodes);
  

  Ipv4AddressHelper address;
  address.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer staNodeInterfaces;
  Ipv4InterfaceContainer apNodeInterface;
  Ipv4InterfaceContainer p2pNodeInterfaces;


  apNodeInterface = address.Assign (apDevice);
  staNodeInterfaces = address.Assign (staDevices);
  address.SetBase ("192.168.2.0", "255.255.255.0");
  p2pNodeInterfaces = address.Assign (p2pdevices);

  // create a map of IP addresses to MAC addresses
  std::map<Ipv4Address, Mac48Address> ipToMac;
  for (uint32_t i = 0; i < wifiStaNodes.GetN(); i++)
  {
    Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (wifiStaNodes.Get (i)->GetDevice (0)); //assuming only one device
    Ptr<WifiMac> wifi_mac = wifi_dev->GetMac ();
    Ptr<StaWifiMac> sta_mac = DynamicCast<StaWifiMac> (wifi_mac);
    ipToMac[staNodeInterfaces.GetAddress (i)] = sta_mac->GetAddress ();
  }

  // Pretty print ipToMac
  std::cout << "IP to MAC mapping:\n";
  for (auto it = ipToMac.begin (); it != ipToMac.end (); it++)
  {
    std::cout << it->first << " => " << it->second << '\n';
  }


  // Setting up TCP Video traffic
  if (enableVideoUplink)
  {
    // Uplink Video traffic setup - TCP is used
    uint16_t port = 50000;

    Ptr<UniformRandomVariable> randTime = CreateObject<UniformRandomVariable> ();
    randTime->SetAttribute ("Min", DoubleValue (AppStartMin.GetMicroSeconds()));
    randTime->SetAttribute ("Max", DoubleValue (AppStartMax.GetMicroSeconds()));
    for (std::size_t i = 0; i < nStations; i++)
    {
      // Server APPs at the Remote server
      Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port+i));
      PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
      ApplicationContainer tempServerApp =sinkHelper.Install (p2pServerNode);
      tempServerApp.Start (Seconds (0.0));
      tempServerApp.Stop (Seconds (simulationTime + 1));
      serverApp.Add (tempServerApp);

      // Client Apps
      VideoHelper video ("ns3::TcpSocketFactory", (InetSocketAddress (p2pNodeInterfaces.GetAddress (1), port+i)));
      

      video.SetAttribute ("FrameInterval", DoubleValue (videoFrameInterval));
      video.SetAttribute ("WeibullScale", DoubleValue (videoWeibullScale));
      video.SetAttribute ("WeibullShape", DoubleValue (videoWeibullShape));
      video.SetAttribute ("HasJitter", BooleanValue (videoHasJitter));
      video.SetAttribute ("GammaScale", DoubleValue (videoGammaScale));
      video.SetAttribute ("GammaShape", DoubleValue (videoGammaShape));
      ApplicationContainer clientApp = video.Install (wifiStaNodes.Get (i));


      Time clientTempStartTime = MicroSeconds(randTime->GetValue());
      std::cout<<"Uplink client App "<<i<<" starts at "<<clientTempStartTime.GetMicroSeconds()<<" us\n";
      clientApp.Start (clientTempStartTime);
      
      // std::cout<<"\nClient APP "<<i<<" starts at :"<<clientTempStartTime.GetMilliSeconds()<<" ms";
      
      clientApp.Stop (Seconds (simulationTime + 1));
    }

  }
  if (enableVideoDownlink)
  {
    // Downlink Video traffic setup - TCP is used
    uint16_t port = 60000;

    Ptr<UniformRandomVariable> randTime = CreateObject<UniformRandomVariable> ();
    randTime->SetAttribute ("Min", DoubleValue (AppStartMin.GetMicroSeconds()));
    randTime->SetAttribute ("Max", DoubleValue (AppStartMax.GetMicroSeconds()));
    for (std::size_t i = 0; i < nStations; i++)
    {
      // Server APPs at the Stations
      Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port+i));
      PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
      // ApplicationContainer tempServerApp =sinkHelper.Install (p2pServerNode);
      ApplicationContainer tempServerApp =sinkHelper.Install (wifiStaNodes.Get (i));
      tempServerApp.Start (Seconds (0.0));
      tempServerApp.Stop (Seconds (simulationTime + 1));
      serverApp.Add (tempServerApp);

      // Client Apps
      // VideoHelper video ("ns3::TcpSocketFactory", (InetSocketAddress (p2pNodeInterfaces.GetAddress (1), port+i)));
      VideoHelper video ("ns3::TcpSocketFactory", (InetSocketAddress (staNodeInterfaces.GetAddress (i), port+i)));
      

      video.SetAttribute ("FrameInterval", DoubleValue (videoFrameInterval));
      video.SetAttribute ("WeibullScale", DoubleValue (videoWeibullScale));
      video.SetAttribute ("WeibullShape", DoubleValue (videoWeibullShape));
      video.SetAttribute ("HasJitter", BooleanValue (videoHasJitter));
      video.SetAttribute ("GammaScale", DoubleValue (videoGammaScale));
      video.SetAttribute ("GammaShape", DoubleValue (videoGammaShape));
      // ApplicationContainer clientApp = video.Install (wifiStaNodes.Get (i));
      ApplicationContainer clientApp = video.Install (p2pServerNode);


      Time clientTempStartTime = MicroSeconds(randTime->GetValue());
      std::cout<<"Downlink client App "<<i<<" starts at "<<clientTempStartTime.GetMicroSeconds()<<" us\n";
      clientApp.Start (clientTempStartTime);
      
      // std::cout<<"\nClient APP "<<i<<" starts at :"<<clientTempStartTime.GetMilliSeconds()<<" ms";
      
      clientApp.Stop (Seconds (simulationTime + 1));
    }

  }

  

  // For state tracing - to log file

  for (std::size_t ii = 0; ii < wifiStaNodes.GetN() ; ii++)
  {
    std::stringstream nodeIndexStringTemp, phyStateStr;
    nodeIndexStringTemp << wifiStaNodes.Get(ii)->GetId();
    phyStateStr << "/NodeList/" << nodeIndexStringTemp.str() << "/DeviceList/*/Phy/State/State";
    Config::Connect (phyStateStr.str(), MakeCallback (&PhyStateTrace_inPlace));
    
    if (enableStateLogs)
    {
      Config::Connect (phyStateStr.str(), MakeCallback (&PhyStateTrace));
    }
    

  }  

  // State log trafce for AP - for channel idle probability

  if (enableStateLogs && recordApPhyState)
  {
    std::stringstream nodeIndexStringTemp, phyStateStr;
    nodeIndexStringTemp << wifiApNodes.Get(0)->GetId();
    phyStateStr << "/NodeList/" << nodeIndexStringTemp.str() << "/DeviceList/*/Phy/State/State";
    Config::Connect (phyStateStr.str(), MakeCallback (&PhyStateTrace));
  }

  Simulator::Schedule (Seconds (0), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);

  // Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (oss.str()));
  
  // If flowmon is needed
  // FlowMonitor setup
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor;
  if (enableFlowMon)
  {
    flowmon.SetMonitorAttribute("StartTime", TimeValue(Seconds (flowMonStartTime_s)));
    monitor = flowmon.InstallAll();
  
  }
  
  Simulator::Schedule (Seconds (10), &setRxBytesAt10Seconds);
  // Simulator::Schedule (keepTrackOfEnergyFrom_S, &UpdateAllRemainingEnergyBeginning);

  // Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Stop (Seconds (simulationTime));

  Simulator::Run ();

  std::cout<<"\n****************************";

  // std::stringstream ssNodeLog;
  // ssNodeLog <<FOLDER_PATH<< "log"<<".nodelog";
  // static std::fstream g (ssNodeLog.str ().c_str (), std::ios::app);
  double *avgCurrent_mAForSTA = new double [nStations];
  double *totEnergyConsumedForSta_J = new double [nStations];
  
  // Map of STA MAC address to current in mA
  std::map<Mac48Address, double> staMacToCurrent_mA;
  std::map<Mac48Address, double> staMacToTimeElapsed_us;
  std::map<Mac48Address, double> staMacToTimeAwake_us;
  // Map of STA MAC address to energy consumed in J
  std::map<Mac48Address, double> staMacToEnergyConsumed_J;
  
  // create a map from STA mac address to the pair
  std::map<Mac48Address, std::pair<double, double>> staMacToTotalBitsUplinkDownlink;
  std::map<Mac48Address, std::pair<double, double>> staMacToUplinkDownlinkThroughput_kbps;
  std::map<Mac48Address, std::pair<double, double>> staMacToUplinkDownlinkLatency_usPerPkt;
  // Initialize for all existing STAs with <0,0>
  for (uint32_t i = 0; i < wifiStaNodes.GetN(); i++)
  {
    Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (wifiStaNodes.Get (i)->GetDevice (0)); //assuming only one device
    Ptr<WifiMac> wifi_mac = wifi_dev->GetMac ();
    Ptr<StaWifiMac> sta_mac = DynamicCast<StaWifiMac> (wifi_mac);
    staMacToTotalBitsUplinkDownlink[sta_mac->GetAddress ()] = std::make_pair (0, 0);
    staMacToUplinkDownlinkThroughput_kbps[sta_mac->GetAddress ()] = std::make_pair (0, 0);
    staMacToUplinkDownlinkLatency_usPerPkt[sta_mac->GetAddress ()] = std::make_pair (0, 0);
  }

  // In place current calculation
  for (std::size_t i = 0; i < nStations; i++)
  {
    Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (wifiStaNodes.Get (i)->GetDevice (0)); //assuming only one device
    Ptr<WifiMac> wifi_mac = wifi_dev->GetMac ();
    Ptr<StaWifiMac> sta_mac = DynamicCast<StaWifiMac> (wifi_mac);
    Mac48Address currentMacAddress = sta_mac->GetAddress ();
    avgCurrent_mAForSTA[i] = current_mA_TimesTime_ns_ForSta_TI[i]/timeElapsedForSta_ns_TI[i];
    totEnergyConsumedForSta_J [i] = (avgCurrent_mAForSTA[i]/1000.0) * (timeElapsedForSta_ns_TI[i]/1e9) * 3; // for 3 volts
    // Add this energy to map staMacToEnergyConsumed_J
    staMacToEnergyConsumed_J[currentMacAddress] = totEnergyConsumedForSta_J [i];
    // Add current mA to map
    staMacToCurrent_mA[currentMacAddress] = avgCurrent_mAForSTA[i];
    staMacToTimeElapsed_us[currentMacAddress] = timeElapsedForSta_ns_TI[i]/1000.0;
    staMacToTimeAwake_us[currentMacAddress] = awakeTimeElapsedForSta_ns_TI[i]/1000.0;
    std::cout<<"\nFor STA "<<i<<";MacAddress="<<currentMacAddress<<"; Elapsed time (ms) = "<<timeElapsedForSta_ns_TI[i]/1e6<<"; Average current (mA) = "<<avgCurrent_mAForSTA[i]<<"; Total energy consumed (J) = "<<staMacToEnergyConsumed_J[currentMacAddress];
    // g <<"simID="<<currentLoopIndex_string<<";" << "nSTA="<<nStations<<";" <<"randSeed="<<randSeed<<";" <<"useCase="<<useCase<<";"<<"triggerBased="<<twtTriggerBased<<";" <<"maxMuSta="<<maxMuSta<<";"
    //     <<"sta#="<<i<<";"
    //     <<"MacAddress="<<currentMacAddress<<";"
    //     <<"timeElapsedForSta_ms="<<timeElapsedForSta_ns_TI[i]/1e6<<";"
    //     <<"avgCurrentForSta_mA="<<avgCurrent_mAForSTA[i]<<";"
    //     <<"totEnergyConsumedForSta_J="<<staMacToEnergyConsumed_J[currentMacAddress]<<";\n";
  }

  // Flowmon ---------------------------------
  // monitor->SerializeToXmlFile("flow1.flowmon", false, true);

  monitor->CheckForLostPackets ();

  // // Log file for flow stats - goodput and latency
  double *avgDelay_us;
  Histogram *delayHist;
  
  // for (int ii = 0; ii < nStations; ii++)
  // {
  //   avgDelay_us [ii] = 0;
  // }
  
  // std::cout<<"Reached here 1\n";
  if (enableFlowMon)
  {
    std::cout<<"\n-----------------\n";
    std::cout<<"\nFlow Level Stats:\n";
    std::cout<<"-----------------\n\n";
    


    // std::stringstream ssFlowLog;
    // ssFlowLog <<FOLDER_PATH<< "log"<<".flowlog";
    // static std::fstream f (ssFlowLog.str ().c_str (), std::ios::app);
    

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    // find size of stats
    size_t flowCount = stats.size();  // number of flows
    avgDelay_us = new double [flowCount];
    delayHist = new Histogram [flowCount];

    int counter = 0;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
      {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      
      double totalBitsRx = i->second.rxBytes * 8.0;
      // double throughputKbps =  i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1000;
      std::cout<<"simulationTime + 1 ="<<simulationTime + 1;
      double throughputKbps =  i->second.rxBytes * 8.0 / ( (simulationTime + 1) - keepTrackOfEnergyFrom_S.GetSeconds())/1000;
      double avgDelayMicroSPerPkt = i->second.delaySum.GetMicroSeconds()/i->second.rxPackets ;
      u_int32_t lostPackets = i->second.lostPackets;
      avgDelay_us[counter] = avgDelayMicroSPerPkt;
      delayHist[counter] = i->second.delayHistogram;
      // std::cout << "  Avg. Delay ( us/pkt): " << avgDelayMicroSPerPkt << " us/pkt\n";
      std::cout<< std::setw(30)<<std::left << "Flow ID" <<":\t"<<i->first<<"\n";
      std::cout<< std::setw(30)<<std::left << "Source IP and Port" <<":\t"<<t.sourceAddress<<" , "<<t.sourcePort<<"\n";
      // if sourceAddress is found in ipToMac map, then print the corresponding MAC address
      if (ipToMac.find(t.sourceAddress) != ipToMac.end())
      {
        std::cout<< std::setw(30)<<std::left << "Source STA MAC" <<":\t"<<ipToMac[t.sourceAddress]<<"\n";
        // Add totalBitsRx to Uplink of this STA in staMacToTotalBitsUplinkDownlink
        staMacToTotalBitsUplinkDownlink[ipToMac[t.sourceAddress]].first += totalBitsRx;
        staMacToUplinkDownlinkThroughput_kbps[ipToMac[t.sourceAddress]].first = throughputKbps;
        staMacToUplinkDownlinkLatency_usPerPkt[ipToMac[t.sourceAddress]].first = avgDelayMicroSPerPkt;
      }
      
      std::cout<< std::setw(30)<<std::left << "Destination IP and Port" <<":\t"<<t.destinationAddress<<" , "<<t.destinationPort<<"\n";
      if (ipToMac.find(t.destinationAddress) != ipToMac.end())
      {
        std::cout<< std::setw(30)<<std::left << "Destination STA MAC" <<":\t"<<ipToMac[t.destinationAddress]<<"\n";
        // Add totalBitsRx to Uplink of this STA in staMacToTotalBitsUplinkDownlink
        staMacToTotalBitsUplinkDownlink[ipToMac[t.destinationAddress]].second += totalBitsRx;
        staMacToUplinkDownlinkThroughput_kbps[ipToMac[t.destinationAddress]].second = throughputKbps;
        staMacToUplinkDownlinkLatency_usPerPkt[ipToMac[t.destinationAddress]].second = avgDelayMicroSPerPkt;
      }
      std::cout<< std::setw(30)<<std::left << "Throughput (kbps)" <<":\t"<<throughputKbps<<"\n";
      std::cout<< std::setw(30)<<std::left << "Total bits received" <<":\t"<<totalBitsRx<<"\n";
      std::cout<< std::setw(30)<<std::left << "Avg. Delay ( us/pkt)" <<":\t"<< avgDelayMicroSPerPkt << " us/pkt\n";
      std::cout<< std::setw(30)<<std::left << "Lost Packets" <<":\t"<< lostPackets << " pkts\n";
      
      
      // std::cout<< "Delay Histogram\n";
      // std::cout<< std::setw(15)<<std::left << "Bin Start (s)"<<" |\t";
      // std::cout<< std::setw(15)<<std::left << "Bin End (s)"<<" |\t";
      // std::cout<< std::setw(15)<<std::left << "Packet Count"<<"\n";
      // // Bin End (s) | Packet Count\n";
      // for (int histBin = 0; histBin < delayHist[counter].GetNBins(); histBin++) {
      //   std::cout<< std::setw(15)<<std::left << delayHist[counter].GetBinStart(histBin)<<" |\t";
      //   std::cout<< std::setw(15)<<std::left << delayHist[counter].GetBinEnd(histBin)<<" |\t";
      //   std::cout<< std::setw(15)<<std::left << delayHist[counter].GetBinCount(histBin)<<"\n";

      //   // "<<delayHist[counter].GetBinEnd(histBin)<<" | "<<delayHist[counter].GetBinCount(histBin)<<"\n";
      // }
      
      // std::cout<<"\n\n";
      std::cout<<"-----------------\n\n";

      // f <<"simID="<<currentLoopIndex_string<<";" << "nSTA="<<nStations<<";" <<"randSeed="<<randSeed<<";" <<"useCase="<<useCase<<";"<<"triggerBased="<<twtTriggerBased<<";" <<"maxMuSta="<<maxMuSta<<";"
      //   <<"protocol="<<protocol<<";"
      //   // <<"packetsPerSecond="<<packetsPerSecond<<";"
      //  <<"flow="<< i->first<<";sourceIp="<<t.sourceAddress<<";sourcePort="<<t.sourcePort
      //   <<";destIp="<<t.destinationAddress<<";destPort="<<t.destinationPort<<";totalBitsRx="<<totalBitsRx
      //   <<";throughputKbps="<<throughputKbps<<";avgDelayMicroSPerPkt="<<avgDelayMicroSPerPkt<<std::endl;
      // counter++;
        
      }
    // flowmon -------------------------------------------------

  }
  // Writing to a log file
  std::stringstream ssNodeLog;
  ssNodeLog <<FOLDER_PATH<< "logAllperSta"<<".log";
  static std::fstream g (ssNodeLog.str ().c_str (), std::ios::app);
  // std::cout << "Per STA node stats:\n";
  for (auto it = staMacToTotalBitsUplinkDownlink.begin (); it != staMacToTotalBitsUplinkDownlink.end (); it++)
  {
    double totalBits = it->second.first  + it->second.second ;
    // print STA MAC and then Uplink and Downlink
    // std::cout 
    g
    <<"scenario="<<scenario
    <<";simID="<<currentLoopIndex_string
    <<";nSTA="<<nStations
    <<";useCase="<<useCase
    <<";twtWakeIntervalMultiplier="<<twtWakeIntervalMultiplier
    <<";twtNominalWakeDurationDivider="<<twtNominalWakeDurationDivider
    <<";nextStaTwtSpOffsetDivider="<<nextStaTwtSpOffsetDivider
    <<";simulationTime="<<simulationTime
    <<";traffic="<<"video_TCP_UL_only"
    <<";randSeed="<<+randSeed
    <<";videoQuality="<<+videoQuality
    <<";StaMacAddress="<< it->first 
    << ";UL_bits=" << it->second.first 
    << ";DL_bits=" << it->second.second 
    << ";total_bits=" << totalBits
    <<";energy_J="<< staMacToEnergyConsumed_J[it->first]
    <<";energyPerTotBit_JPerBit="<< staMacToEnergyConsumed_J[it->first]/totalBits
    <<";current_mA="<<staMacToCurrent_mA[it->first] 
    <<";TimeElapsedForThisSTA_us="<<staMacToTimeElapsed_us[it->first] 
    <<";TimeAwakeForThisSTA_us="<<staMacToTimeAwake_us[it->first] 
    <<";UL_throughput_kbps="<<staMacToUplinkDownlinkThroughput_kbps[it->first].first 
    <<";DL_throughput_kbps="<<staMacToUplinkDownlinkThroughput_kbps[it->first].second 
    <<";UL_latency_usPerPkt="<<staMacToUplinkDownlinkLatency_usPerPkt[it->first].first 
    <<";DL_latency_usPerPkt="<<staMacToUplinkDownlinkLatency_usPerPkt[it->first].second 
    <<'\n';
    // check if this STA MAC is in staMacToEnergyConsumed_J
    // if (staMacToEnergyConsumed_J.find(it->first) != staMacToEnergyConsumed_J.end())
    // {
    //   // print energy consumed by this STA
    //   std::cout << "Energy consumed by this STA (J): " << staMacToEnergyConsumed_J[it->first] << '\n';
    // }
  }
  

  Simulator::Destroy ();

  // Calculating and tracing goodput for all Server Apps
  // std::stringstream ssGoodputLog;
  // ssGoodputLog <<FOLDER_PATH<< "log"<<currentLoopIndex_string<<".metriclog";
  // ssGoodputLog <<FOLDER_PATH<< "log"<<".metriclog";
  // static std::fstream f1 (ssGoodputLog.str ().c_str (), std::ios::out);
  // static std::fstream f1 (ssGoodputLog.str ().c_str (), std::ios::app);



  std::cout<<"\n\n\n\n";
  std::cout<<std::setw(30) << std::right << "scenario" << ":\t" << scenario << std::endl;
  std::cout<<std::setw(30) << std::right << "loopIndex" << ":\t" << loopIndex << std::endl;
  std::cout<<std::setw(30) << std::right << "randSeed" << ":\t" << randSeed << std::endl;
  std::cout<<std::setw(30) << std::right << "UseCase" << ":\t" << useCase << std::endl;
  std::cout<<std::setw(30)<< std::right << "Transport Protocol" << ":\t" <<protocol << std::endl;
  std::cout<<std::setw(30) << std::right << "#STAs" << ":\t" << nStations << std::endl;
  // std::cout<<std::setw(30) << std::right << "packetsPerSecond/STA" << ":\t" << packetsPerSecond << std::endl;
  std::cout<<std::setw(30) << std::right << "simulationTime" << ":\t" << simulationTime << std::endl;
  std::cout<<std::setw(30) << std::right << "p2pLinkDelay(ms)" << ":\t" << p2pLinkDelay << std::endl;

  // std::cout<<std::setw(30) << std::right << "Distance from AP" << ":\t" << distance <<" meters" << std::endl;
  // std::cout<<std::setw(30)<< std::right << "MCS"<<":\t"<<mcs << std::endl;
  // std::cout<<std::setw(30)<< std::right << "Freq. Band" << ":\t" << frequency << " GHz" << std::endl;
  // std::cout<<std::setw(30)<< std::right << "Channel Width" << ":\t" << channelWidth << " MHz" << std::endl;
  // std::cout<<std::setw(30)<< std::right << "GI" << ":\t" <<gi << " ns" << std::endl;
  
  
  // std::cout<<std::setw(30)<< std::right << "Data rate" << ":\t" <<dataRate << " bps" << std::endl;

  // std::cout<< std::endl<< std::setw(30)<< std::right << "Throughput"<< ":\t" << throughput << " Mbps" << std::endl;
  
  // Average results
  // double avgGoodput_kbps=0, avgCurrentAllSta_mA=0, avgLatency_us=0;


  // for (std::size_t i = 0; i < nStations; i++)
  // {
  //   avgGoodput_kbps += throughput[i];
  //   avgCurrentAllSta_mA += avgCurrent_mAForSTA[i];
  //   avgLatency_us += avgDelay_us[i];
  // }
  // avgGoodput_kbps = avgGoodput_kbps/nStations;
  // avgCurrentAllSta_mA = avgCurrentAllSta_mA/nStations;
  // avgLatency_us = avgLatency_us/nStations;



  // std::cout<<"\n\nAvg. Goodput (kbps) = "<<avgGoodput_kbps;
  // std::cout<<"\nAvgCurrentAllSta_mA = "<<avgCurrentAllSta_mA;
  // std::cout<<"\nAvgLatency_us = "<<avgLatency_us;
  // std::cout<<"\nDelayHistogram = ";
  // for (int i = 0; i < delayHist.GetNBins(); i++) {
  //   std::cout<<"\n"<<delayHist.GetBinStart(i)<<" to "<<delayHist.GetBinEnd(i)<<" has "<<delayHist.GetBinCount(i)<<" packets.";
  // }
  // std::cout<<"\n\n\n";
  
  
  std::cout<<"\n\nSimulation with Index "<<loopIndex<<" completed**********************************************************";
  std::cout<<"\n\n\n\n";
  return 0;
}
