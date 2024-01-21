# Journal for implementing TWT

*Use the branch ```wifiTwt```

Look for //TODO #shyam, #fixAsap, #hardcode, #bug --- to fix

## To redirect console output and logs:

```
./ns3 --run PSM_MU > log1.out 2>&1
```

## To redirect only console output to a file:

```
./ns3 --run PSM_MU  >>trial123.out
```


## Tasks

- How to deal with failed MAC frames at the end of an SP? Currently they are being discarded. 
- Fix non trigger based TWT - Aim is to replicate the Bianchi analysis results with non TB TWT
    [x] Create simple example with non TB TWT - UDP only
    [x] AP should not send BSRP for non TB
    [ ] STA side - Do not take Txop greater than available time in SP
        [ ] Fetch remaining time in current TWT SP if TWT is enabled
    - AP side - Do not take Txop greater than available time in SP
    
- Make sure TWT agreement frames are sent - unlimited retries?
- Make TCP work after TWT schedule starts
    - TCP app should start after TWT is set up. Including ARP and TCP SYN
    - Create an eval script. Enable pcap.
- Video application sometimes fails in a congested network with TWT - check why
- Make sure last beacon was sent out before sending out TWT Setup frames from AP. 
- STA should not send ACK for TWT Accept frames from AP
    - Try changing WIFI_MAC_MGT_ACTION to WIFI_MAC_MGT_ACTION to WIFI_MAC_MGT_ACTION_NO_ACK for Accept frame
- redefine CheckTwtBufferForThisSta - call from station manager
- TWT unannounced 
- TWT teardown mechanism
- Make TWT triggerbased, announced to be per flow
- Add/set TWT support element in beacon
- If action TWT setup frame is received with same TWT flowId, existing schedule is not torn down
- In StaWifiMac::SetPhyToSleep, - handle cases that do not need ACK
- AP should not schedule/send UL or DL beyond TWT SP
    - This results in AP sending block ack req repeatedly
- With Video uplink traffic, some STAs randomly stop the traffic at bv6 when used with TWT


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

## To build and run a new script

Add the script to scratch folder and run ```./ns3 configure```

## Current implementation summary

wifi-default-ack-manager:230 - temporary changes to handle basic TF - has to be changed ASAP

TWT susp/resume handled at STA - states maintained.
    STA goes to SLEEP after TWT susp is ack'ed inside TWT SP.
TWT susp/resume handled at AP. States maintained. At the AP, STA is assumed to enter SLEEP if it sends susp.

## TWT: 

Announced TWT with STA pre-notification case is done.

TWT Resume/suspend state maintenane at STA handled only for SU case in frame exchange manager. If using MU frames, make changes to HE FeMan.

TWT susp and resume IFs are implemented. AP and STA side logic pending.


Bugs:

- [ ] After every 'SLEEP' Phy state, there is a 'CCA-BUSY' state added to the state log incorrectly.


## Creating the test network scenario - ```wifi6Net.cc```

Imported ```wifi-he-network.cc``` from examples as ```wifi6Net.cc```.

Following is the configuration:
- Frequency band = 2.4 GHz (can be 2.4 or 5 GHz)
- Channel width = 20 MHz (can be 40 GHz for 2.4GHz band and up to 160 GHz for 5 GHz band)
- Guard interval = 800 ns (can be 1600 or 3200 ns also)

## Adding TWT capability info AP and STA MAC - low priority

Pending

## Make STA go sleep and wake up periodically

- [x] Disable all traffic
- [x] When does first beacon start? 
    - First Beacon Tx always starts at 30000 ns
- [x] Beacons will not be received 
    - [x] Set max missed beacons to a high number 10,000
    - [ ] Disable disassociation if maxBeacons are missed
- [x] After association (t=1 second or later), set up twt schedule
    - [x] Sta will wake up and sleep periodically

## Create a TWT PS queue on STA to buffer all uplink traffic if TWT is active

- [x] Create a TWT PS queue at STA - copy from AP MAC
- [x] If TWT is active, divert all uplink STA packets to buffer
- [x] During TWT SP, see if there are packets in this buffer and send all
    - [x] Check with 1, 2, 5, 10, 100 TCP uplink packets per second
        - [x] 1
        - [x] 2
        - [x] 5
        - [x] 10
        - [x] 100

## Synchronizing TWT SP between AP and STA

Till now AP has no idea of TWT at STA
- [x] Create a public function with AP MAC that can accept TWT parameters - STA MAC address, min TWT wake duration, TWT wake interval - this function has to be scheduled along with the STA function
- [x] Create twt parameters and state variables for each STA at AP to keep track of twtAgreement, twtPeriod, twtMinWakeDuration, twtActive, twtAwakeForSp, twtAnnounced
- [x] Update twt state variables according to parameters using remote station manager for specific STA


## Buffer downlink packets to STA with TWT at AP till TWT SP

- [x] If twt agreement exists for this STA, and if m_twtAwakeForSp is false at AP, send packet to buffer
- [x] At beginning of every SP, if STA is known to be awake, send all unicast packets


## Sending Uplink packet from STA outside TWT SP

- [x] Create a flag in STA TWT parameters - if true, STA will wake up whenever there is uplink traffic, send it 
    - [x] Tested for 1, 2, 10, 20 packets per second - TCP ACk is send only during TWT SP
    - [x] And go back to sleep - Done through FeManager and QosFeManager
Note: Even if there are many uplink packets, STA sends all of them outside SP


## Waking up for all Beacons

- [x] With a flag, wake up for every beacon
- [x] Check if STA goes back to sleep after bcn rx
- [ ] Check if multicast DL packets are received after DTIM beacon


# UL outside TWT SP

- [x] If there is a frame inside TWT PS buffer at end of TWT SP and ```twtUplinkOutsideSp``` is true, the one frame is enqueued again into STA MAC queue

# Announced TWT and co-existence with PSM
## Announced TWT logic

### At AP

At AP: Variable m_twtAwakeForSp is used to keep track of two things: Whether Announced TWT STA is awake for next/ongoing TWT SP and generally whether STA is awake within the TWT SP at this instant.

- [x] If it is announced TWT, do not switch ```m_twtAwakeForSp``` automatically to 1 at beginning of every TWT SP
- [x] If PM bit is set to 0 in UL frame from TWT STA, do not deliver all unicast frames - already done.
    - [x] Only switch STA active status ```m_twtAwakeForSp``` at AP if it is announced TWT.
- [ ] For TWT STA at AP, if it is announced TWT and AP received a data frame with PM = 0, change ```m_twtAwakeForSp = 1```

### At STA

- [x] Create a new state variable ```m_wakeUpForNextTwt``` - used only for announced TWT operation
    - [x] By default set to 0 if announced; 1 if unannounced
    - [x] At end of every TWT SP, set to 0 if announced
    - [x] With every UL packet outside TWT SP, set to 1
- [x] If unannounced, wake up for every TWT SP 
    -[x] ```m_wakeUpForNextTwt``` is checked to be always 1
- [x] If announced, wake up only if ```m_wakeUpForNextTwt``` is 1


### Coexistence with PSM
 - [x] Set m_PSM to 1 at AP for STAs in TWT during initialization - at all times - never changes to 0
 - [x] Change PSM logic at AP - if TWT is active, do not change m_PSM state for STAs based on UL frames PM bit value
 - [x] At STA, set PM bit to 0 - by default if TWT agreement is present

## TWT suspend and resume through TWT IE

Plan:

0. Find and finalize frame format for TWT suspend and resume
1. Create TWT IE frame model
2. Figure out how to attach IE to frames
3. At AP, create a state variable for each STA - m_twtSuspended - set to 1 if suspended
        No downlink is sent if m_twtSuspended is 1
        If TWT resume/suspend is received from a TWT STA, change m_twtSuspended to 0/1
4. At STA, 
        Have a TWT config parameter - to suspend and resume as required
        If there is uplink, send TWT resume IE with uplink data
        Have a timer (=RTT) - with every uplink data, schedule an UL NDP with TWT Suspend IE
        Power: As soon as TWT Susp is ACKed, go to sleep.


## At STA

- [x] If TWT is TB and an UL frame is queued while within TWT SP, it is added to the PS queue.

- [x] Create a mode for Trigger based TWT. If flag is set to true, STA will not send UL packets while awake within TWT SP. They will be added to PS queue.
NOTE: If m_twtUplinkOutsideSp is true, STA will send UL frames outside the SP
- [ ] Bug - twtTriggerBased cannot be passed as an argument inside Simulator::Schedule - fix later. For now, this is set in sta-wifi-mac.h to false using default arguments
- [ ] Schedule a TF from AP at beginning of every SP
        See WifiPrimaryChannelsTest::SendHeTbPpdu