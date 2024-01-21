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

# Note: STA0 is AP

# %%

import string
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


# csvFileName = 'twt100Sta9May.csv'          #changeThisBeforeRun
csvFileName = 'mobicomFeb27.csv'          #changeThisBeforeRun
# staIdList = list(range (1, 101, 1))      #changeThisBeforeRun
# staIdList = list(range (0, 41, 1))      #changeThisBeforeRun
# staIdList = list(range (0, 5, 1))      #changeThisBeforeRun
# staIdList = [0, 1]                        #changeThisBeforeRun              
staIdList = [1]                        #changeThisBeforeRun              
# data_folder = Path("iccTwtLogs/trial")    #changeThisBeforeRun                                         
data_folder = Path("iccTwtLogs")    #changeThisBeforeRun                                         
plotting = True


# data_folder = Path("MU_logs/") 
stateLogStartTime_us = 13 * 1e6          # PHY state log will be imported only after this time from beginning #changeThisBeforeRun
stateLogEndTime_us = 13.5 * 1e6          # PHY state log will be imported only till this time #changeThisBeforeRun
# stateLogEndTime_us = 200 * 1e6          # PHY state log will be imported only till this time #changeThisBeforeRun
print (f"staIdList: {staIdList}")
printFinalStates = False

destIp = "192.168.2.2"



# Following lists will be appended at the end of each iteration. The lists are ordered and correspond to each other in the correct order.
index_list = [] 
processedStaId_list = []     # may have multiple entries for same staId - if there are multiple simulations - use index+staId to identify
totalDuration_us_list = []
channelBusyTime_us_list = []
channelBusyRatio_us_list = []
averageCurrent_mA_list = []
awakeTime_us_list = []
txTime_us_list = []
rxTime_us_list = []     
idleTime_us_list = []     
goodput_kbps_list = []    
ulLatency_us_list = []     
chIdleProb_1_list = []     # Channel Idle probabilty only for AP nodes out of 1
awakePercent_1_list = []   # Awake time % for STA nodes out of 1  

   

ns3_state_log_list_temp = list(data_folder.glob("**/*.statelog"))
ns3_state_log_list = [str(x) for x in ns3_state_log_list_temp]

# ns3_goodput_log_list_temp = list(data_folder.glob("**/*.goodputlog"))
# ns3_goodput_log_list = [str(x) for x in ns3_goodput_log_list_temp]

# ns3_flow_log_list_temp = list(data_folder.glob("**/*.flowlog"))
# ns3_flow_log_list = [str(x) for x in ns3_flow_log_list_temp]


ns3_state_log_list.sort()
# ns3_goodput_log_list.sort()
# ns3_flow_log_list.sort()



logCount = len(ns3_state_log_list)
print (f"Total State log count: {logCount}")

# print (f"Total goodput log count: {len(ns3_goodput_log_list)}")

# print (f"Total flow log count: {len(ns3_flow_log_list)}")


# if ( (len(ns3_state_log_list)!=len(ns3_goodput_log_list)  )   or  (len(ns3_state_log_list)!=len(ns3_flow_log_list)) ):
#     print ("Log counts are not matching - Exiting")
#     quit ()
    

# For random log selection ------------------------------below - just delete this block for batch processing
# chosenLog = randrange(logCount)
# chosenLog = 5
# print(f"Chosen log: {chosenLog}")
# ns3_state_log_list = [ns3_state_log_list[chosenLog]]

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


# State Dictionary definition - Current values are in Amperes

stateDict["SLEEP"] =  0.12e-3                                        # IDLE state - will be assigned LPDS current after processing
# stateDict["ON"] =  66e-3                                            # Used for Always on power modes - value from experiments
stateDict["ON"] =  50e-3                                            # Used for Always on power modes - value from experiments
stateDict["RX"] = 66e-3    
stateDict["IDLE"] =  50e-3                                     # IDLE state - will be assigned Rx current after processing
stateDict["CCA_BUSY"] =  50e-3                                      # CCA_BUSY state - will be assigned Rx current after processing

stateDict["TX"] = 232e-3                                            # This will be referenced in multiple Tx states but not used directly
                                        




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
    ns3_state_log = ns3_state_log_list[logFilesCounter]
    print(f"Chosen state log: {ns3_state_log}")
    # ns3_goodput_log = ns3_goodput_log_list[logFilesCounter]
    # ns3_flow_log = ns3_flow_log_list[logFilesCounter]


    # Checking if log and trace files have the same number
    
    currentIndex = ns3_state_log.split('.')[0][-6:]
    
    print(f"\nProcessing log file ({logFilesCounter+1}/{logCount})")


    for currentStaId in staIdList:
        index_list.append(currentIndex)
        print(f"\nProcessing staId: {currentStaId}")
        processedStaId_list.append (currentStaId)
        
        
        # logFilesCounter = 0
        awakeTime_us_this = 0
        txTime_us_this = 0
        rxTime_us_this = 0
        idleTime_us_this = 0

        
        
        

        stateVector = []
        durationVector = []
        timeStampVector = []

        lastStateEnd_ns = 0
        with open(ns3_state_log) as tsv:
            #for line in csv.reader(tsv, dialect="excel-tab"):
            prevState = 'None'
            for line in csv.reader(tsv, delimiter = ";"):
                
                staIdinThisLine = int(line[4].split("=")[1])
                # print(f"staIdinThisLine: {staIdinThisLine}")
                # print(f"staIdinThisLine==currentStaId: {staIdinThisLine==currentStaId}")
                if (staIdinThisLine != currentStaId):
                    continue
                
                # print(line)

                currentState = line[0].split("=")[1]

                currentTimeStamp = int(line[1].split("=")[1])
                # currentTimeStamp = np.ceil( currentTimeStamp/1000)
                
                
                currentDuration = int(line[2].split("=")[1])
                # currentDuration = np.ceil( currentDuration/1000)
                #Adding before rounding.
                 
                
                
                if (lastStateEnd_ns >0) and (currentTimeStamp < lastStateEnd_ns):
                    continue
                currentTimeStamp = currentTimeStamp + currentDuration  
                
                lastStateEnd_ns = currentTimeStamp
                currentDuration = int( currentDuration/1000)
                currentTimeStamp = int( currentTimeStamp/1000)

                

                # Note : currentTimeStamp is the time when the state completes. Not when it begins.

                
                if (currentTimeStamp < stateLogStartTime_us ) or (currentTimeStamp > stateLogEndTime_us ) :
                    continue
                
                # if currentState == "CCA_BUSY" and prevState == "SLEEP":
                #     # Bug with ns3 state logging system - Every SLEEP state is followed by a duplicate CCA_BUSY state - should be ignored
                #     # print (f"Found CCA_BUSY after SLEEP at time = {currentTimeStamp}; Skipping.")
                #     continue
                
                # if currentState == "CCA_BUSY":
                #     currentState = "IDLE"

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
                    if currentState == 'RX' or currentState == 'TX' or currentState == 'IDLE' or currentState == 'CCA_BUSY':
                        awakeTime_us_this = awakeTime_us_this + currentDuration
                    if currentState == 'TX':
                        txTime_us_this = txTime_us_this + currentDuration
                    if currentState == 'RX' or currentState == 'IDLE' or currentState == 'CCA_BUSY':
                        rxTime_us_this = rxTime_us_this + currentDuration
                    if currentState == 'IDLE' :
                        idleTime_us_this = idleTime_us_this + currentDuration
                        
                
  


        

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
            
            stateVectorModified.append(stateVector[ii])
            durationVectorModified.append(durationVector[ii])
            timeStampVectorModified.append(timeStampVector[ii])
            
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
        # Adding Beacon ramp up and down
        # For BCN_RX states, adding Ramp up and Ramp down if IDLE state is available

        # print(f"Before beacon rampUP down: Total duration: {np.sum(durationVectorModified)}")
        
        stateVectorRampUpDown = stateVectorModified
        durationVectorRampUpDown = durationVectorModified
        timeStampVectorRampUpDown = timeStampVectorModified    
    


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
            if (printFinalStates):
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
        print(f"totalDuration_seconds: {totalDuration_us/1000000}")
        print(f"awakeTime_us_this: {awakeTime_us_this}")
        print(f"txTime_us_this: {txTime_us_this}")
        print(f"rxTime_us_this: {rxTime_us_this}")
        print(f"idleTime_us_this: {idleTime_us_this}")
        

        # Appending to lists in the correct units
        averageCurrent_mA_list.append(simAvgCurrent*1000)
        totalDuration_us_list.append(totalDuration_us)
        awakeTime_us_list.append(int (awakeTime_us_this))
        txTime_us_list.append(int (txTime_us_this))
        rxTime_us_list.append(int (rxTime_us_this))
        idleTime_us_list.append(int (idleTime_us_this))
        
        if (currentStaId == 0):
            # This is an AP
            chIdleProb_1_list.append (idleTime_us_this*1.0/(totalDuration_us*1.0) )
            awakePercent_1_list.append ('NA')
        else:
            # This is an STA
            chIdleProb_1_list.append ('NA')
            awakePercent_1_list.append (awakeTime_us_this*1.0/(totalDuration_us*1.0) )



        # # Processing Goodput
        # if (currentStaId == 0):
        #     # This is the AP
        #     goodput_kbps_list.append ('NA')
        # else:
        #     # This is an STA - open the goodput log
        #     with open(ns3_goodput_log) as tsv:
        #         for line in csv.reader(tsv, delimiter = ";"):
        #             staIdinThisLine = int(line[1].split("=")[1])
        #             print (f"staIdinThisLine: {staIdinThisLine}")
        #             if (staIdinThisLine != currentStaId):
        #                 continue
                    
        #             currentGoodput = line[2].split("=")[1]
        #             print (f"currentGoodput: {currentGoodput}")
        #             goodput_kbps_list.append(currentGoodput)
                    

        # # Processing UL latency
        # if (currentStaId == 0):
        #     # This is the AP
        #     ulLatency_us_list.append ('NA')
        # else:
        #     # This is an STA - open the flow log
        #     with open(ns3_flow_log) as tsv:
        #         flowCounter = 0
        #         for line in csv.reader(tsv, delimiter = ";"):
                    
        #             destIpInThisLine = line[2].split("=")[1]
        #             if (destIpInThisLine == destIp):
        #                 flowCounter = flowCounter + 1
        #             if (flowCounter == currentStaId):
        #                 currentLatency = line[4].split("=")[1]
        #                 ulLatency_us_list.append(currentLatency)




        # # Plotting
        if (plotting):
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
        
        print (f"Finished processing staId: {currentStaId} of Index: {currentIndex}")
        # staId Loop ends here
        # ----------------------------------------------------------------------------------------------------
    
    # print ("Finished processing all staId's in Index: {currentIndex}")
    # Index Loop ends here
    # ----------------------------------------------------------------------------------------------------
# os.system('cls||clear')

# %%
print(f"\n\n----------------------------------------------------------------\nCompleted processing {logCount} simulations.")

# print(f"Average current List in mA: {averageCurrent_mA_list}")

# writing to a csv file
# CSV file format
#  averageCurrent_mA

# dataFrameToWrite = pd.DataFrame(list(zip(index_list, processedStaId_list, totalDuration_us_list,  averageCurrent_mA_list, awakeTime_us_list, txTime_us_list, rxTime_us_list, idleTime_us_list, chIdleProb_1_list, awakePercent_1_list, goodput_kbps_list, ulLatency_us_list)), columns= ['SimulationID', 'processedStaId', 'totalDuration_us', 'averageCurrent_mA', 'awakeTime_us', 'txTime_us', 'rxTime_us', 'idleTime_us', 'chIdleProb_1_list','awakePercent_1_list', 'goodput_kbps_list', 'ulLatency_us_list'])
dataFrameToWrite = pd.DataFrame(list(zip(index_list, processedStaId_list, totalDuration_us_list,  averageCurrent_mA_list, awakeTime_us_list, txTime_us_list, rxTime_us_list, idleTime_us_list, chIdleProb_1_list, awakePercent_1_list)), columns= ['SimulationID', 'processedStaId', 'totalDuration_us', 'averageCurrent_mA', 'awakeTime_us', 'txTime_us', 'rxTime_us', 'idleTime_us', 'chIdleProb_1_list','awakePercent_1_list'])
with pd.option_context('display.max_rows', None, 'display.max_columns', None):
    print(dataFrameToWrite)

dataFrameToWrite.to_csv(csvFileName, index  = False)