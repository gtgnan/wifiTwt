/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2024 SHYAM K VENKATESWARAN
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
 * Author: Shyam K Venkateswaran <vshyamkrishnan@gmail.com>
 */

// This simulation scenario consists of nStations STAs in a WiFi 6 BSS. 
// nStations will be varied from 4 to 32 in steps of 4.
// The STAs offers uplink video traffic at configured video quality V1. 
// PHY rate is set to HE MCS 7 with 20 MHz bandwidth and 800 ns GI.
// HeMcs7 at 20 MHz bandwidth with 800 ns GI and 1 Spatial stream results in 86 Mbps PHY rate - refer https://mcsindex.com/
// Using implicit individual non Trigger Based TWT, 4 scenarios can be realized:
// dTWT-1, dTWT-2, dTWT-3, and CAM
// First 10 seconds are used for setup and will not be considered for metrics
// Metrics of interest are throughput, 95th percentile latency, and energy per bit
// Run the shell script wns3VaryLoadScript.sh to generate the raw results found in logs/wns3VaryLoadResults.log
// Run the Jupyter notebook logs/wns3VaryPlotting.ipynb to process the raw results and generate plots

// Usage example: ./ns3 run "wns3VaryLoad --simId=1000 --randSeed=1000 --simulationTime=40 --useCase=dTwt1 --nStations=8 --videoQuality=1 --parallelSim=false"

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
#include <sys/stat.h>

#define FOLDER_PATH "scratch/"
#define LOG_PATH "logs/"

// This simulation is modified from examples/wireless/wifi-he-network.cc

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wifi6Net");

uint32_t simId = 10002;
uint32_t randSeed = 7000;      // Seed for Random Number Generator
bool parallelSim = false;      // Run simulations in parallel - Separate log files are generated for each run based on simId if true; Else, a single log file is appended to

// Power Scheme ----------------------------------

std::string useCase = "dTwt1";    // Pick between dTwt1, dTwt2, dTwt3, and cam




std::string scenario = "wns3VaryLoad";    // used for logging and processing. Get from command line argument

// Power Scheme ----------------------------------


// Simulation configuration ----------------------

std::string currentsimId_string;
Time keepTrackOfMetricsFrom_S = Seconds (10); // Start time for current logging in seconds
double delayBinWidth_s = 0.0001;   // 100 microseconds
// std::string currentsimId_string = "010001";
// Simulation configuration ----------------------



// Network configuration -------------------------
std::size_t nStations {1};              // number of stations
double simulationTime {10 + 60};        //seconds  - 10 seconds for setup
double roomLength = 10.0;               // 2D room - side length in meters
uint32_t p2pLinkDelay {0};              //p2p link delay in ms -- RTT will be double of this
Time beaconInterval = MicroSeconds(102400);  // 102.4 ms as beacon interval
Time beaconIntervalEffective = MicroSeconds(100400);  // 100.4 ms will be used for TWT scheduling - assume 2 ms for beacon transmission
std::string dlAckSeqType;
// std::string dlAckSeqType = "NO-OFDMA";   // DL Ack Sequence Type
uint32_t bsrLife_ms = 10;                // in ms - BSR life time
// uint32_t packetsPerSecond = 50;        // UL Packets per second per station
uint32_t ampduLimitBytes = 20000;       //50,000 at 143 Mbps and 5/6 coding takes 3.4 ms
// Network configuration -------------------------


// Transport Protocol parameters -----------------------------
uint32_t payloadSize = 1500;                       /* Transport layer payload size in bytes. */
// Transport Protocol -----------------------------


// Video traffic parameters
bool enableVideoUplink = true;
uint32_t videoQuality = 1; // Video quality, bv1 to bv6
double videoFrameInterval_s = 0.0333; // for 30 fps
double videoWeibullScale = 6950; // Will change for each video quality 
// BV1 - 6950, BV2 - 13900, BV3 - 20850, BV4 - 27800, BV5 - 34750, BV6 - 54210
double videoWeibullShape = 0.8099;


// Logging ---------------------------------------
bool enablePcap {false};            // Enable pcap traces if true
bool enableStateLogs = false;      // Enable state logs if true
bool enableFlowMon = true;          // Enable flow monitor if true
bool recordApPhyState = false;     // If true, AP PHY states are also logged to the stateLog file (as StaId=0)
bool linkStatusLogging = false;      // Enable link status logging if true




Time firstTwtSpStart = (82.5 * beaconInterval);    // // TWT is set up after app traffic starts
// Time firstTwtSpStart = (32.5 * beaconInterval);   // TWT is set up before app traffic starts
Time firstTwtSpOffsetFromBeacon = MilliSeconds (2);    // Offset from beacon for first TWT SP
bool twtTriggerBased = false; // Set it to false for contention-based TWT
uint64_t maxMuSta = 1;      // Maximum number of STAs the TWT scheduler will assign for a DATA MU exchange. For BSRP, max possible is used
double twtWakeIntervalMultiplier, twtNominalWakeDurationDivider;
double nextStaTwtSpOffsetDivider = 1;
uint32_t staCountModulusForTwt = 1;    // Offset for next STA TWT start is added only after each staCountModulusForTwt STAs

// App timing to start only

Time AppStartTimeMin = (55 * beaconInterval) + MilliSeconds (3);    
Time AppStartTimeMax = (65 * beaconInterval) + MilliSeconds (3);    



// TI device model
std::unordered_map<std::string, double>TI_currentModel_mA = {
  {"IDLE", 50}, {"CCA_BUSY", 50}, {"RX", 66}, {"TX", 232}, {"SLEEP", 0.12}
};
double *timeElapsedForSta_ns_TI, *awakeTimeElapsedForSta_ns_TI, *sleepTimeElapsedForSta_ns_TI, *current_mA_TimesTime_ns_ForSta_TI, *uplinkTimeoutsForSta, downlinkTimeoutsAllSta, *uplinkExpiredMpduForSta, downlinkExpiredMpduAllSta, *uplinkFailedEnqueueMpduForSta, downlinkFailedEnqueueMpduAllSta;    // This is only after keepTrackOfMetricsFrom_S


// Server Apps
ApplicationContainer serverApp;

uint64_t *totalRxBytes;
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
// Given a Histogram object, return the bin end value that first exceeds xth percentile

double
GetPercentileValue (Histogram hist, double percentile)
{
  NS_ASSERT_MSG (percentile >= 0 && percentile <= 100, "Percentile should be between 0 and 100");
  double sum = 0;
  for (uint32_t i = 0; i < hist.GetNBins (); i++)
    {
      sum += hist.GetBinCount (i);
    }
  double target = sum * percentile / 100;
  sum = 0;
  for (uint32_t i = 0; i < hist.GetNBins (); i++)
    {
      sum += hist.GetBinCount (i);
      if (sum >= target)
        {
          return hist.GetBinEnd (i);
        }
    }
  return hist.GetBinEnd (hist.GetNBins () - 1);
}

//-**************************************************************************
//-**************************************************************************
// PHY state tracing 
//template <int node>
void PhyStateTrace (std::string context, Time start, Time duration, WifiPhyState state)
{
  std::stringstream ss;
  // ss <<FOLDER_PATH<< "log"<<currentsimId_string<<".statelog";
  ss <<FOLDER_PATH<< "log.statelog";

  static std::fstream f (ss.str ().c_str (), std::ios::out);
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
  // Do not use spaces. Use '='  and ';'
  if (Simulator::Now ().GetMicroSeconds()>keepTrackOfMetricsFrom_S.GetMicroSeconds())
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
    else
    {
      // Add to sleep time for this STA
      sleepTimeElapsedForSta_ns_TI[nodeId - 1] += duration.GetNanoSeconds();
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
  ss <<FOLDER_PATH<<"log"<<currentsimId_string<<".txlog";

  static std::fstream f (ss.str ().c_str (), std::ios::out);

  // f << Simulator::Now ().GetSeconds () << "    state=" << state << " start=" << start << " duration=" << duration << std::endl;
  // Do not use spaces. Use '='  and ';'
  if (state == WifiPhyState::TX)
  {
    f<<"startTime_ns="<<start.GetNanoSeconds()<<";duration_ns=" << duration.GetNanoSeconds()<< std::endl;
  }
  
}

//-******************************************
void PsduResponseTimeoutTraceSta (std::string context, uint8_t reason, Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    // Handle the psduResponseTimeout event here.
    // For example, you can log the event or update some statistics.
    // std::cout << "t = "<<Simulator::Now().As(Time::MS)<<";staId="<<ContextToNodeId (context) <<"; PSDU response timeout. Reason: " << static_cast<int>(reason);
    // // Print the source and destination addresses of the PSDU
    // std::cout << "\n\tSource address: " << psdu->GetAddr2();
    // std::cout << "\n\tDestination address: " << psdu->GetAddr1() << std::endl;
    if (Simulator::Now ().GetMicroSeconds()>keepTrackOfMetricsFrom_S.GetMicroSeconds())
    {
      uplinkTimeoutsForSta[ContextToNodeId (context) - 1]++;
    }
    
}
void PsduResponseTimeoutTraceAp (std::string context, uint8_t reason, Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    // Handle the psduResponseTimeout event here.
    // For example, you can log the event or update some statistics.
    // std::cout << "t = "<<Simulator::Now().As(Time::MS)<<";staId="<<ContextToNodeId (context) <<"; PSDU response timeout. Reason: " << static_cast<int>(reason);
    // // Print the source and destination addresses of the PSDU
    // std::cout << "\n\tSource address: " << psdu->GetAddr2();
    // std::cout << "\n\tDestination address: " << psdu->GetAddr1() << std::endl;
    if (Simulator::Now ().GetMicroSeconds()>keepTrackOfMetricsFrom_S.GetMicroSeconds())
    {
      downlinkTimeoutsAllSta++;
    }
    
}
//-**************************************************************************
//-**************************************************************************
void MpduDropped_atSta(std::string context, WifiMacDropReason dropReason,Ptr<const WifiMpdu> mpdu)
{
    if (Simulator::Now ().GetMicroSeconds()>keepTrackOfMetricsFrom_S.GetMicroSeconds())
    {
      if (dropReason == WifiMacDropReason::WIFI_MAC_DROP_EXPIRED_LIFETIME)
      {
        uplinkExpiredMpduForSta[ContextToNodeId (context) - 1]++;
      }
      if (dropReason == WifiMacDropReason::WIFI_MAC_DROP_FAILED_ENQUEUE)
      {
        uplinkFailedEnqueueMpduForSta[ContextToNodeId (context) - 1]++;
      }
      
    }
    // std::cout << "MPDU expired at STA; t = "<<Simulator::Now().As(Time::MS)<<"; dropReason:"<<+dropReason<<";staId="<< ContextToNodeId (context)-1 <<std::endl;
}
void MpduDropped_atAp(std::string context, WifiMacDropReason dropReason,Ptr<const WifiMpdu> mpdu)
{
    if (Simulator::Now ().GetMicroSeconds()>keepTrackOfMetricsFrom_S.GetMicroSeconds())
    {
      if (dropReason == WifiMacDropReason::WIFI_MAC_DROP_EXPIRED_LIFETIME)
      {
        downlinkExpiredMpduAllSta++;
      }
      if (dropReason == WifiMacDropReason::WIFI_MAC_DROP_FAILED_ENQUEUE)
      {
        downlinkFailedEnqueueMpduAllSta++;
      }
      
    }
    // std::cout << "MPDU expired at AP; t = "<<Simulator::Now().As(Time::MS)<<"; dropReason:"<<+dropReason<<";staId="<< ContextToNodeId (context)-1 <<std::endl;
}



void 
initiateTwtAtApAndSta (Ptr<WifiMac> apMac, Ptr<WifiMac> staMac, u_int8_t flowId, Time twtWakeInterval, Time twtNominalWakeDuration, Time nextTwtOffsetFromNextBeacon)
{
  Mac48Address apMacAddress = apMac->GetAddress();
  Mac48Address staMacAddress = staMac->GetAddress();

  apMac -> SetTwtSchedule (flowId, staMacAddress, false, true, true, false, true, 0, twtWakeInterval, twtNominalWakeDuration, nextTwtOffsetFromNextBeacon); 
  staMac -> SetTwtSchedule (flowId, apMacAddress, true, true, true, false, true, 0, twtWakeInterval, twtNominalWakeDuration, nextTwtOffsetFromNextBeacon); 
}
//-*************************************************************************




int main (int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.AddValue ("simId", "The index of current interation. Integer between 0 and 999999", simId);
  cmd.AddValue ("randSeed", "Random seed to initialize position and app start times - unit32", randSeed);
  cmd.AddValue ("parallelSim", "True to run simulations in parallel - Separate log files are generated for each run based on simId if true; Else, a single log file is appended to", parallelSim);
  cmd.AddValue ("scenario", "Used for Logging and grouping for batch processing.",scenario);
  cmd.AddValue ("useCase", "Pick between dTwt1, dTwt2, dTwt3, and cam",useCase);
  cmd.AddValue ("nStations", "Number of non-AP HE stations; Set to 1", nStations);
  cmd.AddValue ("simulationTime", "Simulation time in seconds- first 10 seconds is for setup", simulationTime);
  cmd.AddValue ("p2pLinkDelay", "Delay of P2P link between Ap and App server in ms", p2pLinkDelay);
  cmd.AddValue ("videoQuality", "integer: 1 through 6 for bv1 through bv6", videoQuality);
  cmd.AddValue ("enableStateLogs", "State logs generated if true",enableStateLogs);
  cmd.AddValue ("enablePcap", "PCAP for AP generated if true",enablePcap);
  // cmd.AddValue ("mcs", "A specific MCS (0-11)", mcs);
  
  cmd.AddValue ("maxMuSta", "Max number of STAs the AP can trigger in one MU_UL with Basic TF", maxMuSta);
  cmd.AddValue ("twtWakeIntervalMultiplier", "double K, where wakeInterval = BI * K", twtWakeIntervalMultiplier);

  cmd.Parse (argc,argv);



  std::cout<<"\n\nSimulation with Index "<<simId<<" Started**********************************************************\n\n\n";

  // Check if configuration is valid
  NS_ASSERT_MSG (nStations > 0, "nStations should be > 0");
  
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




  
  
  //*******************************************************
  // Logging if necessary
  
// LogComponentEnable ("WifiHelper", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("StaWifiMac", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("ApWifiMac", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("WifiMac", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("WifiMacQueue", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
// LogComponentEnable ("WifiMpdu", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
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
// LogComponentEnable ("ArpL3Protocol", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
  //*******************************************************
  
  totalRxBytes = new uint64_t[nStations];
  throughput = new double[nStations];

  // Energy model init
  timeElapsedForSta_ns_TI = new double[nStations];
  awakeTimeElapsedForSta_ns_TI = new double[nStations];
  sleepTimeElapsedForSta_ns_TI = new double[nStations];
  current_mA_TimesTime_ns_ForSta_TI = new double[nStations];
  uplinkTimeoutsForSta = new double[nStations];
  uplinkExpiredMpduForSta = new double[nStations];
  uplinkFailedEnqueueMpduForSta = new double[nStations];
  
  for (std::size_t i = 0; i <nStations; i++)
  {
    timeElapsedForSta_ns_TI[i] = 0;
    awakeTimeElapsedForSta_ns_TI[i] = 0;
    sleepTimeElapsedForSta_ns_TI[i] = 0;
    current_mA_TimesTime_ns_ForSta_TI[i] = 0;
    uplinkTimeoutsForSta[i] = 0;
    uplinkExpiredMpduForSta[i] = 0;
    
  }
  downlinkTimeoutsAllSta = 0;
  downlinkExpiredMpduAllSta = 0;
  downlinkFailedEnqueueMpduAllSta = 0;

  bool enableTwt = false;

  if (useCase == "dTwt1" || useCase == "dTwt2" || useCase == "dTwt3")
  {
    enableTwt = true;
  }
  else if (useCase == "cam")
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
  

  bool useRts {false};
//   bool useExtendedBlockAck {false};


  // double distance {5.0}; //meters
  double frequency {5}; //whether 2.4, 5 or 6 GHz
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
  // HeMcs7 at 20 MHz bandwidth with 800 ns GI and 1 Spatial stream gives 86 Mbps PHY rate - refer https://mcsindex.com/

  std::ostringstream oss;
  oss << "HeMcs" << mcs;


  
  // uint32_t staMaxMissedBeacon = 10000;                 // Set the max missed beacons for STA before attempt for reassociation
//   Time AdvanceWakeupPS = MicroSeconds (10);

  std::string phyModel {"Spectrum"};
  





  std::stringstream indexStringTemp;
  indexStringTemp << std::setfill('0') << std::setw(6) << simId;
  currentsimId_string = indexStringTemp.str();


  
  Config::SetDefault ("ns3::ArpCache::MaxRetries", UintegerValue (20));
  Config::SetDefault ("ns3::ArpCache::WaitReplyTimeout", TimeValue(MilliSeconds(1000)));
  Config::SetDefault ("ns3::WifiMacQueue::MaxDelay", TimeValue (MilliSeconds (1000)));    // Default is 500 ms - changed because frames timed out and TCP connection kept dropping. 
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


  // /* Configure TCP Options */
  // Config::SetDefault ("ns3::TcpSocket::DelAckTimeout", TimeValue (MilliSeconds (delACKTimer_ms)));
  // // Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (Seconds (1)));    //"TCP retransmission timeout when opening connection (seconds) - default 3 seconds"
  // Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (MilliSeconds(200)));    //"TCP retransmission timeout when opening connection (seconds) - default 3 seconds"
  // Config::SetDefault ("ns3::TcpSocket::DataRetries", UintegerValue (20)); 
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
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (ossDataMode.str()), "ControlMode", StringValue ("HeMcs0"));
  // wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss.str ()), "ControlMode", StringValue (oss.str ()));
  // wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss.str ()), "ControlMode", StringValue (oss.str ()));
  // wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager", "RtsCtsThreshold", UintegerValue (65535));
  Ssid ssid = Ssid ("ns3-80211ax");


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
              "MaxMissedBeacons", UintegerValue(std::numeric_limits<uint32_t>::max()),  // To avoid disassociation
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
  // for (u_int32_t ii = 0; ii < wifiStaNodes.GetN() ; ii++)
  // {
    
  //   std::stringstream nodeIndexStringTemp, maxBcnStr, advWakeStr, apsdStr;
  //   nodeIndexStringTemp << wifiStaNodes.Get(ii)->GetId();
  //   // maxBcnStr = "/NodeList/" + std::string(ii+1) + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/MaxMissedBeacons";
  //   maxBcnStr << "/NodeList/" << nodeIndexStringTemp.str() << "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/MaxMissedBeacons";
  //   // advWakeStr << "/NodeList/" << nodeIndexStringTemp.str() << "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/AdvanceWakeupPS";

  //   Config::Set (maxBcnStr.str(), UintegerValue(staMaxMissedBeacon));

  // }

  // Config::Set ("/NodeList/1/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/MaxMissedBeacons", UintegerValue(staMaxMissedBeacon));
  // Config::Set ("/NodeList/1/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/AdvanceWakeupPS", TimeValue(AdvanceWakeupPS));

  



  // Setup TWT schedule
  // if (useCase == "twt")
  // {
  //   for (uint32_t i=0 ; i < wifiStaNodes.GetN(); i++)
  //   {
  //     Ptr<WifiNetDevice> apDevice = DynamicCast<WifiNetDevice>(ApNode->GetDevice(1));
  //     Ptr<WifiMac> apMac = apDevice->GetMac();
      
  //     Ptr<WifiNetDevice> staDevice = DynamicCast<WifiNetDevice>(wifiStaNodes.Get(i)->GetDevice(0));
  //     Ptr<WifiMac> staMac = staDevice->GetMac();
      
  //     // Setting up TWT at AP and STA
  //     Time nextTwtOffsetFromNextBeacon = firstTwtSpOffsetFromBeacon;
  //     Time twtWakeInterval = beaconInterval*twtWakeIntervalMultiplier;
  //     Time twtNominalWakeDuration = beaconIntervalEffective/twtNominalWakeDurationDivider;
      
  //     Simulator::Schedule (firstTwtSpStart, &initiateTwtAtApAndSta, apMac, staMac, 0, twtWakeInterval, twtNominalWakeDuration, nextTwtOffsetFromNextBeacon);

  //     // Print TWT schedule information for each STA
  //     std::cout<<"\nTWT agreement for STA: "<<i<<"\n\tInitiated at t = "<<firstTwtSpStart.As(Time::S) << ";\n\tTWT SP starts at "<<nextTwtOffsetFromNextBeacon.As(Time::MS)<<" after next beacon;\n\tTWT Wake Interval = "<< twtWakeInterval.As(Time::MS)<<";\n\tTWT Nominal Wake Duration = "<< twtNominalWakeDuration.As(Time::MS)<<";";
  //   }
  // }
  if (useCase == "dTwt1")
  {
    Time twtNominalWakeDuration = beaconIntervalEffective/16;
    Time twtWakeInterval = nStations * beaconInterval/16;
    for (uint32_t i=0 ; i < wifiStaNodes.GetN(); i++)
    {
      Ptr<WifiNetDevice> apDevice = DynamicCast<WifiNetDevice>(ApNode->GetDevice(1));
      Ptr<WifiMac> apMac = apDevice->GetMac();
      
      Ptr<WifiNetDevice> staDevice = DynamicCast<WifiNetDevice>(wifiStaNodes.Get(i)->GetDevice(0));
      Ptr<WifiMac> staMac = staDevice->GetMac();
      
      // Setting up TWT at AP and STA
      Time nextTwtOffsetFromNextBeacon = firstTwtSpOffsetFromBeacon + (i * twtNominalWakeDuration);
      if (nextTwtOffsetFromNextBeacon >= beaconInterval - firstTwtSpOffsetFromBeacon)
      {
        nextTwtOffsetFromNextBeacon += firstTwtSpOffsetFromBeacon;
      }
      
      Simulator::Schedule (firstTwtSpStart, &initiateTwtAtApAndSta, apMac, staMac, 0, twtWakeInterval, twtNominalWakeDuration, nextTwtOffsetFromNextBeacon);

      // Print TWT schedule information for each STA
      std::cout<<"\nTWT agreement for STA: "<<i<<"\n\tInitiated at t = "<<firstTwtSpStart.As(Time::S) << ";\n\tTWT SP starts at "<<nextTwtOffsetFromNextBeacon.As(Time::MS)<<" after next beacon;\n\tTWT Wake Interval = "<< twtWakeInterval.As(Time::MS)<<";\n\tTWT Nominal Wake Duration = "<< twtNominalWakeDuration.As(Time::MS)<<";";
    }
  }
  else if (useCase == "dTwt2")
  {
    Time twtNominalWakeDuration = 2 * beaconIntervalEffective/nStations;
    Time twtWakeInterval = 2 * beaconInterval;
    for (uint32_t i=0 ; i < wifiStaNodes.GetN(); i++)
    {
      Ptr<WifiNetDevice> apDevice = DynamicCast<WifiNetDevice>(ApNode->GetDevice(1));
      Ptr<WifiMac> apMac = apDevice->GetMac();
      
      Ptr<WifiNetDevice> staDevice = DynamicCast<WifiNetDevice>(wifiStaNodes.Get(i)->GetDevice(0));
      Ptr<WifiMac> staMac = staDevice->GetMac();
      
      // Setting up TWT at AP and STA
      Time nextTwtOffsetFromNextBeacon = firstTwtSpOffsetFromBeacon + (i * twtNominalWakeDuration);
      if (nextTwtOffsetFromNextBeacon >= beaconInterval - firstTwtSpOffsetFromBeacon)
      {
        nextTwtOffsetFromNextBeacon += firstTwtSpOffsetFromBeacon;
      }
      
      Simulator::Schedule (firstTwtSpStart, &initiateTwtAtApAndSta, apMac, staMac, 0, twtWakeInterval, twtNominalWakeDuration, nextTwtOffsetFromNextBeacon);

      // Print TWT schedule information for each STA
      std::cout<<"\nTWT agreement for STA: "<<i<<"\n\tInitiated at t = "<<firstTwtSpStart.As(Time::S) << ";\n\tTWT SP starts at "<<nextTwtOffsetFromNextBeacon.As(Time::MS)<<" after next beacon;\n\tTWT Wake Interval = "<< twtWakeInterval.As(Time::MS)<<";\n\tTWT Nominal Wake Duration = "<< twtNominalWakeDuration.As(Time::MS)<<";";
    }
  }
  else if (useCase == "dTwt3")
  {
    Time twtNominalWakeDuration = beaconIntervalEffective/16;
    Time twtWakeInterval = 2 * beaconInterval;
    for (uint32_t i=0 ; i < wifiStaNodes.GetN(); i++)
    {
      Ptr<WifiNetDevice> apDevice = DynamicCast<WifiNetDevice>(ApNode->GetDevice(1));
      Ptr<WifiMac> apMac = apDevice->GetMac();
      
      Ptr<WifiNetDevice> staDevice = DynamicCast<WifiNetDevice>(wifiStaNodes.Get(i)->GetDevice(0));
      Ptr<WifiMac> staMac = staDevice->GetMac();
      
      // Setting up TWT at AP and STA
      Time nextTwtOffsetFromNextBeacon = firstTwtSpOffsetFromBeacon + (i * twtNominalWakeDuration);
      if (nextTwtOffsetFromNextBeacon >= beaconInterval - firstTwtSpOffsetFromBeacon)
      {
        nextTwtOffsetFromNextBeacon += firstTwtSpOffsetFromBeacon;
      }
      
      Simulator::Schedule (firstTwtSpStart, &initiateTwtAtApAndSta, apMac, staMac, 0, twtWakeInterval, twtNominalWakeDuration, nextTwtOffsetFromNextBeacon);

      // Print TWT schedule information for each STA
      std::cout<<"\nTWT agreement for STA: "<<i<<"\n\tInitiated at t = "<<firstTwtSpStart.As(Time::S) << ";\n\tTWT SP starts at "<<nextTwtOffsetFromNextBeacon.As(Time::MS)<<" after next beacon;\n\tTWT Wake Interval = "<< twtWakeInterval.As(Time::MS)<<";\n\tTWT Nominal Wake Duration = "<< twtNominalWakeDuration.As(Time::MS)<<";";
    }
  }

  
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
    std::cout<<"\n\tPosition of TWT STA "<<ii<<" : [ "<< currentX<<", "<<currentY <<", 0.0 ];\n";
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
  std::cout << "\n\nIP to MAC mapping:\n";
  for (auto it = ipToMac.begin (); it != ipToMac.end (); it++)
  {
    std::cout << it->first << " => " << it->second << '\n';
  }

  // Setting up Video TCP UL traffic
  if (enableVideoUplink)
  {
    // Generate a random time between AppStartTimeMin and AppStartTimeMax
    Ptr<UniformRandomVariable> thisAppStartTime_ms = CreateObject<UniformRandomVariable> ();
    thisAppStartTime_ms->SetAttribute ("Min", DoubleValue (AppStartTimeMin.GetMilliSeconds()));
    thisAppStartTime_ms->SetAttribute ("Max", DoubleValue (AppStartTimeMax.GetMilliSeconds()));
    uint16_t port = 50000;
    for (std::size_t i = 0; i < nStations; i++)
    {
      // Server APPs at the Remote server
      Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port+i));
      PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
      ApplicationContainer tempServerApp =sinkHelper.Install (p2pServerNode);
      tempServerApp.Start (Seconds (0.0));
      tempServerApp.Stop (Seconds (simulationTime + 1));
      serverApp.Add (tempServerApp);

      
      // Client App
      VideoHelper videoUL ("ns3::TcpSocketFactory", (InetSocketAddress (p2pNodeInterfaces.GetAddress (1), port+i)));
      
      videoUL.SetAttribute ("FrameInterval", TimeValue (Seconds(videoFrameInterval_s)));
      videoUL.SetAttribute ("WeibullScale", DoubleValue (videoWeibullScale));
      videoUL.SetAttribute ("WeibullShape", DoubleValue (videoWeibullShape));
      ApplicationContainer clientApp = videoUL.Install (wifiStaNodes.Get (i));

      clientApp.Start (MilliSeconds (thisAppStartTime_ms->GetValue ()));
      clientApp.Stop (Seconds (simulationTime));
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

  // Tracing retries: PSDU
  for (uint32_t i=0 ; i < wifiStaNodes.GetN(); i++)
  {
    std::stringstream nodeIndexStringTemp, macStr;
    nodeIndexStringTemp << wifiStaNodes.Get(i)->GetId();
    macStr << "/NodeList/" << nodeIndexStringTemp.str() << "/DeviceList/*/$ns3::WifiNetDevice/Mac/PsduResponseTimeout";
    Config::Connect (macStr.str(), MakeCallback (&PsduResponseTimeoutTraceSta));
    
  }

  // PSDU timeout trace for AP - for downlink timeouts
  std::stringstream nodeIndexStringTemp, macStr;
  nodeIndexStringTemp << wifiApNodes.Get(0)->GetId();
  macStr << "/NodeList/" << nodeIndexStringTemp.str() << "/DeviceList/*/$ns3::WifiNetDevice/Mac/PsduResponseTimeout";
  Config::Connect (macStr.str(), MakeCallback (&PsduResponseTimeoutTraceAp));

  // Tracing MPDU drops at STAs
  for (uint32_t i=0 ; i < wifiStaNodes.GetN(); i++)
  {
    std::stringstream nodeIndexStringTemp, macStr;
    nodeIndexStringTemp << wifiStaNodes.Get(i)->GetId();
    macStr << "/NodeList/" << nodeIndexStringTemp.str() << "/DeviceList/*/$ns3::WifiNetDevice/Mac/DroppedMpdu";
    Config::Connect (macStr.str(), MakeCallback (&MpduDropped_atSta));
  }
  
  // At AP
  std::stringstream nodeIndexStringTemp2, macStr2;
  nodeIndexStringTemp2 << wifiApNodes.Get(0)->GetId();
  macStr2 << "/NodeList/" << nodeIndexStringTemp2.str() << "/DeviceList/*/$ns3::WifiNetDevice/Mac/DroppedMpdu";
  Config::Connect (macStr2.str(), MakeCallback (&MpduDropped_atAp));

  // State log trace for AP - for channel idle probability

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
    flowmon.SetMonitorAttribute("StartTime", TimeValue(keepTrackOfMetricsFrom_S));
    flowmon.SetMonitorAttribute("DelayBinWidth", DoubleValue(delayBinWidth_s)); 
    monitor = flowmon.InstallAll();
  
  }
  
  Simulator::Stop (Seconds (simulationTime));

  Simulator::Run ();

  std::cout<<"\n****************************\n";

  // std::stringstream ssNodeLog;
  // ssNodeLog <<FOLDER_PATH<< "log"<<".nodelog";
  // static std::fstream g (ssNodeLog.str ().c_str (), std::ios::app);
  double *avgCurrent_mAForSTA = new double [nStations];
  double *totEnergyConsumedForSta_J = new double [nStations];
  
  // Map of STA MAC address to current in mA
  std::map<Mac48Address, double> staMacToCurrent_mA;
  std::map<Mac48Address, double> staMacToTimeElapsed_us;
  std::map<Mac48Address, double> staMacToTimeAwake_us;
  std::map<Mac48Address, double> staMacToTimeAsleep_us;
  // Map of STA MAC address to energy consumed in J
  std::map<Mac48Address, double> staMacToEnergyConsumed_J;
  std::map<Mac48Address, double> staMacToUplinkTimeoutsThisSta;
  std::map<Mac48Address, double> staMacToDownlinkTimeoutsAllSta;
  std::map<Mac48Address, double> staMacToUplinkMpduExpiredThisSta;
  std::map<Mac48Address, double> staMacToDownlinkMpduExpiredAllSta;
  std::map<Mac48Address, double> staMacToUplinkMpduFailedEnqueueThisSta;
  std::map<Mac48Address, double> staMacToDownlinkMpduFailedEnqueueAllSta;
  
  // create a map from STA mac address to the pair - uplink and downlink
  std::map<Mac48Address, std::pair<double, double>> staMacToTotalBitsUplinkDownlink;
  std::map<Mac48Address, std::pair<double, double>> staMacToUplinkDownlinkThroughput_kbps;
  std::map<Mac48Address, std::pair<double, double>> staMacToUplinkDownlinkLatency_usPerPkt;
  std::map<Mac48Address, std::pair<double, double>> staMacToUplinkDownlink90Latency_us;   // 90th percentile
  std::map<Mac48Address, std::pair<double, double>> staMacToUplinkDownlink95Latency_us;   // 95th percentile
  std::map<Mac48Address, std::pair<double, double>> staMacToUplinkDownlink99Latency_us;   // 99th percentile
  // Initialize for all existing STAs with <0,0>
  for (uint32_t i = 0; i < wifiStaNodes.GetN(); i++)
  {
    Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (wifiStaNodes.Get (i)->GetDevice (0)); //assuming only one device
    Ptr<WifiMac> wifi_mac = wifi_dev->GetMac ();
    Ptr<StaWifiMac> sta_mac = DynamicCast<StaWifiMac> (wifi_mac);
    staMacToTotalBitsUplinkDownlink[sta_mac->GetAddress ()] = std::make_pair (0, 0);
    staMacToUplinkDownlinkThroughput_kbps[sta_mac->GetAddress ()] = std::make_pair (0, 0);
    staMacToUplinkDownlinkLatency_usPerPkt[sta_mac->GetAddress ()] = std::make_pair (0, 0);
    staMacToUplinkDownlink90Latency_us[sta_mac->GetAddress ()] = std::make_pair (0, 0);
    staMacToUplinkDownlink95Latency_us[sta_mac->GetAddress ()] = std::make_pair (0, 0);
    staMacToUplinkDownlink99Latency_us[sta_mac->GetAddress ()] = std::make_pair (0, 0);
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
    staMacToTimeAsleep_us[currentMacAddress] = sleepTimeElapsedForSta_ns_TI[i]/1000.0;
    staMacToUplinkTimeoutsThisSta[currentMacAddress] = uplinkTimeoutsForSta[i];
    staMacToDownlinkTimeoutsAllSta[currentMacAddress] = downlinkTimeoutsAllSta;
    staMacToUplinkMpduExpiredThisSta[currentMacAddress] = uplinkExpiredMpduForSta[i];
    staMacToDownlinkMpduExpiredAllSta[currentMacAddress] = downlinkExpiredMpduAllSta;
    staMacToUplinkMpduFailedEnqueueThisSta[currentMacAddress] = uplinkFailedEnqueueMpduForSta[i];
    staMacToDownlinkMpduFailedEnqueueAllSta[currentMacAddress] = downlinkFailedEnqueueMpduAllSta;
    
    std::cout<<"For STA "<<i<<";MacAddress="<<currentMacAddress
    <<"; Elapsed time (ms) = "<<timeElapsedForSta_ns_TI[i]/1e6
    <<"; Awake time (ms) = "<<awakeTimeElapsedForSta_ns_TI[i]/1e6
    <<"; Effective Duty cycle = "<< (awakeTimeElapsedForSta_ns_TI[i]/timeElapsedForSta_ns_TI[i])
    <<"; Average current (mA) = "<<avgCurrent_mAForSTA[i]<<"; Total energy consumed (J) = "<<staMacToEnergyConsumed_J[currentMacAddress] <<"; Uplink timeouts = "<<uplinkTimeoutsForSta[i]<<"; Downlink timeouts for all STAs = "<<downlinkTimeoutsAllSta<<"; Uplink Expired MPDUs for this STA = "<<uplinkExpiredMpduForSta[i]<<"; Downlink Expired MPDUs for all STA = "<<downlinkExpiredMpduAllSta<<"; Uplink Failed Enqueue MPDUs for this STA = "<<uplinkFailedEnqueueMpduForSta[i]<<"; Downlink Failed Enqueue MPDUs for all STA = "<<downlinkFailedEnqueueMpduAllSta<<";\n";
  }

  // Flowmon ---------------------------------
  // monitor->SerializeToXmlFile("flow1.flowmon", false, true);

  monitor->CheckForLostPackets ();


  // std::cout<<"Reached here 1\n";
  int staWithZeroThroughput = 0;
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
   

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      
      double totalBitsRx = i->second.rxBytes * 8.0;
      // double throughputKbps =  i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1000;
      std::cout<< std::setw(30)<<std::left <<"simulationTime"<<":\t"<<simulationTime<<"\n";
      double throughputKbps =  i->second.rxBytes * 8.0 / ( (simulationTime) - keepTrackOfMetricsFrom_S.GetSeconds())/1000;
      double avgDelayMicroSPerPkt = i->second.delaySum.GetMicroSeconds()/i->second.rxPackets ;
      u_int32_t lostPackets = i->second.lostPackets;
      Histogram delayHist = i->second.delayHistogram;
      // Calculate 90th, 95th and 99th percentile latencies from histogram
      double latency90 = GetPercentileValue (delayHist, 90);
      double latency95 = GetPercentileValue (delayHist, 95);
      double latency99 = GetPercentileValue (delayHist, 99);

      

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
        staMacToUplinkDownlink90Latency_us[ipToMac[t.sourceAddress]].first = latency90 * 1e6;
        staMacToUplinkDownlink95Latency_us[ipToMac[t.sourceAddress]].first = latency95 * 1e6;
        staMacToUplinkDownlink99Latency_us[ipToMac[t.sourceAddress]].first = latency99 * 1e6;
      }

      std::cout<< std::setw(30)<<std::left << "Destination IP and Port" <<":\t"<<t.destinationAddress<<" , "<<t.destinationPort<<"\n";
      if (ipToMac.find(t.destinationAddress) != ipToMac.end())
      {
        std::cout<< std::setw(30)<<std::left << "Destination STA MAC" <<":\t"<<ipToMac[t.destinationAddress]<<"\n";
        // Add totalBitsRx to Uplink of this STA in staMacToTotalBitsUplinkDownlink
        staMacToTotalBitsUplinkDownlink[ipToMac[t.destinationAddress]].second += totalBitsRx;
        staMacToUplinkDownlinkThroughput_kbps[ipToMac[t.destinationAddress]].second = throughputKbps;
        staMacToUplinkDownlinkLatency_usPerPkt[ipToMac[t.destinationAddress]].second = avgDelayMicroSPerPkt;
        staMacToUplinkDownlink90Latency_us[ipToMac[t.destinationAddress]].second = latency90 * 1e6;
        staMacToUplinkDownlink95Latency_us[ipToMac[t.destinationAddress]].second = latency95 * 1e6;
        staMacToUplinkDownlink99Latency_us[ipToMac[t.destinationAddress]].second = latency99 * 1e6;
      }
      if (throughputKbps == 0)
      {
        staWithZeroThroughput++;
      }
      std::cout<< std::setw(30)<<std::left << "Throughput (kbps)" <<":\t"<<throughputKbps<<"\n";
      std::cout<< std::setw(30)<<std::left << "Total bits received" <<":\t"<<totalBitsRx<<"\n";
      std::cout<< std::setw(30)<<std::left << "Avg. Delay ( us/pkt)" <<":\t"<< avgDelayMicroSPerPkt << " us/pkt\n";
      std::cout<< std::setw(30)<<std::left << "Latency 90th percentile (us)" <<":\t"<< latency90 * 1e6 << " us\n";
      std::cout<< std::setw(30)<<std::left << "Latency 95th percentile (us)" <<":\t"<< latency95 * 1e6 << " us\n";
      std::cout<< std::setw(30)<<std::left << "Latency 99th percentile (us)" <<":\t"<< latency99 * 1e6 << " us\n";
      std::cout<< std::setw(30)<<std::left << "Lost Packets" <<":\t"<< lostPackets << " pkts\n";
      
      // std::cout<< "Delay Histogram\n";
      // std::cout<< std::setw(15)<<std::left << "Bin Start (s)"<<" |\t";
      // std::cout<< std::setw(15)<<std::left << "Bin End (s)"<<" |\t";
      // std::cout<< std::setw(15)<<std::left << "Packet Count"<<"\n";
      // // Bin End (s) | Packet Count\n";
      // for (int histBin = 0; histBin < delayHist.GetNBins(); histBin++) {
      //   std::cout<< std::setw(15)<<std::left << delayHist.GetBinStart(histBin)<<" |\t";
      //   std::cout<< std::setw(15)<<std::left << delayHist.GetBinEnd(histBin)<<" |\t";
      //   std::cout<< std::setw(15)<<std::left << delayHist.GetBinCount(histBin)<<"\n";

      //   // "<<delayHist.GetBinEnd(histBin)<<" | "<<delayHist.GetBinCount(histBin)<<"\n";
      // }
      
      // std::cout<<"\n\n";
      std::cout<<"-----------------\n\n";

      }
    // flowmon -------------------------------------------------

  }
  // Writing to a log file
  std::stringstream ssNodeLog;
  // If LOG_PATH does not exist, create the directory

  struct stat st = {0};

  if (stat(LOG_PATH, &st) == -1) 
  {
    mkdir(LOG_PATH, 0700);  // The 0700 parameter sets the permissions of the directory (read, write, and execute permissions for the owner).
  }


  if (parallelSim)
  {
    ssNodeLog <<LOG_PATH<< "wns3VaryLoadResults"<<currentsimId_string<<".log";
  }
  else
  {
    ssNodeLog <<LOG_PATH<< "wns3VaryLoadResults.log";  
  }
  // static std::fstream g (ssNodeLog.str ().c_str (), std::ios::app);
  // Open the file with std::ios::app | std::ios::out to ensure it's created if it doesn't exist.
  std::fstream g(ssNodeLog.str().c_str(), std::ios::app | std::ios::out);
    
  // Check if the file was successfully opened
  if (!g.is_open() || g.fail()) 
  {
    NS_ABORT_MSG ("Failed to open the log file for appending.");
  } 
  else 
  {
    for (auto it = staMacToTotalBitsUplinkDownlink.begin (); it != staMacToTotalBitsUplinkDownlink.end (); it++)
    {
      double totalBits = it->second.first  + it->second.second ;
      // print STA MAC and then Uplink and Downlink
      // std::cout 
      g
      <<"scenario="<<scenario
      <<";simID="<<currentsimId_string
      <<";nSTA="<<nStations
      <<";videoQuality="<<+videoQuality
      <<";useCase="<<useCase
      <<";twtWakeIntervalMultiplier="<<twtWakeIntervalMultiplier
      <<";twtNominalWakeDurationDivider="<<twtNominalWakeDurationDivider
      <<";nextStaTwtSpOffsetDivider="<<nextStaTwtSpOffsetDivider
      <<";simulationTime="<<simulationTime
      <<";randSeed="<<+randSeed
      <<";StaMacAddress="<< it->first 
      << ";UL_bits=" << it->second.first 
      << ";DL_bits=" << it->second.second 
      << ";total_bits=" << totalBits
      <<";energy_J="<< staMacToEnergyConsumed_J[it->first]
      <<";energyPerTotBit_JPerBit="<< staMacToEnergyConsumed_J[it->first]/totalBits
      <<";current_mA="<<staMacToCurrent_mA[it->first] 
      <<";TimeElapsedForThisSTA_us="<<staMacToTimeElapsed_us[it->first] 
      <<";TimeAwakeForThisSTA_us="<<staMacToTimeAwake_us[it->first] 
      <<";TimeAsleepForThisSTA_us="<<staMacToTimeAsleep_us[it->first] 
      <<";EffectiveDutyCycle="<<staMacToTimeAwake_us[it->first]/staMacToTimeElapsed_us[it->first]  
      <<";UL_throughput_kbps="<<staMacToUplinkDownlinkThroughput_kbps[it->first].first 
      <<";DL_throughput_kbps="<<staMacToUplinkDownlinkThroughput_kbps[it->first].second 
      <<";UL_latency_usPerPkt="<<staMacToUplinkDownlinkLatency_usPerPkt[it->first].first 
      <<";DL_latency_usPerPkt="<<staMacToUplinkDownlinkLatency_usPerPkt[it->first].second 
      <<";UL_latency_90th_us="<<staMacToUplinkDownlink90Latency_us[it->first].first 
      <<";DL_latency_90th_us="<<staMacToUplinkDownlink90Latency_us[it->first].second 
      <<";UL_latency_95th_us="<<staMacToUplinkDownlink95Latency_us[it->first].first 
      <<";DL_latency_95th_us="<<staMacToUplinkDownlink95Latency_us[it->first].second 
      <<";UL_latency_99th_us="<<staMacToUplinkDownlink99Latency_us[it->first].first 
      <<";DL_latency_99th_us="<<staMacToUplinkDownlink99Latency_us[it->first].second 
      <<";UplinkRetriesThisSta="<<staMacToUplinkTimeoutsThisSta[it->first] 
      <<";DownlinkRetriesAllSta="<<staMacToDownlinkTimeoutsAllSta[it->first] 
      <<";UplinkExpiredMpduThisSta="<<staMacToUplinkMpduExpiredThisSta[it->first] 
      <<";DownlinkExpiredMpduAllSta="<<staMacToDownlinkMpduExpiredAllSta[it->first] 
      <<";UplinkFailedEnqueueMpduForSta="<<staMacToUplinkMpduFailedEnqueueThisSta[it->first] 
      <<";DownlinkExpiredMpduAllSta="<<staMacToDownlinkMpduFailedEnqueueAllSta[it->first] 
      <<";DownlinkFailedEnqueueMpduAllSta="<<staWithZeroThroughput
      <<'\n';
    }
    if (g.fail()) 
    {
        NS_ABORT_MSG ("Failed to append to the log file");
    }
        
    // Explicitly close the file
    g.close();
  }
  
  
  Simulator::Destroy ();

  // Calculating and tracing goodput for all Server Apps
  // std::stringstream ssGoodputLog;
  // ssGoodputLog <<FOLDER_PATH<< "log"<<currentsimId_string<<".metriclog";
  // ssGoodputLog <<FOLDER_PATH<< "log"<<".metriclog";
  // static std::fstream f1 (ssGoodputLog.str ().c_str (), std::ios::out);
  // static std::fstream f1 (ssGoodputLog.str ().c_str (), std::ios::app);



  std::cout<<"\n\n";
  std::cout<<std::setw(30) << std::right << "Flows With Zero Throughput" << ":\t" << staWithZeroThroughput << std::endl;
  std::cout<<std::setw(30) << std::right << "scenario" << ":\t" << scenario << std::endl;
  std::cout<<std::setw(30) << std::right << "simId" << ":\t" << simId << std::endl;
  std::cout<<std::setw(30) << std::right << "randSeed" << ":\t" << randSeed << std::endl;
  std::cout<<std::setw(30) << std::right << "UseCase" << ":\t" << useCase << std::endl;
  std::cout<<std::setw(30) << std::right << "#STAs" << ":\t" << nStations << std::endl;
  // std::cout<<std::setw(30) << std::right << "packetsPerSecond/STA" << ":\t" << packetsPerSecond << std::endl;
  std::cout<<std::setw(30) << std::right << "simulationTime" << ":\t" << simulationTime << std::endl;
  std::cout<<std::setw(30) << std::right << "p2pLinkDelay(ms)" << ":\t" << p2pLinkDelay << std::endl;

  
  std::cout<<"\n\nSimulation with Index "<<simId<<" completed**********************************************************";
  std::cout<<"\n\n\n\n";
  return 0;
}
