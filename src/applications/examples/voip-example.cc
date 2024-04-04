/*
 * Copyright (c) 2024 Shyam K Venkateswaran (shyam1@gatech.edu), Roshni Garnayak (rgarnayak3@gatech.edu)
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
 * Authors: Shyam K Venkateswaran <shyam1@gatech.edu>, Roshni Garnayak <rgarnayak3@gatech.edu>
 */


// Example to demonstrate the working of the VoIP Application 
// Network topology: 
// Client -------------------------------------------- Server 
//               Configurable capacity and delay
// The topology has two nodes, a client and a server, interconnected by 
// a point-to-point link. The client node can send uplink VoIP traffic to the server, 
// and the server can send downlink VoIP traffic to the client. 
// Both uplink and downlink VoIP traffic can be enabled separately
// Exponential mean, Same state probability, frame intervals, and packet sizes can be configured for the VoIP traffic
// No jitter is introduced by the VoIP application.
// The delay and jitter histograms will be printed at the end of the simulation for each flow. 
// Please disregard the flow corresponding to the TCP ACKs when analyzing the results. 

// Example Usage: ./ns3 run "voip-example --activePktSize=35 --silentPktSize=10 --simTimeSec=100 --sameStateProbability=0.8 --exponentialMean=0.5 --activeFrameInterval_ms=10 --silentFrameInterval_ms=200"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VoipApplicationExample");

int main(int argc, char *argv[]) 
{
    double simTimeSec = 30;                 // Simulation time in seconds
    double p2pLinkDelay_ms = 10;            // Delay of P2P link between the server node and the client node in ms
    bool enableVoipUplink = false;          // Uplink enabled if true
    bool enableVoipDownlink = true;        // Downlink enabled if true
    bool enablePcap = false;                // Enable pcap if true
    
    // VoIP traffic parameters
    double exponentialMean = 1.25;        // The exponential mean for active/silent state duration
    double sameStateProbability = 0.984;  // The probability of staying in the same state
    uint32_t activePktSize = 33;          // Active state packet size in bytes
    uint32_t silentPktSize = 7;          // Silent state packet size in bytes
    double activeFrameInterval_ms = 20;   // Active state frame interval in ms
    double silentFrameInterval_ms = 160;   // Silent state frame interval in ms

    

    // Parse command line arguments
    CommandLine cmd;
    cmd.AddValue ("simTimeSec", "Simulation time in seconds", simTimeSec);
    cmd.AddValue ("enableVoipUplink", "Uplink VoIP traffic enabled for the client node if true", enableVoipUplink);
    cmd.AddValue ("enableVoipDownlink", "Downlink VoIP traffic enabled for the client node if true", enableVoipDownlink);
    cmd.AddValue ("enablePcap", "Enable pcap if true", enablePcap);
    cmd.AddValue ("p2pLinkDelay_ms", "Delay of P2P link between the server node and the client node", p2pLinkDelay_ms);
    cmd.AddValue ("exponentialMean", "The exponential mean for active/silent state duration. Default = 1.25", exponentialMean);
    cmd.AddValue ("sameStateProbability", "The probability of staying in the same state. Default = 0.984", sameStateProbability);
    cmd.AddValue ("activePktSize", "Active state VoIP packet size in bytes. Default = 33", activePktSize);
    cmd.AddValue ("silentPktSize", "Silent state VoIP packet size in bytes. Default = 7", silentPktSize);
    cmd.AddValue ("activeFrameInterval_ms", "Active state frame interval in ms. Default = 20", activeFrameInterval_ms);
    cmd.AddValue ("silentFrameInterval_ms", "Silent state frame interval in ms. Default = 160", silentFrameInterval_ms);
    cmd.Parse(argc, argv);

    // LogComponentEnable ("VoiPApplication", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
    
    // Create nodes
    NodeContainer nodes;
    nodes.Create(2);


    Ptr<Node> serverNode = nodes.Get(0);
    Ptr<Node> clientNode = nodes.Get(1);

    

    // Create a P2P link between server and client node
    PointToPointHelper p2pLink;
    p2pLink.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2pLink.SetChannelAttribute("Delay", TimeValue(MilliSeconds(p2pLinkDelay_ms)));
    NetDeviceContainer devices;
    devices = p2pLink.Install(serverNode, clientNode);
    
    // Install internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Set TCP segment size to 1500 bytes
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1500));

    ApplicationContainer serverApp_UL, serverApp_DL;
    
    // Setting up TCP VoIP traffic
    if (enableVoipUplink)
    {
        uint16_t port = 50000;
    
        // Server APP at the Remote server
        Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
        PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
        serverApp_UL = sinkHelper.Install (serverNode);
        serverApp_UL.Start (Seconds (0.0));
        serverApp_UL.Stop (Seconds (simTimeSec));
        

        // Client App
        VoiPHelper voipUL ("ns3::TcpSocketFactory", (InetSocketAddress (interfaces.GetAddress (0), port)));
        voipUL.SetAttribute ("ExponentialMean", DoubleValue (exponentialMean));
        voipUL.SetAttribute ("SameStateProbability", DoubleValue (sameStateProbability));
        voipUL.SetAttribute ("ActivePktSize", UintegerValue (activePktSize));
        voipUL.SetAttribute ("SilentPktSize", UintegerValue (silentPktSize));
        voipUL.SetAttribute ("ActiveFrameInterval", TimeValue (MilliSeconds(activeFrameInterval_ms)));
        voipUL.SetAttribute ("SilentFrameInterval", TimeValue (MilliSeconds(silentFrameInterval_ms)));

        ApplicationContainer clientApp = voipUL.Install (clientNode);

        clientApp.Start (Seconds (0));        
        clientApp.Stop (Seconds (simTimeSec));
    }
    if (enableVoipDownlink)
    {
        // Downlink TCP VoIP traffic setup
        uint16_t port = 60000;
        Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
        PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
        serverApp_DL =sinkHelper.Install (clientNode);
        serverApp_DL.Start (Seconds (0.0));
        serverApp_DL.Stop (Seconds (simTimeSec));

        // Client App
        VoiPHelper voipDL ("ns3::TcpSocketFactory", (InetSocketAddress (interfaces.GetAddress (1), port)));
        voipDL.SetAttribute ("ExponentialMean", DoubleValue (exponentialMean));
        voipDL.SetAttribute ("SameStateProbability", DoubleValue (sameStateProbability));
        voipDL.SetAttribute ("ActivePktSize", UintegerValue (activePktSize));
        voipDL.SetAttribute ("SilentPktSize", UintegerValue (silentPktSize));
        voipDL.SetAttribute ("ActiveFrameInterval", TimeValue (MilliSeconds(activeFrameInterval_ms)));
        voipDL.SetAttribute ("SilentFrameInterval", TimeValue (MilliSeconds(silentFrameInterval_ms)));
        ApplicationContainer clientApp = voipDL.Install (serverNode);

        clientApp.Start (Seconds (0));
        clientApp.Stop (Seconds (simTimeSec));
    }

    // Enable packet capture
    if (enablePcap)
    {
        p2pLink.EnablePcapAll("voip-example");
    }
    

    // FlowMonitor setup
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    
    // Start simulation
    Simulator::Stop(Seconds(simTimeSec));
    Simulator::Run();


    // Printing Flow Monitor statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    for (auto i = stats.begin(); i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> "
                    << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / simTimeSec / 1000 / 1000
                    << " Mbps\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / simTimeSec / 1000 / 1000
                    << " Mbps\n";
        
        // Print the delay histogram
        Histogram delayHist = i->second.delayHistogram;
        std::cout<< "Delay Histogram\n";
        std::cout<< std::setw(15)<<std::left << "Bin Start (s)"<<" |\t";
        std::cout<< std::setw(15)<<std::left << "Bin End (s)"<<" |\t";
        std::cout<< std::setw(15)<<std::left << "Packet Count"<<"\n";
        
        for (int histBin = 0; histBin < delayHist.GetNBins(); histBin++) 
        {
            std::cout<< std::setw(15)<<std::left << delayHist.GetBinStart(histBin)<<" |\t";
            std::cout<< std::setw(15)<<std::left << delayHist.GetBinEnd(histBin)<<" |\t";
            std::cout<< std::setw(15)<<std::left << delayHist.GetBinCount(histBin)<<"\n";
        }
        std::cout<<"-----------------\n\n";

        // Print the jitter histogram
        Histogram jitterHist = i->second.jitterHistogram;
        std::cout<< "Jitter Histogram\n";
        std::cout<< std::setw(15)<<std::left << "Bin Start (s)"<<" |\t";
        std::cout<< std::setw(15)<<std::left << "Bin End (s)"<<" |\t";
        std::cout<< std::setw(15)<<std::left << "Packet Count"<<"\n";

        for (int histBin = 0; histBin < jitterHist.GetNBins(); histBin++) 
        {
            std::cout<< std::setw(15)<<std::left << jitterHist.GetBinStart(histBin)<<" |\t";
            std::cout<< std::setw(15)<<std::left << jitterHist.GetBinEnd(histBin)<<" |\t";
            std::cout<< std::setw(15)<<std::left << jitterHist.GetBinCount(histBin)<<"\n";
        }
        std::cout<<"-----------------\n\n";
    }

    Simulator::Destroy();

    return 0;
}
