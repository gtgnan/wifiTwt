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


// Example to demonstrate the working of the Video Application 
// Network topology: 
// Client ------------------------------------------------------- Server 
//               P2P link: Configurable capacity and delay
// The topology has two nodes, a client and a server, interconnected by 
// a point-to-point link. The client node can send uplink video traffic to the server, 
// and the server can send downlink video traffic to the client. 
// Both uplink and downlink video traffic can be enabled separately
// The video quality can be selected via command line arguments. 
// The delay and jitter histograms will be printed at the end of the simulation for each flow. 
// Please disregard the flow corresponding to the TCP ACKs when analyzing the results. 
// The video traffic parameters (such as frame rate, video quality, etc.) can be tweaked.
// Example usage: ./ns3 run "video-example --simTimeSec=30 --enableVideoUplink=true --enableVideoDownlink=true --enablePcap=false --p2pLinkDelay_ms=10 --videoQuality=1 --videoFrameInterval_s=0.0333"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VideoApplicationExample");

int main(int argc, char *argv[]) 
{
    double simTimeSec = 30;                 // Simulation time in seconds
    double p2pLinkDelay_ms = 10;            // Delay of P2P link between the server node and the client node in ms
    bool enableVideoUplink = false;          // Uplink enabled if true
    bool enableVideoDownlink = true;        // Downlink enabled if true
    bool enablePcap = false;                // Enable pcap if true

    // Video traffic parameters
    uint8_t videoQuality = 1; // Video quality, bv1 to bv6
    double videoFrameInterval_s = 0.0333; // for 30 fps
    double videoWeibullScale = 6950; // Will change for each video quality 
    // BV1 - 6950, BV2 - 13900, BV3 - 20850, BV4 - 27800, BV5 - 34750, BV6 - 54210
    double videoWeibullShape = 0.8099;

    // Parse command line arguments
    CommandLine cmd;
    cmd.AddValue ("simTimeSec", "Simulation time in seconds", simTimeSec);
    cmd.AddValue ("enableVideoUplink", "Uplink video traffic enabled for the client node if true", enableVideoUplink);
    cmd.AddValue ("enableVideoDownlink", "Downlink video traffic enabled for the client node if true", enableVideoDownlink);
    cmd.AddValue ("enablePcap", "Enable pcap if true", enablePcap);
    cmd.AddValue ("p2pLinkDelay_ms", "Delay of P2P link between the server node and the client node", p2pLinkDelay_ms);
    cmd.AddValue ("videoQuality", "integer: 1 through 6 for bv1 through bv6", videoQuality);
    cmd.AddValue ("videoFrameInterval_s", "Double: Frame interval based on frame rate. 0.03333 for 30 FPS", videoFrameInterval_s);
    cmd.Parse(argc, argv);

    // LogComponentEnable ("VideoApplication", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));

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
    
    // Setting up TCP Video traffic
    if (enableVideoUplink)
    {
        uint16_t port = 50000;
    
        // Server APP at the Remote server
        Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
        PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
        serverApp_UL = sinkHelper.Install (serverNode);
        serverApp_UL.Start (Seconds (0.0));
        serverApp_UL.Stop (Seconds (simTimeSec));
        

        // Client App
        VideoHelper videoUL ("ns3::TcpSocketFactory", (InetSocketAddress (interfaces.GetAddress (0), port)));
        
        videoUL.SetAttribute ("FrameInterval", TimeValue (Seconds(videoFrameInterval_s)));
        videoUL.SetAttribute ("WeibullScale", DoubleValue (videoWeibullScale));
        videoUL.SetAttribute ("WeibullShape", DoubleValue (videoWeibullShape));
        ApplicationContainer clientApp = videoUL.Install (clientNode);

        clientApp.Start (Seconds (0));        
        clientApp.Stop (Seconds (simTimeSec));
    }
    if (enableVideoDownlink)
    {
        // Downlink TCP Video traffic setup
        uint16_t port = 60000;
        Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
        PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
        serverApp_DL =sinkHelper.Install (clientNode);
        serverApp_DL.Start (Seconds (0.0));
        serverApp_DL.Stop (Seconds (simTimeSec));

        // Client App
        VideoHelper videoDL ("ns3::TcpSocketFactory", (InetSocketAddress (interfaces.GetAddress (1), port)));

        videoDL.SetAttribute ("FrameInterval", TimeValue (Seconds(videoFrameInterval_s)));
        videoDL.SetAttribute ("WeibullScale", DoubleValue (videoWeibullScale));
        videoDL.SetAttribute ("WeibullShape", DoubleValue (videoWeibullShape));
        ApplicationContainer clientApp = videoDL.Install (serverNode);

        clientApp.Start (Seconds (0));
        clientApp.Stop (Seconds (simTimeSec));
    }

    // Enable packet capture
    if (enablePcap)
    {
        p2pLink.EnablePcapAll("video-example");
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
