# This code is for batch processing ns3 trace and PHY state logs for PSM and TWT with uplink TCP/UDP traffic
# Author : Shyam Krishnan Venkateswaran
# Email: shyam1@gatech.edu
# Variables to take care of before each run will be tagged with #changeThisBeforeRun

# Reference Links for calculating IFS based on frame type: https://mrncciew.com/2014/10/12/cwap-802-11-medium-contention/
    # https://www.cwnp.com/uploads/802-11_arbitration.pdf
    # https://mrncciew.files.wordpress.com/2014/10/802-11-qos-overview.pdf 

    # With high enough traffic, there are a considerable number of retransmissions. Check pcap files. How to deal with retries?
    
    # Frame type                            IFS                Comment
    # Beacon (MGT_BEACON)               SIFS + 1 slot       See ApWifiMac::ApWifiMac ()
    # BE QoS data (QOSDATA              SIFS + 2 slot       See WifiMac::ConfigureDcf 
    # non QoS data                      SIFS + 2 slot       See WifiMac::ConfigureDcf 
    # CTL_ACK                           SIFS
    # PS-POLL (CTL_PSPOLL)              SIFS + 2 slot       Same as DIFS - but will be different if AIFSN is different
    # management assoc/resp             SIFS + 2 slot       EXCLUDE BEACONS (MGT_ASSOCIATION_REQUEST, MGT_ASSOCIATION_RESPONSE)
    # Mgt BA request (CTL_BACKREQ )     SIFS + 2 slot       See https://mrncciew.com/2014/10/12/cwap-802-11-medium-contention/
    # Mgt BA response (CTL_BACKRESP )   SIFS                Denotes Block ACK       
    # AMPDU (AmpduSubframeHeader)       SIFS + 2 slot       Can be handled like other packets

    # Dealing with collisions
    #   In txlog, keep track of last tx end timestamp (prevTxEndTimeStamp). If next tx start timestamps are before prevTxEndTimeStamp, there is a collision. 
    #   Just update prevTxEndTimeStamp to latest end of Tx. 
    #   Add only the extra Tx time to channel busy time
    #   If two Tx start at same time, the longer Tx is used for calculation

    # Dealing with retries
    #   Treat as a normal frame


# %%

import numpy as np
import matplotlib.pyplot as plt
import csv
from matplotlib.pyplot import figure
import json
import random
import glob     # for parsing directories
import os       # for clearing terminal in windows OS
import pandas as pd
from pathlib import Path       # to handle directories across OSs
from random import randrange



# %%

# File management


# naming convention for log files from ns3
# stateLogs for PSM STA must be named as 'log######.statelog', where ###### are 6 numbers 0-9
# Ascii traces  must be named as 'log######.asciitr', where ###### are 6 numbers 0-9
# Tx traces (have only Tx PHY trace for all WiFi nodes) must be named as 'log######.txlog', where ###### are 6 numbers 0-9
# The '#####' will be used as IDs to find corresponding log - trace pairs. 

# Output CSV file name


csvFileName = 'twtTest.csv'          #changeThisBeforeRun
# For PSM
# StaNodeDeviceStringASCII = "NodeList/1/DeviceList/0"           # note: Make sure this is the STA node and interface of interest from ns3. Other network events will be ignored. #changeThisBeforeRun
# ApNodeDeviceStringASCII = "NodeList/0/DeviceList/1"           # note: Make sure this is the AP node and interface of interest from ns3. Other network events will be ignored. #changeThisBeforeRun

# For TWT
StaNodeDeviceStringASCII = "NodeList/1/DeviceList/0"           # note: Make sure this is the STA node and interface of interest from ns3. Other network events will be ignored. #changeThisBeforeRun
ApNodeDeviceStringASCII = "NodeList/0/DeviceList/0"           # note: Make sure this is the AP node and interface of interest from ns3. Other network events will be ignored. #changeThisBeforeRun

enableAlwaysOn = False
useCase = 'TCP'                         #changeThisBeforeRun

# Following lists will be appended at the end of each iteration. The lists are ordered and correspond to each other in the correct order.
index_list = [] 
totalDuration_us_list = []
channelBusyTime_us_list = []
channelBusyRatio_us_list = []
averageCurrent_mA_list = []
stateTransitions_list = []
bufferCurrent_list = []
awakeTime_us_list = []
txTime_us_list = []
rxTime_us_list = []     # Any time awake that is not in Tx is considered Rx
collisionCount_list = []
successTcpULCount_list = []
ULAckDelay_us_avg_list = []

data_folder = Path("twtLogs/")    #changeThisBeforeRun                                         
# data_folder = Path("MU_logs/")    

ns3_state_log_list_temp = list(data_folder.glob("**/*.statelog"))
ns3_state_log_list = [str(x) for x in ns3_state_log_list_temp]

ns3_txstate_log_list_temp = list(data_folder.glob("**/*.txlog"))
ns3_txstate_log_list = [str(x) for x in ns3_txstate_log_list_temp]

ns3_ascii_log_list_temp = list(data_folder.glob("**/*.asciitr"))
ns3_ascii_log_list = [str(x) for x in ns3_ascii_log_list_temp]

ns3_macTx_log_list_temp = list(data_folder.glob("**/*.macTx"))
ns3_macTx_log_list = [str(x) for x in ns3_macTx_log_list_temp]

ns3_ackedMpdu_log_list_temp = list(data_folder.glob("**/*.ackedMpdu"))
ns3_ackedMpdu_log_list = [str(x) for x in ns3_ackedMpdu_log_list_temp]

ns3_state_log_list.sort()
ns3_txstate_log_list.sort()
ns3_ascii_log_list.sort()
ns3_macTx_log_list.sort()
ns3_ackedMpdu_log_list.sort()

logCount = len(ns3_state_log_list)
print (f"Total log count: {logCount}")

# For random log selection ------------------------------below - just delete this block for batch processing
# chosenLog = randrange(logCount)
# chosenLog = 49
# print(f"Chosen log: {chosenLog}")

# ns3_state_log_list = [ns3_state_log_list[chosenLog]]
# ns3_ascii_log_list = [ns3_ascii_log_list[chosenLog]]
# ns3_txstate_log_list = [ns3_txstate_log_list[chosenLog]]

# print(f"{ns3_state_log_list}, {ns3_ascii_log_list}, {ns3_txstate_log_list}")
# logCount = 1
# print(f"Chosen log: {ns3_state_log_list}")

# For random log selection ------------------------------Above


print(f"Number of logs to be processed: {logCount}")
# -----------------------------------------------------------
# -----------------------------------------------------------


# %%
# All configuration parameters, numerical values and adjustments are defined here. \

stateDict = {}                  # This is the master dict storing state names and current values in mA
durationDict = {}               # This dictionary contains the durations for manually added states in us (micro seconds)     
# milliSecToPlot = 1024*4    # This sets how much data will be plotted
# timeUnits = milliSecToPlot*1000  

stateLogStartTime_us = 4 * 1e6          # PHY state log will be imported only after this time from beginning
stateLogEndTime_us = 5.5 * 1e6          # PHY state log will be imported only till this time
# stateLogEndTime_us = 100 * 1e6          # PHY state log will be imported only till this time



enableStateTransitions = True         # If enabled, state transitions will be added
enableBufferCurrent = False             # If enabled, different (higher) current will be used for SLEEP states with unACK'ed TCP packets, else - same as SLEEP



# State Dictionary definition - Current values are in Amperes

stateDict["SLEEP"] =  0.12e-3                                        # IDLE state - will be assigned LPDS current after processing
# stateDict["ON"] =  66e-3                                            # Used for Always on power modes - value from experiments
stateDict["ON"] =  50e-3                                            # Used for Always on power modes - value from experiments
stateDict["RX"] = 50e-3    
stateDict["IDLE"] =  stateDict["RX"]                                    # IDLE state - will be assigned Rx current after processing
stateDict["SLEEP_BUFFER"] =  10e-3                                  # Sleep state while waiting for TCP ACK 
# stateDict["BCN_RX"] =  45e-3                                        # For Beacon Rx - from datasheet
stateDict["BCN_RX"] =  stateDict["RX"]                                        # For Beacon Rx - changed for ICC paper
stateDict["TX"] = 232e-3                                            # This will be referenced in multiple Tx states but not used directly
stateDict["RX_FULL"] = stateDict["RX"]                                        # This will be referenced in multiple Rx states but not used directly
                                        


stateDict["ACK_802_11_RX"] = stateDict["RX_FULL"]                       # Wi-Fi ACK Rx current
stateDict["ACK_802_11_TX"] =  stateDict["TX"]                           # Set to Tx current
stateDict["TCP_TX"] =  stateDict["TX"]                                  # Set to Tx current
stateDict["UDP_TX"] =  stateDict["TX"]                                  # Set to Tx current
stateDict["TCP_ACK_RX"] = stateDict["RX_FULL"] 
stateDict["PSPOLL_TX"] =  stateDict["TX"]                               # Set to Tx current


stateDict["BCN_RAMPUP"] =  4.5e-3                                   # Ramp up to beacon Rx from LPDS
stateDict["BCN_RAMPDOWN"] =  12.5e-3                                # Ramp down from Beacon Rx to LPDS
stateDict["BCN_RAMPUP_BUFFER"] =  32e-3                              # Ramp up to Beacon Rx from Sleep (10 mA)
stateDict["BCN_RAMPDOWN_BUFFER"] =  12.5e-3                          # Ramp down from Beacon Rx to Sleep (10 mA)


stateDict["TCP_TX_RAMPUP"] =  25e-3  
stateDict["TCP_TX_RAMPDOWN"] =  36e-3                               # This will be added after 802.11 ACK Rx
stateDict["TCP_TX_RAMPUP_BUFFER"] =  36e-3                               # Not used yet - when STA wakes up to send another TCP packet with unACK'ed packets already
stateDict["TCP_TX_RAMPDOWN_BUFFER"] =  36e-3                               # This will be added after 802.11 ACK Rx when buffer takes extra current

# stateDict["ACK_802_11_RX_RAMPDOWN"] = 28e-3 
stateDict["ACK_802_11_RX_RAMPDOWN"] = stateDict["BCN_RAMPDOWN"] 
stateDict["ACK_802_11_RX_RAMPDOWN_BUFFER"] = 36e-3 

# stateDict["ACK_802_11_TX_RAMPDOWN"] = 65e-3     # old value used for PSM eval
stateDict["ACK_802_11_TX_RAMPDOWN"] = 28e-3 

stateDict["TCP_RAMPUP_SLEEP_TO_TCP_ACK_RX"] =  20e-3  
    
stateDict["UDP_TX_RAMPUP"] =  28e-3  


# Durations ------------------------- in micro seconds

durationDict["BCN_rampUp"]                  = 2600                                              # From LPDS
durationDict["BCN_rampDown"]                = 800                                               # From LPDS
durationDict["BCN_rampUp_fromSleepBuffer"]        = durationDict["BCN_rampUp"]
durationDict["BCN_rampDown_toSleepBuffer"]        = durationDict["BCN_rampDown"]

durationDict["TCP_TX_rampUp"]               = 23500
durationDict["TCP_TX_rampDown"]             = 5500                                              
durationDict["TCP_SLEEP_TO_TCP_ACK_RX_rampUp"] = 5000

durationDict["UDP_TX_rampUp"]               = 23000


# durationDict["ACK_802_11_RX_rampDown"] = 5500 
durationDict["ACK_802_11_RX_rampDown"] = durationDict["BCN_rampDown"] 
# durationDict["ACK_802_11_TX_rampDown"] = 1300 # old value used for PSM eval
durationDict["ACK_802_11_TX_rampDown"] = 600 
# durationDict["ACK_802_11_RX_rampDown_toSleepBuffer"] = 5500 







## Numbers for Channel busy time and ratio evaluation

IfsDuration_us_Dict = dict()                # microseconds
IfsDuration_us_Dict["DIFS"] = 50        # microseconds
IfsDuration_us_Dict["SIFS"] = 10        # microseconds
IfsDuration_us_Dict["PIFS"] = 30        # microseconds




# %%
# Function definition for state comparison and priority assignment

def stateCompare(state1, state2):
    """
    Parameters
    ----------
    state1 : Name of first state as a string. 
    state2 : Name of first state as a string.
    Returns
    -------
    Returns boolean = current (state1) > current (state2)
    That is, True if current consumption of state1 is higher. 
    If stateCompare (state1, state2) returns True, state2 can be replaced with state1 IF NECESSARY.

    """
    
    return stateDict[state1] > stateDict[state2]
    
# %%
# ----------------------------------------------------------------------------------------------------10380
# Master Loop starts here


for logFilesCounter in range(logCount):
    
    # logFilesCounter = 0
    channelBusyTime_us_this = 0
    awakeTime_us_this = 0
    txTime_us_this = 0
    rxTime_us_this = 0
    successTcpULCount_this = 0
    ULAckDelay_us_list_this = []
    ULAckDelay_us_avg_this = 0

    ns3_state_log = ns3_state_log_list[logFilesCounter]
    print(f"Chosen state log: {ns3_state_log}")
    ns3_ascii_log = ns3_ascii_log_list[logFilesCounter]
    ns3_txstate_log = ns3_txstate_log_list[logFilesCounter]
    ns3_macTx_log = ns3_macTx_log_list[logFilesCounter]
    ns3_ackedMpdu_log = ns3_ackedMpdu_log_list[logFilesCounter]

    # Checking if log and trace files have the same number
    print(f"ns3_state_log:{ns3_state_log}, ns3_ascii_log: {ns3_ascii_log}, ns3_txstate_log:{ns3_txstate_log}, ns3_macTx_log:{ns3_macTx_log},  ns3_ackedMpdu_log:{ns3_ackedMpdu_log}")
    if (ns3_state_log.split('.')[0][-6:] != ns3_ascii_log.split('.')[0][-6:] ):
        print(f"State Log and trace file numbers do not match at number = {logFilesCounter}. CHECK!!")
    if (ns3_state_log.split('.')[0][-6:] != ns3_txstate_log.split('.')[0][-6:] ):
        print(f"State Log and Tx log file numbers do not match at number = {logFilesCounter}. CHECK!!")
    index_list.append(ns3_state_log.split('.')[0][-6:])
    print(f"\nProcessing log file ({logFilesCounter+1}/{logCount})")
    
    

    stateVector = []
    durationVector = []
    timeStampVector = []
    with open(ns3_state_log) as tsv:
        #for line in csv.reader(tsv, dialect="excel-tab"):
        prevState = 'None'
        for line in csv.reader(tsv, delimiter = ";"):
            # print(line)
            currentState = line[0].split("=")[1]

            currentTimeStamp = int(line[1].split("=")[1])
            # currentTimeStamp = np.ceil( currentTimeStamp/1000)
            
            
            currentDuration = int(line[2].split("=")[1])
            # currentDuration = np.ceil( currentDuration/1000)
            #Adding before rounding.
            currentTimeStamp = currentTimeStamp + currentDuration   

            currentDuration = int( currentDuration/1000)
            currentTimeStamp = int( currentTimeStamp/1000)


            # Note : currentTimeStamp is the time when the state completes. Not when it begins.

            
            if (currentTimeStamp < stateLogStartTime_us ) or (currentTimeStamp > stateLogEndTime_us ) :
                continue
            
            if currentState == "CCA_BUSY" and prevState == "SLEEP":
                # Bug with ns3 state logging system - Every SLEEP state is followed by a duplicate CCA_BUSY state - should be ignored
                # print (f"Found CCA_BUSY after SLEEP at time = {currentTimeStamp}; Skipping.")
                continue
            
            if currentState == "CCA_BUSY":
                currentState = "IDLE"

            if currentState == "SLEEP" and prevState == "IDLE":
                stateVector[-1] = "SLEEP"
                prevState = "SLEEP"

            if prevState == currentState:
                # Consequtive states that are the same - consolidating
                durationVector[-1] = int(durationVector[-1] + currentDuration)
                timeStampVector[-1] = int(timeStampVector[-1] + currentDuration)
                # print (f"Added {currentDuration} to previous state")
                continue

            prevState = currentState
            

            # #print(currentTimeStamp)
            # currentState = line[4]
            # currentState = currentState.split("=")[1]
            # currentDuration = line[-1]
            
            # currentDuration = currentDuration.split("+",1)[1]
            # #print(currentDuration)
            # currentDuration = float(currentDuration.split("ns")[0])
            # #print(currentDuration)
            # #******************************************* approximating to precision of 1 microseconds
            # currentDuration = int(currentDuration/1000)
            # currentDuration = int(np.ceil(currentDuration))
            # #print(f"{currentState}, {currentDuration}")
            # print (f"currentState={currentState}, StateStartedAt={currentTimeStamp-currentDuration}, currentTimeStamp={currentTimeStamp}, currentDuration={currentDuration}")
            if (currentDuration >0.0):
                stateVector.append(currentState)
                durationVector.append(int(currentDuration))
                timeStampVector.append(int(currentTimeStamp))
                if currentState == 'RX' or currentState == 'TX' or currentState == 'IDLE' :
                    awakeTime_us_this = awakeTime_us_this + currentDuration
                if currentState == 'TX':
                    txTime_us_this = txTime_us_this + currentDuration
                if currentState == 'RX' or currentState == 'IDLE' :
                    rxTime_us_this = rxTime_us_this + currentDuration
                    
            
    # print(timeStampVector)
    # Importing partial ASCII trace as a dictionary
    ascii_ns3_traceDict = dict()
    full_channelBusy_us_Dict = dict()
    with open(ns3_ascii_log) as file1:
        Lines = file1.readlines() 
        for line in Lines: 
            if (StaNodeDeviceStringASCII not in line ) and (ApNodeDeviceStringASCII not in line):
                # print (f"Node of interest not found in Line: {line}; Skipping this line")
                continue
            timeStamp_string = line.split(" ")[1]
            try:
                int (timeStamp_string)
            except ValueError:
                # means it is not an int string - is a hex string - some bug with ns3
                timeStamp_string = int(timeStamp_string, 16)
            timeStamp = int (int(timeStamp_string) / 1000)
            if (timeStamp > stateLogEndTime_us):
                break
            ascii_ns3_traceDict[timeStamp]= line
            #print(timeStamp)


    # print(json.dumps(ascii_ns3_traceDict, indent=1))   


    # %%
    # Calculating average wait time for successful uplink TCP packets to get WiFi ACKed
    # 
    # Importing and processing ns3_macTx_log - this contains all packets added to the MAC queue from upper layers. MAC layer packets will not be found in this
    macTx_dict = {}     # key is digest of packet header. value is timestamp
    with open(ns3_macTx_log) as file1:
        Lines = file1.readlines() 
        for line in Lines: 
            # print(line)
            if ("ns3::TcpHeader" not in line) and ("ns3::UdpHeader" not in line):
                continue
            timeTemp = line.split(";")[0]
            currentTimeStamp_us = int( int(timeTemp.split("=")[1]) / 1000)
            
            if useCase == 'TCP':
                currentPacketId = line.split("ns3::TcpHeader ")[1]
            elif useCase == 'UDP':
                currentPacketId = line.split("ns3::UdpHeader ")[1]
            currentPacketId = currentPacketId.rstrip()
            # print(f"currentTimeStamp_us:{currentTimeStamp_us}; currentPacketId={currentPacketId}" )
            if (currentTimeStamp_us > stateLogStartTime_us) and (currentTimeStamp_us < stateLogEndTime_us) :
                if currentPacketId in macTx_dict.keys():
                    print (f"{currentPacketId} was already in macTx_dict with value {macTx_dict[currentPacketId]}")
                macTx_dict[currentPacketId] = currentTimeStamp_us
    
    # print (macTx_dict)

    # Importing and processing ns3_ackedMpdu_log - this contains all acked mac packets
    # ackedMpdu_dict = {}     # key is digest of packet header. value is timestamp
    with open(ns3_ackedMpdu_log) as file1:
        Lines = file1.readlines() 
        for line in Lines: 
            # print(line)
            if ("ns3::TcpHeader" not in line) and ("ns3::UdpHeader" not in line):
                continue
            timeTemp = line.split(";")[0]
            currentTimeStamp_us = int( int(timeTemp.split("=")[1]) / 1000)
            
            if useCase == 'TCP':
                currentPacketId = line.split("ns3::TcpHeader ")[1]
            elif useCase == 'UDP':
                currentPacketId = line.split("ns3::UdpHeader ")[1]
                # print(f"currentPacketId:{currentPacketId}")
            currentPacketId = currentPacketId.rstrip()
            # print(f"currentTimeStamp_us:{currentTimeStamp_us}; currentPacketId={currentPacketId}")

            if (currentTimeStamp_us > stateLogStartTime_us) and (currentTimeStamp_us < stateLogEndTime_us) :
                if currentPacketId in macTx_dict.keys():
                    # print (f"{currentPacketId} found in macTx_dict with value {macTx_dict[currentPacketId]}")
                    tempDelay_us = currentTimeStamp_us - macTx_dict[currentPacketId]
                    # print(f"Calculated delay in us to ACK = {tempDelay_us}")
                    if tempDelay_us < 0:
                        print (f"tempDelay_us was negative. Skipping this. currentTimeStamp_us = {currentTimeStamp_us}")
                        continue
                    ULAckDelay_us_list_this.append(tempDelay_us)
                    successTcpULCount_this +=1
                # macTx_dict[currentPacketId] = currentTimeStamp_us
    
    print(f"ULAckDelay_us_list_this: {ULAckDelay_us_list_this}")
    print (f"successTcpULCount_this: {successTcpULCount_this}")
    successTcpULCount_list.append(successTcpULCount_this)
    ULAckDelay_us_avg_this = np.average(ULAckDelay_us_list_this)
    print(f"ULAckDelay_us_avg_this:{ULAckDelay_us_avg_this}")
    ULAckDelay_us_avg_list.append(ULAckDelay_us_avg_this)


    # %%

    # Channel Busy time and ratio calculation

    # Importing and parsing Tx only combined State log as a dict
    # Note - if there are multiple Tx states starting at same time, the longest tx duration will be taken
    full_tx_only_duration_us_Dict = dict()          # Dict keys are in ns; Dict values are is us
    with open(ns3_txstate_log) as file1:
        Lines = file1.readlines() 
        for line in Lines: 
            timeStamp_ns = line.split(";")[0]
            timeStamp_ns = int(timeStamp_ns.split("=")[1])
            if (timeStamp_ns > stateLogEndTime_us*1000):
                break
            if (timeStamp_ns < stateLogStartTime_us * 1000 ) or (timeStamp_ns > stateLogEndTime_us*1000 ) :
                continue
            txDuration_ns = line.split(";")[1]
            txDuration_us = int(int(txDuration_ns.split("=")[1])/1000)
            if timeStamp_ns in full_tx_only_duration_us_Dict.keys():
                if (txDuration_us < full_tx_only_duration_us_Dict[timeStamp_ns] ):
                    txDuration_us = full_tx_only_duration_us_Dict[timeStamp_ns]
                # print(f"Existing duration = {full_tx_only_duration_us_Dict[timeStamp_ns]}")
                # print(f"New duration = {txDuration_us}")
                print(f"TxWarning; Duplicate Tx state found in tx state log at timeStamp_ns: {timeStamp_ns}. Replacing with longest Tx duration")
            full_tx_only_duration_us_Dict[timeStamp_ns] = txDuration_us

            
            

    




    # Importing FULL Tx only ASCII trace as a dictionary
    full_channelBusy_us_Dict = dict()
    prevTxEndTimeStamp_ns = 0       # To detect collisions
    lastAddedTimeStamp_ns = 0
    lastAddedDuration_us = 0
    totalCollisions = 0
    with open(ns3_ascii_log) as file1:
        Lines = file1.readlines() 
        for line in Lines: 
            # if (StaNodeDeviceStringASCII not in line ) and (ApNodeDeviceStringASCII not in line):
            #     # print (f"Node of interest not found in Line: {line}; Skipping this line")
            #     continue
            if ( line.split(" ")[0] == 't' ):
                timeStamp_string = line.split(" ")[1]
                try:
                    int (timeStamp_string)
                except ValueError:
                    # means it is not an int string - is a hex string - some bug with ns3
                    timeStamp_string = int(timeStamp_string, 16)
                timeStamp_ns = int(timeStamp_string)
                if (timeStamp_ns > stateLogEndTime_us*1000):
                    break
                if (timeStamp_ns < stateLogStartTime_us * 1000 ) or (timeStamp_ns > stateLogEndTime_us*1000 ) :
                    continue
                tempDuration_us = full_tx_only_duration_us_Dict[timeStamp_ns]
                if (prevTxEndTimeStamp_ns < timeStamp_ns):
                    # No collision
                    prevTxEndTimeStamp_ns = timeStamp_ns + tempDuration_us*1000
                    if ("CTL_ACK" in line or "CTL_BACKRESP" in line):
                        full_channelBusy_us_Dict[timeStamp_ns] = tempDuration_us + IfsDuration_us_Dict["SIFS"]
                    elif ("MGT_BEACON" in line):
                        full_channelBusy_us_Dict[timeStamp_ns] = tempDuration_us + IfsDuration_us_Dict["PIFS"]
                    else:
                        full_channelBusy_us_Dict[timeStamp_ns] = tempDuration_us + IfsDuration_us_Dict["DIFS"]
                    lastAddedTimeStamp_ns = timeStamp_ns
                    lastAddedDuration_us = tempDuration_us
                else:
                    totalCollisions += 1
                    # print (f"Collision detected at time:{timeStamp_ns}; Collision count = {totalCollisions}")
                    extraTxDuration_us = int(timeStamp_ns/1000) + tempDuration_us - int(lastAddedTimeStamp_ns/1000 + lastAddedDuration_us)
                    if extraTxDuration_us < 0:      # If the colliding Tx ends before ongoing Tx
                        extraTxDuration_us = 0
                    # print(f"extraTxDuration_us:{extraTxDuration_us}")
                    full_channelBusy_us_Dict[lastAddedTimeStamp_ns] = full_channelBusy_us_Dict[lastAddedTimeStamp_ns] + extraTxDuration_us
                    prevTxEndTimeStamp_ns = prevTxEndTimeStamp_ns + extraTxDuration_us*1000
                    lastAddedDuration_us = lastAddedDuration_us + extraTxDuration_us
    

            
    
    # print(json.dumps(full_tx_only_duration_us_Dict, indent=1))      
    # print(json.dumps(full_channelBusy_us_Dict, indent=1))      
    collisionCount_this = totalCollisions
    channelBusyTime_us_this = sum(full_channelBusy_us_Dict.values())
    print(f"channelBusyTime_us_this: {channelBusyTime_us_this}")
    

    # %%
    # Tagging the log File. This is before current values are assigned
    # CCA_BUSY is changed to IDLE
    # If always ON mode is enabled, IDLE is replaced with ON
    # All adjacent but same states are combined
    # TCP states are identified and assigned


    stateVectorLength = len(stateVector)
    timeStampVectorModified = []
    stateVectorModified = []
    durationVectorModified = []
    for ii in range(stateVectorLength):
        
        # # Replacing all CCA_BUSY with IDLE
        # if stateVector[ii]== 'CCA_BUSY':
        #     stateVector[ii] = 'IDLE'
        if stateVector[ii]== 'IDLE' and enableAlwaysOn:
            stateVector[ii] = 'ON'
        # Finding BCN_RX and 802.11 ACK receptions
        # print(f"timeStampVector[ii] = {timeStampVector[ii]}")
        if timeStampVector[ii] in ascii_ns3_traceDict:

            tempASCIItrace = ascii_ns3_traceDict[timeStampVector[ii]]
            # print(f"At time {timeStampVector[ii]}, ASCII trace is \n {tempASCIItrace}")
            if stateVector[ii]== 'RX' and ("RxOk" in tempASCIItrace) and (tempASCIItrace.split(" ")[0]=="r") and ("MGT_BEACON" in tempASCIItrace):  
                stateVector[ii] = "BCN_RX"
                # print (f"BCN RX found. Prev state is: {stateVector[ii - 1]}")
                # If previous state is IDLE, change it to SLEEP so that it will be combined with preceding SLEEP state. Else rampup cannot be added.
                if (ii > 0) and stateVector[ii - 1]== "IDLE":
                    # print (f"Changing prev IDLE to SLEEP")
                    stateVectorModified[-1] = "SLEEP"

            elif stateVector[ii]== 'TX' and (ApNodeDeviceStringASCII in tempASCIItrace) and ("TcpHeader" in tempASCIItrace) and ("RxOk" in tempASCIItrace):
                stateVector[ii] = 'TCP_TX'
                # print (tempASCIItrace)

            elif stateVector[ii]== 'TX' and (ApNodeDeviceStringASCII in tempASCIItrace) and ("UdpHeader" in tempASCIItrace) and ("RxOk" in tempASCIItrace):
                stateVector[ii] = 'UDP_TX'

            elif stateVector[ii]== 'TX' and (ApNodeDeviceStringASCII in tempASCIItrace) and ("CTL_PSPOLL" in tempASCIItrace) and ("RxOk" in tempASCIItrace):
                stateVector[ii] = 'PSPOLL_TX'


            elif stateVector[ii]== 'TX' and (ApNodeDeviceStringASCII in tempASCIItrace) and (("CTL_ACK" in tempASCIItrace) or  ("CTL_BACKRESP" in tempASCIItrace) ) and ("RxOk" in tempASCIItrace):
                stateVector[ii] = 'ACK_802_11_TX'
            
            
            elif stateVector[ii]== 'RX' and (("CTL_ACK" in tempASCIItrace) or ("CTL_BACKRESP" in tempASCIItrace)) and (tempASCIItrace.split(" ")[0]=="r") and ("RxOk" in tempASCIItrace):
                stateVector[ii] = 'ACK_802_11_RX'
                 

                #print(f"TCP Tx found at t = {timeStampVector[ii]}")
            elif stateVector[ii]== 'RX' and ("TcpHeader" in tempASCIItrace) and (tempASCIItrace.split(" ")[0]=="r") and ("RxOk" in tempASCIItrace) and ("[ACK]" in tempASCIItrace):
                stateVector[ii] = 'TCP_ACK_RX'
                #print(f"TCP ACK found at t = {timeStampVector[ii]}")
            elif stateVector[ii] != 'IDLE':
                pass
                # print(f"State not tagged at ii: {ii}: {stateVector[ii]}")
                # print(f"tempASCIItrace:{tempASCIItrace}")
        else:
            pass
            # print (f"timeStampVector[ii]= {timeStampVector[ii]} not found in ASCII trace. State is {stateVector[ii]}")



        # To combine adjacent same states  
        # if ii == 0:
        #     #prevState = stateVector[ii]
        stateVectorModified.append(stateVector[ii])
        durationVectorModified.append(durationVector[ii])
        timeStampVectorModified.append(timeStampVector[ii])
        # elif (stateVector[ii] == stateVectorModified[-1]):
        #     durationVectorModified[-1] = durationVectorModified[-1] + durationVector[ii]
        # else:
        #     stateVectorModified.append(stateVector[ii])
        #     durationVectorModified.append(durationVector[ii])
        #     timeStampVectorModified.append(timeStampVector[ii])
    durationVectorModified = [int(i) for i in durationVectorModified]

    # print (stateVectorModified)

    # totalDuration_this = np.sum (durationVectorModified)
    # print(f"Total duration for calculation = {totalDuration_this}")
    

    # %%
    # Combining same adjacent states

    # print (f"Before state consolidation: ")
    # print (f"stateVectorModified:{stateVectorModified}")
    # print (f"durationVectorModified:{durationVectorModified}")
    # print (f"timeStampVectorModified:{timeStampVectorModified}")

    stateVectorLength = len(stateVectorModified)
    timeStampVectorTemp = timeStampVectorModified
    stateVectorTemp = stateVectorModified 
    durationVectorTemp = durationVectorModified

    timeStampVectorModified = []
    stateVectorModified = []
    durationVectorModified = []

    for ii in range(stateVectorLength):
        # print (f"stateVectorTemp[ii]:{stateVectorTemp[ii]};stateVectorModified:{stateVectorModified}")
        if ii == 0:
            stateVectorModified.append(stateVectorTemp[ii])
            durationVectorModified.append(durationVectorTemp[ii])
            timeStampVectorModified.append(timeStampVectorTemp[ii])
        elif stateVectorTemp[ii] == stateVectorModified[-1]:
            durationVectorModified[-1] += durationVectorTemp[ii]
            timeStampVectorModified[-1] += timeStampVectorTemp[ii]
        else:
            stateVectorModified.append(stateVectorTemp[ii])
            durationVectorModified.append(durationVectorTemp[ii])
            timeStampVectorModified.append(timeStampVectorTemp[ii])

    # print (f"After state consolidation: ")
    # print (f"stateVectorModified:{stateVectorModified}")
    # print (f"durationVectorModified:{durationVectorModified}")
    # print (f"timeStampVectorModified:{timeStampVectorModified}")
    


            


    # %%

    # handling 'ON' state for PS-POLL and multicast downlink mechanisms
    # Logic: Look for all beacons -> until a SLEEP state is found, change all IDLE to ON. Leave other states as they are.
    stateVectorTemp = stateVectorModified
    durationVectorTemp = durationVectorModified
    timeStampVectorTemp = timeStampVectorModified

    stateVectorLength = len(stateVectorModified)

    for ii in range(stateVectorLength):
        if (stateVectorModified[ii] == 'BCN_RX'):
            # Look for next sleep state
            for counter in range(20):
                if (ii + counter) >= stateVectorLength:
                    break
                if stateVectorModified[ii + counter] == 'SLEEP':
                    break
                elif stateVectorModified[ii + counter] == 'IDLE':
                    stateVectorModified[ii + counter] = 'ON'

    # %%
    # Adding Beacon ramp up and down
    # For BCN_RX states, adding Ramp up and Ramp down if IDLE state is available

    # print(f"Before beacon rampUP down: Total duration: {np.sum(durationVectorModified)}")
    if (enableStateTransitions):
        stateVectorRampUpDown = []
        durationVectorRampUpDown = []
        timeStampVectorRampUpDown = []
        addedAlreadyLookAhead = False

        stateVectorLength = len(stateVectorModified)
        for ii in range(stateVectorLength):
            if addedAlreadyLookAhead:
                addedAlreadyLookAhead = False
                continue
            if ii==0 or ii == stateVectorLength-1 and not(addedAlreadyLookAhead) :
                stateVectorRampUpDown.append(stateVectorModified[ii])
                durationVectorRampUpDown.append(durationVectorModified[ii])
                timeStampVectorRampUpDown.append(timeStampVectorModified[ii])
                continue
            if (stateVectorModified[ii] == 'BCN_RX'):
                # The state is BCN_RX. Look to add BCN_RAMPUP
                # print (f"stateVectorRampUpDown[-1] ={stateVectorRampUpDown[-1]}; durationVectorRampUpDown[-1] = {durationVectorRampUpDown[-1]}; Ramp up required = {durationDict['BCN_rampUp']}")
                isPreviousStateIDLEorSLEEP = (stateVectorRampUpDown[-1] == 'IDLE') or (stateVectorRampUpDown[-1] == 'SLEEP')
                if (  isPreviousStateIDLEorSLEEP and durationVectorRampUpDown[-1] >= durationDict["BCN_rampUp"]):
                    durationVectorRampUpDown[-1] = durationVectorRampUpDown[-1] - durationDict["BCN_rampUp"]
                    stateVectorRampUpDown.append('BCN_RAMPUP')
                    durationVectorRampUpDown.append(durationDict["BCN_rampUp"])
                    timeStampVectorRampUpDown.append(timeStampVectorModified[ii] - durationDict["BCN_rampUp"])
                elif (isPreviousStateIDLEorSLEEP and durationVectorRampUpDown[-1] < durationDict["BCN_rampUp"]):
                    # insufficient IDLE state. Just  convert what is available
                    stateVectorRampUpDown[-1] = 'BCN_RAMPUP'
                    
                else:
                    pass
                    # print(f"Error at ii: {ii}. Could not add beacon RampUp")
                # Adding RX state
                stateVectorRampUpDown.append('BCN_RX')
                durationVectorRampUpDown.append(durationVectorModified[ii])
                timeStampVectorRampUpDown.append(timeStampVectorModified[ii])
                
                
                #Look to add BCN_RAMPDOWN
                isNextStateIDLEorSLEEP = (stateVectorModified[ii+1] == 'IDLE') or (stateVectorModified[ii+1] == 'SLEEP')
                if (isNextStateIDLEorSLEEP and durationVectorModified[ii+1] >= durationDict["BCN_rampDown"]):
                    addedAlreadyLookAhead = True
                    stateVectorRampUpDown.append('BCN_RAMPDOWN')
                    durationVectorRampUpDown.append(durationDict["BCN_rampDown"])
                    timeStampVectorRampUpDown.append(timeStampVectorModified[ii+1])
                    # Adding next idle time after deduction
                    stateVectorRampUpDown.append('SLEEP')           #shyam - note
                    durationVectorRampUpDown.append(durationVectorModified[ii+1] - durationDict["BCN_rampDown"])
                    timeStampVectorRampUpDown.append(timeStampVectorModified[ii+1]+ durationDict["BCN_rampDown"])
                elif (isNextStateIDLEorSLEEP and durationVectorModified[ii+1] < durationDict["BCN_rampDown"]):
                    stateVectorModified[ii+1] == 'BCN_RAMPDOWN'

        
        
            else:
                # Just some other state
                stateVectorRampUpDown.append(stateVectorModified[ii])
                durationVectorRampUpDown.append(durationVectorModified[ii])
                timeStampVectorRampUpDown.append(timeStampVectorModified[ii])

    else:
        stateVectorRampUpDown = stateVectorModified
        durationVectorRampUpDown = durationVectorModified
        timeStampVectorRampUpDown = timeStampVectorModified    
    

        
    # %%

    # If case is TCP and sleep buffer has different current, change sleep to sleep/idle buffer and consolidate states.


    # %%
    # For UDP TX case
    if useCase == 'UDP':
        stateVectorModified = stateVectorRampUpDown * 1
        durationVectorModified = durationVectorRampUpDown * 1
        timeStampVectorModified = timeStampVectorRampUpDown * 1
        stateVectorRampUpDown = []
        durationVectorRampUpDown = []
        timeStampVectorRampUpDown = []
        addedAlreadyLookAhead = False

        stateVectorLength = len(stateVectorModified)

        for ii in range(stateVectorLength):
            if addedAlreadyLookAhead:
                addedAlreadyLookAhead = False
                continue
            if (stateVectorModified[ii] == 'UDP_TX'):
                # Adding RampUp
                # if (stateVectorRampUpDown[-1] == 'IDLE' and durationVectorRampUpDown[-1] >= durationDict["TCP_TX_rampUp"]): 
                if (enableStateTransitions):
                    # print (f"Adding UDP rampup at ii: {ii}")
                    remainingRampUpDuration = durationDict["UDP_TX_rampUp"]
                    temp_stateVector = []
                    temp_durationVector = []
                    temp_timeStampVector = []
                    while (remainingRampUpDuration > 0):
                        if (durationVectorRampUpDown[-1] <= remainingRampUpDuration):
                            remainingRampUpDuration = remainingRampUpDuration - durationVectorRampUpDown[-1]
                            #print(stateVectorRampUpDown)
                            if stateCompare('UDP_TX_RAMPUP', stateVectorRampUpDown[-1]):
                                stateVectorRampUpDown[-1] = "UDP_TX_RAMPUP"
                            # Adding the previous state to temp vectors so that they can be popped
                            temp_stateVector.append(stateVectorRampUpDown[-1])
                            temp_durationVector.append(durationVectorRampUpDown[-1])
                            temp_timeStampVector.append(timeStampVectorRampUpDown[-1])
                            # Popping the last states in the vectors
                            stateVectorRampUpDown.pop()
                            durationVectorRampUpDown.pop()
                            timeStampVectorRampUpDown.pop()
                        else:
                            durationVectorRampUpDown[-1] = durationVectorRampUpDown[-1] - remainingRampUpDuration
                            timeStampVectorRampUpDown.append(timeStampVectorRampUpDown[-1] + durationVectorRampUpDown[-1])
                            stateVectorRampUpDown.append('UDP_TX_RAMPUP')
                            durationVectorRampUpDown.append(remainingRampUpDuration)
                            remainingRampUpDuration = 0     # so that loop ends
                            temp_stateVector.reverse()
                            temp_durationVector.reverse()
                            temp_timeStampVector.reverse()
                            
                            stateVectorRampUpDown.extend(temp_stateVector)
                            durationVectorRampUpDown.extend(temp_durationVector)
                            timeStampVectorRampUpDown.extend(temp_timeStampVector)
                    stateVectorRampUpDown.append('UDP_TX')
                    durationVectorRampUpDown.append(durationVectorModified[ii])
                    timeStampVectorRampUpDown.append(timeStampVectorModified[ii])                            
                    # Ramp Up has been added            
                            
                        

                # Until ACK_802_11_RX is found, set to ON if IDLE
                for tempCounter in range(5):
                    if (stateVectorModified[ii+tempCounter+1] == 'ACK_802_11_RX'):
                        break
                    elif (stateVectorModified[ii+tempCounter+1] == 'IDLE'):
                        stateVectorModified[ii+tempCounter+1] = 'ON'
                        # print (f"Changed IDLE to ON after TCP Tx")

            elif (stateVectorModified[ii] == 'ACK_802_11_RX'):  
                # This can be ACK for any packet - so should be handled as general case
                stateVectorRampUpDown.append('ACK_802_11_RX')
                durationVectorRampUpDown.append(durationVectorModified[ii])
                timeStampVectorRampUpDown.append(timeStampVectorModified[ii])

                if (enableStateTransitions and ii < (stateVectorLength-1)):
                    tempNextState = stateVectorModified[ii+1]
                    transitionState = 'None'
                    # print(f"next states: {stateVectorModified[ii+1], stateVectorModified[ii+2]}; Duration: {durationVectorModified[ii+1], durationVectorModified[ii+2]}")
                    if (tempNextState == 'SLEEP'):
                        transitionState = 'ACK_802_11_RX_RAMPDOWN'
                    else:
                        pass
                        # print(f"Did not add rampdown after ACK_802_11_RX - next state was {tempNextState} for {durationVectorModified[ii+1]}")
                    
                    
                    if (transitionState != 'None') and (durationVectorModified[ii+1] >= durationDict["ACK_802_11_RX_rampDown"]):
                        addedAlreadyLookAhead = True
                        stateVectorRampUpDown.append(transitionState)
                        durationVectorRampUpDown.append(durationDict["ACK_802_11_RX_rampDown"])
                        timeStampVectorRampUpDown.append(timeStampVectorModified[ii+1])
                        # Adding next idle time after deduction
                        stateVectorRampUpDown.append(tempNextState)
                        durationVectorRampUpDown.append(durationVectorModified[ii+1] - durationDict["ACK_802_11_RX_rampDown"])
                        timeStampVectorRampUpDown.append(timeStampVectorModified[ii+1]+ durationDict["ACK_802_11_RX_rampDown"])
                    elif (transitionState != 'None') and (durationVectorModified[ii+1] < durationDict["ACK_802_11_RX_rampDown"]):
                        stateVectorModified[ii+1] = transitionState
                    else:
                        pass
            
            else:
                # Just some other state
                stateVectorRampUpDown.append(stateVectorModified[ii])
                durationVectorRampUpDown.append(durationVectorModified[ii])
                timeStampVectorRampUpDown.append(timeStampVectorModified[ii])
    # print(stateVectorRampUpDown)


    # %%
    # For TCP_TX states, adding Ramp up, intermediate, and Ramp down if IDLE state is available. Sleep state beacon ramp ups and downs are also updated here
    if useCase == 'TCP':
        stateVectorModified = stateVectorRampUpDown * 1
        durationVectorModified = durationVectorRampUpDown * 1
        timeStampVectorModified = timeStampVectorRampUpDown * 1
        stateVectorRampUpDown = []
        durationVectorRampUpDown = []
        timeStampVectorRampUpDown = []
        addedAlreadyLookAhead = False

        stateVectorLength = len(stateVectorModified)
        for ii in range(stateVectorLength):
            if addedAlreadyLookAhead:
                addedAlreadyLookAhead = False
                continue
    #         if (ii==0 or ii == stateVectorLength-1) and not(addedAlreadyLookAhead) :
    #             stateVectorRampUpDown.append(stateVectorModified[ii])
    #             durationVectorRampUpDown.append(durationVectorModified[ii])
    #             timeStampVectorRampUpDown.append(timeStampVectorModified[ii])
    #             continue
        #   

            if (stateVectorModified[ii] == 'TCP_TX'):
                # Adding RampUp
                # if (stateVectorRampUpDown[-1] == 'IDLE' and durationVectorRampUpDown[-1] >= durationDict["TCP_TX_rampUp"]): 
                if (enableStateTransitions):

                    remainingRampUpDuration = durationDict["TCP_TX_rampUp"]
                    temp_stateVector = []
                    temp_durationVector = []
                    temp_timeStampVector = []
                    while (remainingRampUpDuration > 0):
                        if (durationVectorRampUpDown[-1] <= remainingRampUpDuration):
                            remainingRampUpDuration = remainingRampUpDuration - durationVectorRampUpDown[-1]
                            #print(stateVectorRampUpDown)
                            if stateCompare('TCP_TX_RAMPUP', stateVectorRampUpDown[-1]):
                                stateVectorRampUpDown[-1] = "TCP_TX_RAMPUP"
                            # Adding the previous state to temp vectors so that they can be popped
                            temp_stateVector.append(stateVectorRampUpDown[-1])
                            temp_durationVector.append(durationVectorRampUpDown[-1])
                            temp_timeStampVector.append(timeStampVectorRampUpDown[-1])
                            # Popping the last states in the vectors
                            stateVectorRampUpDown.pop()
                            durationVectorRampUpDown.pop()
                            timeStampVectorRampUpDown.pop()
                        else:
                            durationVectorRampUpDown[-1] = durationVectorRampUpDown[-1] - remainingRampUpDuration
                            timeStampVectorRampUpDown.append(timeStampVectorRampUpDown[-1] + durationVectorRampUpDown[-1])
                            stateVectorRampUpDown.append('TCP_TX_RAMPUP')
                            durationVectorRampUpDown.append(remainingRampUpDuration)
                            remainingRampUpDuration = 0     # so that loop ends
                            temp_stateVector.reverse()
                            temp_durationVector.reverse()
                            temp_timeStampVector.reverse()
                            
                            stateVectorRampUpDown.extend(temp_stateVector)
                            durationVectorRampUpDown.extend(temp_durationVector)
                            timeStampVectorRampUpDown.extend(temp_timeStampVector)
                    stateVectorRampUpDown.append('TCP_TX')
                    durationVectorRampUpDown.append(durationVectorModified[ii])
                    timeStampVectorRampUpDown.append(timeStampVectorModified[ii])                            
                    # Ramp Up has been added            
                            
                        

                # Until ACK_802_11_RX is found, set to ON if IDLE
                for tempCounter in range(5):
                    if (stateVectorModified[ii+tempCounter+1] == 'ACK_802_11_RX'):
                        break
                    elif (stateVectorModified[ii+tempCounter+1] == 'IDLE'):
                        stateVectorModified[ii+tempCounter+1] = 'ON'
                        # print (f"Changed IDLE to ON after TCP Tx")
                
                # following is required if Buffer current is extra
                if  (enableBufferCurrent):
                    for tempCounter in range(20):  # Look ahead and change all (20) the following IDLE states to SLEEP_BUFFER till TCP_ACK is received. This is required in case of PSM
                        if (ii + tempCounter >= (stateVectorLength - 1)):
                            break
                        if (stateVectorModified[ii+tempCounter]) == 'TCP_ACK_RX':
                            break
                        if (stateVectorModified[ii+tempCounter] == 'IDLE'  or  stateVectorModified[ii+tempCounter] == 'SLEEP'):
                            stateVectorModified[ii+tempCounter] = 'SLEEP_BUFFER'   
                            # stateVectorModified[ii+tempCounter] = 'ON'    # Use this only for Case 4e   
                            
                        if (stateVectorModified[ii+tempCounter] == 'BCN_RAMPUP'):
                            stateVectorModified[ii+tempCounter] = 'BCN_RAMPUP_BUFFER'
                        if (stateVectorModified[ii+tempCounter] == 'BCN_RAMPDOWN'):
                            stateVectorModified[ii+tempCounter] = 'BCN_RAMPDOWN_BUFFER'

                        
                
            elif (stateVectorModified[ii] == 'ACK_802_11_RX'):  
                # This can be ACK for any packet - so should be handled as general case
                stateVectorRampUpDown.append('ACK_802_11_RX')
                durationVectorRampUpDown.append(durationVectorModified[ii])
                timeStampVectorRampUpDown.append(timeStampVectorModified[ii])

                if (enableStateTransitions and ii < (stateVectorLength-1)):
                    tempNextState = stateVectorModified[ii+1]
                    transitionState = 'None'
                    # print(f"next states: {stateVectorModified[ii+1], stateVectorModified[ii+2]}; Duration: {durationVectorModified[ii+1], durationVectorModified[ii+2]}")
                    if (tempNextState == 'SLEEP'):
                        transitionState = 'ACK_802_11_RX_RAMPDOWN'
                    elif (tempNextState == 'SLEEP_BUFFER'):
                        transitionState = 'ACK_802_11_RX_RAMPDOWN_BUFFER'
                    else:
                        pass
                        # print(f"Did not add rampdown after ACK_802_11_RX - next state was {tempNextState} for {durationVectorModified[ii+1]}")
                    
                    
                    if (transitionState != 'None') and (durationVectorModified[ii+1] >= durationDict["ACK_802_11_RX_rampDown"]):
                        addedAlreadyLookAhead = True
                        stateVectorRampUpDown.append(transitionState)
                        durationVectorRampUpDown.append(durationDict["ACK_802_11_RX_rampDown"])
                        timeStampVectorRampUpDown.append(timeStampVectorModified[ii+1])
                        # Adding next idle time after deduction
                        stateVectorRampUpDown.append(tempNextState)
                        durationVectorRampUpDown.append(durationVectorModified[ii+1] - durationDict["ACK_802_11_RX_rampDown"])
                        timeStampVectorRampUpDown.append(timeStampVectorModified[ii+1]+ durationDict["ACK_802_11_RX_rampDown"])
                    elif (transitionState != 'None') and (durationVectorModified[ii+1] < durationDict["ACK_802_11_RX_rampDown"]):
                        stateVectorModified[ii+1] = transitionState
                    else:
                        pass

            elif (stateVectorModified[ii] == 'TCP_ACK_RX'):
                # Adding RampUp - using Beacon ramp up
                # if (stateVectorRampUpDown[-1] == 'IDLE' and durationVectorRampUpDown[-1] >= durationDict["TCP_TX_rampUp"]): 
                if (enableStateTransitions):
                    remainingRampUpDuration = durationDict["BCN_rampUp"]
                    temp_stateVector = []
                    temp_durationVector = []
                    temp_timeStampVector = []
                    while (remainingRampUpDuration > 0):
                        if (durationVectorRampUpDown[-1] <= remainingRampUpDuration):
                            remainingRampUpDuration = remainingRampUpDuration - durationVectorRampUpDown[-1]
                            #print(stateVectorRampUpDown)
                            if stateCompare('BCN_RAMPUP', stateVectorRampUpDown[-1]):
                                stateVectorRampUpDown[-1] = "BCN_RAMPUP"
                            # Adding the previous state to temp vectors so that they can be popped
                            temp_stateVector.append(stateVectorRampUpDown[-1])
                            temp_durationVector.append(durationVectorRampUpDown[-1])
                            temp_timeStampVector.append(timeStampVectorRampUpDown[-1])
                            # Popping the last states in the vectors
                            stateVectorRampUpDown.pop()
                            durationVectorRampUpDown.pop()
                            timeStampVectorRampUpDown.pop()
                        else:
                            durationVectorRampUpDown[-1] = durationVectorRampUpDown[-1] - remainingRampUpDuration
                            timeStampVectorRampUpDown.append(timeStampVectorRampUpDown[-1] + durationVectorRampUpDown[-1])
                            stateVectorRampUpDown.append('BCN_RAMPUP')
                            durationVectorRampUpDown.append(remainingRampUpDuration)
                            remainingRampUpDuration = 0     # so that loop ends
                            temp_stateVector.reverse()
                            temp_durationVector.reverse()
                            temp_timeStampVector.reverse()
                            
                            stateVectorRampUpDown.extend(temp_stateVector)
                            durationVectorRampUpDown.extend(temp_durationVector)
                            timeStampVectorRampUpDown.extend(temp_timeStampVector)
                    stateVectorRampUpDown.append('TCP_ACK_RX')
                    durationVectorRampUpDown.append(durationVectorModified[ii])
                    timeStampVectorRampUpDown.append(timeStampVectorModified[ii])  

            # Adding RampDown After 802.11 ACK Tx    
            elif (stateVectorModified[ii] == 'ACK_802_11_TX'):
                stateVectorRampUpDown.append('ACK_802_11_TX')
                durationVectorRampUpDown.append(durationVectorModified[ii])
                timeStampVectorRampUpDown.append(timeStampVectorModified[ii])
                #print("found ack")
                if enableStateTransitions and ((ii+1) < stateVectorLength):
                    if (stateVectorModified[ii+1] == 'SLEEP' and durationVectorModified[ii+1] >= durationDict["ACK_802_11_TX_rampDown"]):
                        addedAlreadyLookAhead = True
                        stateVectorRampUpDown.append('ACK_802_11_TX_RAMPDOWN')
                        durationVectorRampUpDown.append(durationDict["ACK_802_11_TX_rampDown"])
                        timeStampVectorRampUpDown.append(timeStampVectorModified[ii+1])
                        # Adding next idle time after deduction
                        stateVectorRampUpDown.append('SLEEP')
                        durationVectorRampUpDown.append(durationVectorModified[ii+1] - durationDict["ACK_802_11_TX_rampDown"])
                        timeStampVectorRampUpDown.append(timeStampVectorModified[ii+1]+ durationDict["ACK_802_11_TX_rampDown"])
                    elif (stateVectorModified[ii+1] == 'SLEEP' and durationVectorModified[ii+1] < durationDict["ACK_802_11_TX_rampDown"]):   # Available IDLE state is less than needed. Just change the available to ACK_802_11_TX_RAMPDOWN. No lookahead
                        stateVectorModified[ii+1] = 'ACK_802_11_TX_RAMPDOWN'
                        #print(f"Warning at ii: {ii}. TCP RampDown added with less duration\ndurationVectorModified[ii+1] = {durationVectorModified[ii+1]} \n durationDict["ACK_802_11_TX_rampDown"]={durationDict["ACK_802_11_TX_rampDown"]}")
                    else:
                        pass
                        # print(f"Error at ii: {ii}. Could not add TCP RampDown\ndurationVectorModified[ii+1] = {durationVectorModified[ii+1]} \n durationDict['ACK_802_11_TX_rampDown']={durationDict['ACK_802_11_TX_rampDown']}\nstateVectorModified[ii+1]= {stateVectorModified[ii+1]}")

                    
            else:
                # Just some other state
                stateVectorRampUpDown.append(stateVectorModified[ii])
                durationVectorRampUpDown.append(durationVectorModified[ii])
                timeStampVectorRampUpDown.append(timeStampVectorModified[ii])


    # %%
    # Assigning current values
    # print(stateVectorRampUpDown)
    totalDuration_us = int(np.sum(durationVectorRampUpDown))
    # if (totalDuration_us == 0):
    #     continue
    #print(totalDuration_us)
    ampereTimeSeries = np.zeros(totalDuration_us)
    #print(ampereTimeSeries.shape)
    currentTimePointer = 0
    # print (f"Final stateVectorRampUpDown: {stateVectorRampUpDown}")
    # print (f"Final durationVectorRampUpDown: {durationVectorRampUpDown}")
    for ii in range(len(stateVectorRampUpDown)):
        # final states print here
        print (f"{stateVectorRampUpDown[ii]} for {durationVectorRampUpDown[ii]/1000} ms")
        if stateVectorRampUpDown[ii] in stateDict:
            tempCurrent = stateDict[stateVectorRampUpDown[ii]]
        else:
            print (f" Error at {ii}. Did not find State {stateVectorRampUpDown[ii]} in Dict")
        
        
        #print(durationVector[ii])
        #print(f"{currentTimePointer}, {durationVectorRampUpDown[ii]}")
        ampereTimeSeries[currentTimePointer:currentTimePointer + durationVectorRampUpDown[ii]] = tempCurrent
        #print(ampereTimeSeries[currentTimePointer:durationVector[ii]])
        currentTimePointer = currentTimePointer + durationVectorRampUpDown[ii]


    # %%

    # *****************************************************************

    # Measuring the average current for this time window

    simAvgCurrent = np.sum(ampereTimeSeries[:totalDuration_us])/totalDuration_us


    print(f"Average current consumed by simulation: {simAvgCurrent*1000} mA")
    print(f"awakeTime_us_this: {awakeTime_us_this}")
    print(f"txTime_us_this: {txTime_us_this}")
    print(f"rxTime_us_this: {rxTime_us_this}")
    print(f"Collisions: {collisionCount_this}")

    # print(f"Network RTT: {networkRTT_us} us =  {networkRTT_us/1000} ms\nTime to next beacon : {timeToNextBeacon_us} us = {timeToNextBeacon_us/1000} ms ")

    # Calculate Channel busy ratio
    channelBusyRatio_us_this = channelBusyTime_us_this/totalDuration_us
    # Appending to lists in the correct units
    averageCurrent_mA_list.append(simAvgCurrent*1000)
    totalDuration_us_list.append(totalDuration_us)
    channelBusyTime_us_list.append(channelBusyTime_us_this)
    channelBusyRatio_us_list.append(channelBusyRatio_us_this)
    stateTransitions_list.append (int(enableStateTransitions))
    bufferCurrent_list.append (int(enableBufferCurrent))
    awakeTime_us_list.append(int (awakeTime_us_this))
    txTime_us_list.append(int (txTime_us_this))
    rxTime_us_list.append(int (rxTime_us_this))
    collisionCount_list.append (collisionCount_this)

    # # Plotting
           
    xAxis = np.arange(0,totalDuration_us)
    plt.figure(num=None, figsize=(14, 6), dpi=80, facecolor='w', edgecolor='k')
    plt.plot(xAxis/1000,ampereTimeSeries[:totalDuration_us]*1000, 'b')


    plt.xlabel("Time (ms)")
    plt.ylabel("Current (mA)")
    plt.legend(["ns3"], loc = "upper right")


    # textString = "Scenario A \nNetwork RTT: " + str(networkRTT_us/1000) + " ms\nTime to next beacon: " + str(timeToNextBeacon_us/1000) + " ms\n"
    
    # font = {'family': 'serif',
    #     'color':  'black',
    #     'weight': 'normal',
    #     'size': 12,
    #     }
    # plt.text(2, 190, textString, fontdict = font,
    #              bbox=dict(boxstyle="square",
    #                ec=(0, 0, 0),
    #                fc=(1., 1, 1),
    #                ))
    
    # Master Loop ends here
    # ----------------------------------------------------------------------------------------------------
# os.system('cls||clear')

# %%
print(f"\n\n----------------------------------------------------------------\nCompleted processing {logCount} simulations.")

# print(f"Average current List in mA: {averageCurrent_mA_list}")

# writing to a csv file
# CSV file format
#  averageCurrent_mA

dataFrameToWrite = pd.DataFrame(list(zip(index_list, totalDuration_us_list,  averageCurrent_mA_list, channelBusyTime_us_list, channelBusyRatio_us_list, stateTransitions_list, bufferCurrent_list, awakeTime_us_list, txTime_us_list, rxTime_us_list, collisionCount_list, successTcpULCount_list, ULAckDelay_us_avg_list)), columns= ['SimulationID', 'totalDuration_us', 'averageCurrent_mA', 'channelBusyTime_us','channelBusyRatio_us', 'stateTransitions_list', 'bufferCurrent_list', 'awakeTime_us_list', 'txTime_us_list', 'rxTime_us_list','collisionCount_list', 'successTcpULCount_list', 'ULAckDelay_us_avg_list'])
with pd.option_context('display.max_rows', None, 'display.max_columns', None):
    print(dataFrameToWrite)

dataFrameToWrite.to_csv(csvFileName, index  = False)



