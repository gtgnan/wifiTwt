/*
 * Copyright (c) 2024 Roshni Garnayak <rgarnayak3@gatech.edu>
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
 * Author: Roshni Garnayak <rgarnayak3@gatech.edu>
 *
 * This is a Voice over IP (VoIP) traffic generator application
 * The model is based on the evaluation methodology proposed by IEEE TGax
 * Model details can be found at: https://mentor.ieee.org/802.11/dcn/14/11-14-0571-12-00ax-evaluation-methodology.docx
 * For latest updates from the Task Group, visit https://www.ieee802.org/11/Reports/tgax_update.htm
 */

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "voip-application.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"
#include "ns3/double.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VoiPApplication");

NS_OBJECT_ENSURE_REGISTERED (VoiPApplication);

TypeId
VoiPApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VoiPApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<VoiPApplication> ()
    .AddAttribute ("ActivePktSize", "Size of packets in bytes while in Active State.",
                   UintegerValue (33),
                   MakeUintegerAccessor (&VoiPApplication::m_activePktSize),
                   MakeUintegerChecker<uint32_t>())
    .AddAttribute ("SilentPktSize", "Size of packets in bytes while in Silent State.",
                   UintegerValue (7),
                   MakeUintegerAccessor (&VoiPApplication::m_silentPktSize),
                   MakeUintegerChecker<uint32_t>())
    .AddAttribute ("ExponentialMean", "The exponential mean for active/silent state duration.",
                   DoubleValue (1.25),
                   MakeDoubleAccessor (&VoiPApplication::m_expMean),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("SameStateProbability", "The probability to stay in the same state.",
                   DoubleValue (0.984),
                   MakeDoubleAccessor (&VoiPApplication::m_ssprob),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ActiveFrameInterval", "Frame interval in active state.",
                   TimeValue (MilliSeconds (20)),
                   MakeTimeAccessor (&VoiPApplication::m_activeFrameInterval),
                   MakeTimeChecker())
    .AddAttribute ("SilentFrameInterval", "Frame interval in silent state.",
                   TimeValue (MilliSeconds (160)),
                   MakeTimeAccessor (&VoiPApplication::m_silentFrameInterval),
                   MakeTimeChecker())
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&VoiPApplication::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("Local",
                   "The Address on which to bind the socket. If not set, it is generated automatically.",
                   AddressValue (),
                   MakeAddressAccessor (&VoiPApplication::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol", "The type of protocol to use. This should be "
                   "a subclass of ns3::SocketFactory",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&VoiPApplication::m_tid),
                   // This should check for SocketFactory as a parent
                   MakeTypeIdChecker ())
    .AddAttribute ("EnableSeqTsSizeHeader",
                   "Enable use of SeqTsSizeHeader for sequence number and timestamp",
                   BooleanValue (false),
                   MakeBooleanAccessor (&VoiPApplication::m_enableSeqTsSizeHeader),
                   MakeBooleanChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&VoiPApplication::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("TxWithAddresses", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&VoiPApplication::m_txTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
    .AddTraceSource ("TxWithSeqTsSize", "A new packet is created with SeqTsSizeHeader",
                     MakeTraceSourceAccessor (&VoiPApplication::m_txTraceWithSeqTsSize),
                     "ns3::PacketSink::SeqTsSizeCallback")
  ;
  return tid;
}


VoiPApplication::VoiPApplication ()
  : m_socket (0),
    m_connected (false),
    m_activeFrameInterval (MilliSeconds (20)),
    m_silentFrameInterval (MilliSeconds (160)),
    m_activePktSize (33),
    m_silentPktSize (7),
    m_totBytes (0),
    m_unsentPacket (0)
{
  NS_LOG_FUNCTION (this);
  m_currState = 1;
  m_nextState = m_currState;
  double min = 0.0;
  double max = 1.0;
  
  randU->SetAttribute ("Min", DoubleValue (min));
  randU->SetAttribute ("Max", DoubleValue (max));
}

VoiPApplication::~VoiPApplication()
{
  NS_LOG_FUNCTION (this);
}


Ptr<Socket>
VoiPApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

void
VoiPApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
  m_socket = 0;
  m_unsentPacket = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void VoiPApplication::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      int ret = -1;

      if (! m_local.IsInvalid())
        {
          NS_ABORT_MSG_IF ((Inet6SocketAddress::IsMatchingType (m_peer) && InetSocketAddress::IsMatchingType (m_local)) ||
                           (InetSocketAddress::IsMatchingType (m_peer) && Inet6SocketAddress::IsMatchingType (m_local)),
                           "Incompatible peer and local address IP version");
          ret = m_socket->Bind (m_local);
        }
      else
        {
          if (Inet6SocketAddress::IsMatchingType (m_peer))
            {
              ret = m_socket->Bind6 ();
            }
          else if (InetSocketAddress::IsMatchingType (m_peer) ||
                   PacketSocketAddress::IsMatchingType (m_peer))
            {
              ret = m_socket->Bind ();
            }
        }

      if (ret == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }

      m_socket->Connect (m_peer);
      m_socket->SetAllowBroadcast (true);
      m_socket->ShutdownRecv ();

      m_socket->SetConnectCallback (
        MakeCallback (&VoiPApplication::ConnectionSucceeded, this),
        MakeCallback (&VoiPApplication::ConnectionFailed, this));
    }

  // Insure no pending event
  CancelEvents ();
  StateStart ();
}

void VoiPApplication::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
  if (m_socket != nullptr)
    {
      m_socket->Close ();
    }
  else
    {
      NS_LOG_WARN ("VoiPApplication found null socket to close in StopApplication");
    }
}

void VoiPApplication::CancelEvents ()
{
  NS_LOG_FUNCTION (this);

  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_stateStartEvent);
  // Canceling events may cause discontinuity in sequence number if the
  // SeqTsSizeHeader is header, and m_unsentPacket is true
  if (m_unsentPacket)
    {
      NS_LOG_DEBUG ("Discarding cached packet upon CancelEvents ()");
    }
  m_unsentPacket = 0;
}

// Private helpers
void VoiPApplication::ScheduleNextTx ()
{
  NS_LOG_FUNCTION (this);
  Time nextTime;
  if (m_currState == ACTIVE_STATE) {
    nextTime = m_activeFrameInterval;
  }
  else {
    nextTime = m_silentFrameInterval;
  }
  NS_LOG_INFO ("Next packet scheduled after " << nextTime << " seconds. Current state = " << m_currState);
  m_sendEvent = Simulator::Schedule (nextTime, &VoiPApplication::SendPacket, this);
}

void VoiPApplication::GenerateNextState()
{
    uint32_t r = randU->GetValue() * 1000;
    NS_LOG_INFO ("Random number generated: " << r);
    if (r < m_ssprob*1000) 
    {
      NS_LOG_INFO ("r = " << r << " < " << m_ssprob*1000 << " = ssprob*1000; State is unchanged");

      m_nextState = m_currState;
    }
    else 
    {
      NS_LOG_INFO ("r = " << r << " > " << m_ssprob*1000 << " = ssprob*1000; State is changed");
        if (m_currState == ACTIVE_STATE)
        {
          m_nextState = SILENT_STATE;
        }
        else {
          m_nextState = ACTIVE_STATE;
        }
        
    }

    NS_LOG_INFO ("Current state " << m_currState << ", Next state " << m_nextState);
}

void VoiPApplication::StateStart()
{  
  // Schedules the event to start sending data (switch to the "On" state)
  NS_LOG_FUNCTION (this);
  
  m_currState = m_nextState;
  GenerateNextState();

  double stateDuration = m_expMean * -log(randU->GetValue());
  m_stateStartEvent = Simulator::Schedule (Seconds(stateDuration), &VoiPApplication::StateStart, this);
  NS_LOG_INFO ("m_expMean = " << m_expMean << "; State duration " << stateDuration);
  Time currentFrameInterval;
  if (m_currState == ACTIVE_STATE) {
    currentFrameInterval = m_activeFrameInterval;
  }
  else {
    currentFrameInterval = m_silentFrameInterval;
  }
  m_numTotalPkts = floor(stateDuration/currentFrameInterval.GetSeconds());
  m_numRemainingPkts = m_numTotalPkts;
  NS_LOG_INFO ("Total number of packets to be transmitted in this state: " << m_numTotalPkts);
  if (m_numTotalPkts > 0) {
    m_sendEvent = Simulator::Schedule (Seconds(0.0), &VoiPApplication::SendPacket, this);
  }
}

void VoiPApplication::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

  if (m_currState == ACTIVE_STATE) {
    m_pktSize = m_activePktSize;
    NS_LOG_INFO ("Active packet size " << m_pktSize);
  }
  else {
    m_pktSize = m_silentPktSize;
    NS_LOG_INFO ("Silent packet size " << m_pktSize);
  }

  Ptr<Packet> packet;
  if (m_unsentPacket)
    {
      packet = m_unsentPacket;
      NS_LOG_INFO ("There was an unsent packet");
    }
  else if (m_enableSeqTsSizeHeader)
    {
      NS_LOG_INFO ("sqts header enabled");
      Address from, to;
      m_socket->GetSockName (from);
      m_socket->GetPeerName (to);
      SeqTsSizeHeader header;
      header.SetSeq (m_seq++);
      header.SetSize (m_pktSize);
      NS_LOG_INFO ("seq " << header.GetSeq ());
      NS_LOG_INFO ("pkt size as in header" << header.GetSize ());
      NS_ABORT_IF (m_pktSize < header.GetSerializedSize ());
      packet = Create<Packet> (m_pktSize - header.GetSerializedSize ());
      NS_LOG_INFO ("packet size after subtracting header size" << packet->GetSize ());
      // Trace before adding header, for consistency with PacketSink
      m_txTraceWithSeqTsSize (packet, from, to, header);
      packet->AddHeader (header);
      NS_LOG_INFO ("subtracted packet size");
    }
  else
    {
      packet = Create<Packet> (m_pktSize);
      NS_LOG_INFO ("original packet size "<< m_pktSize);
    }

  int actual = m_socket->Send (packet);
  if ((unsigned) actual == m_pktSize)
    {
      m_txTrace (packet);
      m_totBytes += m_pktSize;
      m_unsentPacket = 0;
      Address localAddress;
      m_socket->GetSockName (localAddress);
      if (InetSocketAddress::IsMatchingType (m_peer))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S)
                       << " VoiP application sent "
                       <<  packet->GetSize () << " bytes to "
                       << InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (m_peer).GetPort ()
                       << " total Tx " << m_totBytes << " bytes");
          m_txTraceWithAddresses (packet, localAddress, InetSocketAddress::ConvertFrom (m_peer));
        }
      else if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S)
                       << " VoiP application sent "
                       <<  packet->GetSize () << " bytes to "
                       << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (m_peer).GetPort ()
                       << " total Tx " << m_totBytes << " bytes");
          m_txTraceWithAddresses (packet, localAddress, Inet6SocketAddress::ConvertFrom(m_peer));
        }
    }
  else
    {
      NS_LOG_DEBUG ("Unable to send packet; actual " << actual << " size " << m_pktSize << "; caching for later attempt");
      m_unsentPacket = packet;
    }
  
  m_numRemainingPkts--;
  NS_LOG_INFO ("Number of packets remaining in this state: " << m_numRemainingPkts);
  if (m_numRemainingPkts > 0) ScheduleNextTx ();
}


void VoiPApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void VoiPApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_FATAL_ERROR ("Can't connect");
}


} // Namespace ns3
