# Journal while implementing PSM in ns-3


Look for #fixAsap #hardcode


## Configuring vscode

Use the following c++ configuration

```

{
    "configurations": [
        {
            "name": "Mac",
            "includePath": [
                "${workspaceFolder}/**"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/clang",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "macos-clang-arm64",
            "configurationProvider": "ms-vscode.cpptools",
            "macFrameworkPath": [
                "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks"
            ]
        }
    ],
    "version": 4
}

```

### Steps to build ns3 from scratch on MacOS:

```

./ns3 clean
CXXFLAGS="-std=c++17" ./ns3 configure --build-profile=debug --enable-examples --enable-tests --disable-werror
./ns3 build

```

Try without the CXXFLAGS if there is an error.

## bug

If an STA has queued data at AP when it switches from CAM to PSM, there will be an error.

With high congestion:

assert failed. cond="!m_wifiPhy->m_currentPreambleEvents.empty ()", +18.350865753s 1 file=/Users/shyam/opt/ns3_PSM/ns-3-dev/src/wifi/model/phy-entity.cc, line=817

assert failed. cond="now - m_lastRxStart <= GetSifs ()", +8.637602783s 1 file=/Users/shyam/opt/ns3_PSM/ns-3-dev/src/wifi/model/channel-access-manager.cc, line=606

assert failed. cond="now - m_lastRxStart <= GetSifs ()", +8.641612658s 1 file=/Users/shyam/opt/ns3_PSM/ns-3-dev/src/wifi/model/channel-access-manager.cc, line=606

Steps to build ns3.35 from scratch after dependencies are satisfied on Ubuntu:
```
./ns3 clean
CXXFLAGS="-std=c++17" ./ns3 configure --build-profile=debug --enable-examples --enable-tests --disable-werror
./ns3 build
```

If you add a new file into scratch, clean and rebuild for run it.

Make sure to run all tests: ```./ns3 --run test-runner```
Look for #forFuture tags - Have to deal with them in future. 
Look for #bug 
Search for 'shyam' in source files. These are to do lists.


To run PSM_MU.cc with an example set of parameters:
```
./ns3 --run "PSM_MU --simulationTime=5 --enablePSM_flag=true --enableTcpUplink=true --dataPeriod=0.33 --otherStaCount=10 --loopIndex=123321 --dataRatebps_other=31000 --enableMulticast=true --multicastInterval_ms=200"

./ns3 --run "PSM_MU --simulationTime=15 --enablePSM_flag=true --enableTcpUplink=true --dataPeriod=0.33 --otherStaCount=0 --loopIndex=123321 --dataRatebps_other=1 --enableMulticast=false --multicastInterval_ms=20000"

```
To redirect only console output to a file:
```
./ns3 --run "PSM_MU --simulationTime=5 --enablePSM_flag=false --enableTcpUplink=false --dataPeriod=0.33 --otherStaCount=0 --loopIndex=123321 --dataRatebps_other=1000 --enableMulticast=false --multicastInterval_ms=200" >>trial123.out
```
To redirect console output and logs:
```
./ns3 --run PSM_MU > log1.out 2>&1
```

## To build and run a new script

Add the script to scratch folder and run ```./ns3 configure```

## Note

If a beacon is missed while in PS, STA goes back to Sleep after m_PsBeaconTimeOut (5 ms) and reschedules a new beacon wake up event based on previous known beacon interval

## To do

[x] For UL while in PSM, STA should go to sleep after 802.11 ACK


[ ] #bug: In order to accomodate for Tx durations and to Rx ACK packets while in PS, a function StaWifiMac::SleepPsIfQueuesEmpty () that checks MAC queues every 10*DIFS is used. In future, have an interrupt mechanism to do this. 

    Try from this? QosFrameExchangeManager::NotifyReceivedNormalAck

[ ] #bug: AP has a built in 100us delay before delivering multicast packets after DTIM beacon Tx. Remove this number and have some mechanism to trigger multicast packet delivery after beacon Tx.

[ ] #bug: In PHY state logs, when PSM is enabled, two cases arise: 
      If there is an IDLE state before entering SLEEP, a CCA_BUSY state is also created with SLEEP state that starts with IDLE state.
      If previous state is not IDLE, a duplicate CCA_BUSY state with exact duration as SLEEP state is created.

      Solution for now: If there is CCA_BUSY after SLEEP in the log, ignore it.


[ ] If good beacon is not received, STA should go back to Sleep after a timeOut

[ ] Check TIM (and other) impementation for multiple STAs


[x] Set STA back to PHY sleep upon receiving 802.11 ACK and if MAC queue is empty
      See ```m_stationManager->RecordWaitAssocTxOk (from);``` at line 1173 in ap-wifi-mac.cc

[ ] #bug In ap-wifi-mac.cc, attributes PsUnicastBufferSize and PsUnicastBufferDropDelay are not working. They are now hardcoded in the file (line ~140). Make these attributes work. 
    Do same for Multicast.

[ ] When STAs dis-associate - clear the PS queue for that STA's remote station state.

[ ] Create TIM ostream overload in tim.cc for operator "<<" #bug




## Functional description of this version

> STA will go to PHY SLEEP and wake up when the MAC function ```staMac->SetPowerSaveMode(PSMenable)``` is called.

>> If already in PS (m_PSM flag = 1), the ```SetPowerSaveMode(true)``` function will return without any changes.

>> STA will enter PS through the function call only if it is in ASSOCIATED state.

> If STA becomes UNASSOCIATED due to too many beacon misses (defined by m_maxMissedBeacons) and if it is in PSM, it will switch out of PSM after the beaconWatchdog timer expires. It will not enter PSM by itself upon re-association

> The STA wakes up for every beacon while in PS. Upon every successful beacon reception, a function ```WakeForNextBeaconPS``` is scheduled based on beacon timestamp and beacon interval


> Informing AP of entering/exiting PSM

>> STA sends a Null frame while entering or exiting PSM with PS bit set appropriately.

>> If in PS, STA sets PS bit to 1 in all outgoing frames

> State maintenance at AP

>> A state m_PSM is create for all STAs in AP and is updated whenever any frame is received by looking at the Power management bit in the header.

>> Number of STAs in PS can be found at any time using the function WifiRemoteStationManager::GetNStaInPs.

> TIM/DTIM implementation

>> An attribute m_dtimPeriod is defined at AP and initialized to 1. Can be changed through Config::Set

>> DTIM count updates for every beacon. 

>> TIM is added as part of beacon - 4 octets. DTIM is added to ever beacon. Counts properly. Tested for DTIM periods 1, 3, and 5. Starts count at 0. wscript in wifi was modified to add tim.h and tim.cc

>> Bitmap is added and tested for unicast traffic. STA decodes properly. 

>> Multicast buffering bit is set if required. 

> Beacon reception

>> After receiving a beacon from the associated AP, STA goes to sleep if it is in PSM and if AID is not in TIM and if multicast bit is 0 for DTIM beacons.


>> If the beacon arrives before STA wakes up, STA just stays awake till next beacon. 

>> If beacon arrives later than expected, STA goes back to sleep after successfully receiving this late beacon.

>> The case when a good beacon is not received upon waking up from SLEEP while in PS is not handled yet. 

> In case of uplink traffic while PS = 1:

>> STA PHY resumes from sleep while MAC is in PS = 1 whenever a packet is enqueued at the MAC layer

>> STA schedules a function to check queues every 10*DIFS durations and sleeps is empty.

>> With TCP traffic of low RTT, TCP ACK is usually received while the STA is awake

> Buffering at AP

>> Two wifi-mac-queue objects are created for STAs that use PS - one for unicast, one for multicast. Size and drop delay of both queues is hardcoded in ap-wifi-mac.cc

> Unicast

>> Any packet to a PS STA is sent to the unicast buffer. Size and drop delay of buffer is hardcoded for now.

>> AID is set properly in the TIM of every beacon.

>> STA sends PS-POLL if it sees its AID in beacon. 

>> AP sees PS-POLL and sends one packet. More data field is set if more data is present for same STA.

>> STA receives packet, checks 'More data' field and sends more PS-POLLs if necessary.

>> If 'More data' is zero, Sleep is scheduled after SIFS to allow for ACK Tx. It works. 

>> If an STA comes out of PS and AP is informed, all packets in unicast buffer for that STA are forwarded immediately.

> Multicast

>> If there are any STAs in PS, any group addressed packet is not sent to the multicast buffer and not transmitted right away.

>> In the DTIM beacon, the multicast bit is set and the multicast packets are sent after a 100 us delay in succession. 'More data' field is set in all but last multicast packet.


### Bug in MU case - resolved

Scenario:

> Two STAs - one in PSM - same traffic profile - 3Packets/second

> Bug occurs in PSM STA with following message:

```assert failed. cond="IsStateIdle () || IsStateCcaBusy ()", +1.305919165s 1 file=../src/wifi/model/wifi-phy-state-helper.cc, line=401```

> If PSM is started after steady TCP flow starts, there is no bug. Starting PSM at 0.7 seconds -> has bug. Start at 2 seconds -> no bug.

> When PSM is started later, increasing other STA traffic does not create bugs. 10x, 100x creates no bugs.


STA received a preamble and was just put to SLEEP before receiving the data. Resolved by checking if STA is in SLEEP before receiving fields.

### Bug - STA sleeps sometimes before receiving 802.11 ACK - resolved by checking if State is Tx and rescheduling.



### BUG in phy-entity.cc - Resolved

Scenario: 
> No PSM

> There is one extra node with no traffic

> Or, there is no extra node

> One STA, not in PSM, has high traffic - 10x than 3 packets/second. 

Error is similar to:

```assert failed. cond="m_wifiPhy->m_currentPreambleEvents.size () == 1", +2.633023033s 1 file=../src/wifi/model/phy-entity.cc, line=784```

or

```assert failed. cond="!m_wifiPhy->m_currentPreambleEvents.empty ()", +9.596083033s 1 file=../src/wifi/model/phy-entity.cc, line=809```

> Under same simulation code, teh unmodified ns3 source (without PSM modification) does not throw this error under very high traffic.

Plan to debug:

1. Start with SU case - only one STA in simulation. 
2. Keep this file handy - roll back and search in git commits where this error is introduced. Find the exact change. 

Error occurs right during a beacon reception at STA. There is an StaWifiMac::Enqueue() function called during beacon preamble detection when this error occurs. 

Bug resolved - missing curly braces in sta-wifi-mac.cc in the Enqueue() function.





        



### Note

1. To direct the logs from stdout to a file:
```
./waf --run labSetupPSM > log1.out 2>&1
```
2. There is an attribute in *sta-wifi-mac.cc* -> m_maxMissedBeacons. If m_maxMissedBeacons beacons are missed consecutively, STA will try to reassociate upon receiving a beacon. Default is 10.
    
    For every beacon received, the beacon watchdog timer is reset back to m_maxMissedBeacons\*beaconInterval

    To change this attribute to 20:
    ```
    Config::Set ("/NodeList/1/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/MaxMissedBeacons", UintegerValue (20));
    ```
3. State log entries will be made only STA PHY switches out of it. If last PHY state is SLEEP and it never changes to something else, it will not appear on the log.

4. In order to print uint8_t variables to console, use ```unsigned (varName)
```

5. For bit wise operations to fill buffers, see wifi-mac-header.cc around lines 900

6. To set individual bits, see capability-information.cc

    Encode bits in the order seen in wireshark (as in the standards doc)

7. When AP has more than 1 MAC packet queued for an STA, it sends a couple of 'Action' frames that request for immediate Block ACKs before forwarding the packets.

7. If adding new header/source files, include it in the wscript file of correct module - wifi in this case.
## Sleep and wake by brute force on PHY

Using brute force to set PHY layer to sleep and back with following code:

```
void
putToSleep (Ptr<WifiPhy> phy)
{
  // This function puts the specific PHY to sleep
  phy->SetSleepMode();
}

void
wakeFromSleep (Ptr<WifiPhy> phy)
{
  // This function wakes the specific PHY from sleep
  phy->ResumeFromSleep();
}

// ------------ Sleep and wake up - manually
Simulator::Schedule (Seconds (2.0), &putToSleep, phy);
Simulator::Schedule (Seconds (3.0), &wakeFromSleep, phy);
  ```

Results: 

1. PHY layer is put to sleep and is woken up - this reflects on PHY state logs also.
2. Device de-associates from AP and associates back upon wake-up. This is because 10 (m_maxMissedBeacons) beacons are missed.

  This has been solved by changing the attribute MaxMissedBeacons for the STA.


### What does each log component print?

Example code for enabling a log component: 
```
LogComponentEnable ("StaWifiMac", LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL));
```

* __WifiHelper__: ASCII traces of all Wi-Fi nodes with transmit and receive info
* __StaWifiMac__: Comprehensive info of all MAC operations - including Tx and Rx - internal MAC updates
* __WifiPhy__: PHY layer info - power settings - sends- channel access requests
* __ApWifiMac__: Sends and receives at AP, Update rates and parameters - *Very long*
* __WifiMac__: Not very useful - DCF configurations



## PSM enable and disable using a function on MAC

Aim is to create a function that takes in true or false. True means STA MAC will enter PSM. 
```
void StaWifiMac::SetPowerSaveMode (bool enable)
```
A bool MAC attribute is created in sta-wifi-mac.h called m_PSM.
Above function changes this attribute after setting PHY states properly.

### Just enable and disable SLEEP state upon function calls. STA will not wake up for beacons (or anything by itself). It will just enter and exit SLEEP state upon function calls. 

Enabling PSM works - STA goes to sleep - PHY state log reflects this. Able to manually wake up by forcing the PHY function ``` void wakeFromSleep (Ptr<WifiPhy> phy)```

### Bug#1

m_phy is a zero pointer inside StaWifiMac::SetPowerSaveMode and StaWifiMac::GetPowerSaveMode

But, it is a valid pointer inside StaWifiMac:SetWifiPhy, which is defined above PSM functions in sta-wifi-mac.cc

  Order of definition does not matter. The order in which these functions are called matters. 

Running under LLDB, ```StaWifiMac:SetWifiPhy``` is called by the function ```node->AddDevice(device)``` inside ```WifiHelper::Install```

But, ```StaWifiMac::SetPowerSaveMode``` is called before ```SetWifiPhy``` because it is made as an *accessor for the attribute* and attribute accessors are initialized before installing on device in this line:

```
Ptr<WifiMac> mac = macHelper.Create (device, m_standard);
```

Solution:

m_PSM does not have to be an attribute. Just have two functions that can set/get this variable (private).
    But m_PSM has to be initialized to false when node is initialized.
        This is done in sta-wifi-mac.cc in the constructor ```StaWifiMac::StaWifiMac ()```: ```m_PSM (0)```

**m_PSM is no longer an attribute but just a private variable in sta-wifi-mac**

#forFuture Connect m_PSM to some sort of tracing system. 

Functions for entering and exiting SLEEP PHY state are added the sta-wifi-mac.cc and committed.


## Making the STA wake up for every beacon while in PSM.

First, access the data from each beacon --> find beacon period from previous beacon and print this from inside the MAC module - Done. 

Have a beacon timer (not the watchdog timer) that is always running irrespective of whether or not in PSM and is reset whenever a beacon is received with the received beacon interval. Ignore TIM and DTIM for now.
    How is this implemented?

    1. There is a new function in sta-wifi-mac called void WakeForNextBeaconPS (void) that is scheduled after every good beacon reception based on beacon timestamp and interval

    2. If STA is in PSM, it wakes up inside WakeForNextBeaconPS function for next beacon. Else, nothing is done.

What happens if a good beacon is not received? This case is not handled yet. The parameter ```m_maxBeaconRxWaitPS``` will be used for this.

## Handling beacon misses while in PS

#forFuture - This is not implemented completely  - create a timer that puts STA back to sleep after waiting a while for the beacon.
    
1. Currently if the beacon arrives before STA wakes up, STA just stays awake till next beacon. 
2. If beacon arrives later than expected, STA goes back to sleep after successfully receiving this late beacon.

#forFuture - When STA position is changed suddenly to be outside AP range, there are two PHY states printe in the state log for same timestamp and duration - both SLEEP and CCA_BUSY. This could be a bug.


## Waking up for uplink traffic

The STA has to wake up while staying in PSM whenever there is uplink traffic. 

A simple way to implement this is to resume PHY from Sleep whenever there is a packet Enqueued. 

*Going back to sleep after sending uplink data is pending*


## Sending Null frame with PS bit when entering/exiting PSM

Whenever SetPowerSave function is called and the PSM is to be changed, a null frame is sent to AP with the correct PM bit. The Pm bit was set/unset by creating functions in the wifi-mac-header model.


## Creating m_PSM state for each STA at AP

A state variable m_PSM was created inside wifi-remote-station-manager -> WifiRemoteStationState along with functions to set and get the state. 

Whenever any frame is received by AP MAC (in ap-wifi-mac), the SetPowerMgt function is called to update this state regardless of whether or not the state was changed. 

## Creating DTIM period attribute on AP

Created in ap-wifi-mac as uint8_t, defined as an attribute ```m_dtimPeriod```, and initialized to 1. 

With every call to ApWifiMac::SendOneBeacon(), m_dtimPeriod is printed to NS_LOG_DEBUG.

To print uint8_t to console, use unsigned (varName).

Attribute can be set using ```Config::Set ("/NodeList/0/DeviceList/1/$ns3::WifiNetDevice/Mac/$ns3::ApWifiMac/DtimPeriod", UintegerValue(apDtimPeriod));```

TIM element was added to beacon without the bitmap. 

## Creating a mac queue for each STA in wifi-remote-station-manager

Created a function to initialize a queue inside wifi-remote-station-manager for each STA upon association. 
An attribute is created in AP wifi mac to set the max queue size for each STA's PS queue. 

 