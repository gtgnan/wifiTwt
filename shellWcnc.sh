# logFileName=wcnc_vary_wakeDuration_bv1.txt

# # To evaluate impact of TWT SP duration (nominal wake duration) on network performance metrics for one STA with fixed TCP UL Video traffic
# nSTA=1
# loopIndex=11000
# simulationTime=25
# videoQuality=1
# useCase=twt
# scenario=vary_wakeDuration_bv1
# for randSeed in 1000 2000 3000 4000 5000
# do
#     for twtNominalWakeDurationDivider in 2 3 4 5 8 10 12 15 17 20 30 40 50 75 100
#     do
#         echo "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=false --twtWakeIntervalMultiplier=1 --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=4 --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
#         ./ns3 --run "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=false --twtWakeIntervalMultiplier=1 --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=4 --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
#         loopIndex=$(($loopIndex+1))
#     done
# done

# logFileName=wcnc_vary_wakeDuration_splitSP_bv1.txt

# # # To evaluate impact of TWT SP duration (nominal wake duration) on network performance metrics for one STA with fixed TCP UL Video traffic. In this case, the SPs are split. Equivalently, wake interval and duration are divided by 2
# nSTA=1
# loopIndex=11100
# simulationTime=25
# videoQuality=1
# useCase=twt
# twtWakeIntervalMultiplier=0.5
# scenario=vary_wakeDuration_bv1_splitSP

# for randSeed in 1000 2000 3000 4000 5000 6000 7000
# do
#     for twtNominalWakeDurationDivider in 4 5 8 10 12 15 17 20 25 30 40 50 75 100
#     do
#         echo "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=false --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=4 --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
#         ./ns3 --run "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=false --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=4 --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
#         loopIndex=$(($loopIndex+1))
#     done
# done

# logFileName=wcnc_vary_wakeDuration_CAM_bv1.txt

# # To evaluate impact of TWT SP duration (nominal wake duration) on network performance metrics for one STA with fixed TCP UL Video traffic - THis is for 100% duty cycle
# nSTA=1
# loopIndex=11100
# simulationTime=40
# videoQuality=1
# useCase=noTwt
# scenario=vary_wakeDuration_bv1_CAM
# for randSeed in 1000 2000 3000 4000 5000
# do
#     for twtNominalWakeDurationDivider in 2  # does not matter
#     do
#         echo "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=false --twtWakeIntervalMultiplier=1 --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=4 --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
#         ./ns3 --run "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=false --twtWakeIntervalMultiplier=1 --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=4 --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
#         loopIndex=$(($loopIndex+1))
#     done
# done




# logFileName=wcncTwt_dedicated_scenario1.txt

# # In this scenario, number of STAs is increased from 4, 8, ... 24. TWT SPs are stacked one after the other. SP duration = BI/12
# # this is non TB case. Duty cycle depends on nSTA
# nSTA=1
# loopIndex=120000
# simulationTime=15
# videoQuality=1
# useCase=twt  
# twtNominalWakeDurationDivider=12
# # twtWakeIntervalMultiplier is nSTA/twtNominalWakeDurationDivider; It is a double value
# twtWakeIntervalMultiplier=$(echo "scale=3; $nSTA/12" | bc -l)   #dummmy
# nextStaTwtSpOffsetDivider=12
# enablePcap=false
# enableVideoUplink=true
# twtTriggerBased=false
# scenario=Twt_dedicated_scenario1


# for randSeed in 400 500 600 700 800
# do
#     for nSTA in 4 8 12 16 20 24
#     do
#         twtWakeIntervalMultiplier=$(echo "scale=3; $nSTA/12" | bc -l)
#         echo "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=${enableVideoUplink} --enableVideoDownlink=false --enablePcap=${enablePcap} --useCase=${useCase} --twtTriggerBased=${twtTriggerBased} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
#         ./ns3 --run "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=${enableVideoUplink} --enableVideoDownlink=false --enablePcap=${enablePcap} --useCase=${useCase} --twtTriggerBased=${twtTriggerBased} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
#         loopIndex=$(($loopIndex+1))
#     done
# done

# logFileName=wcncTwt_dedicated_scenario2.txt

# # In this scenario, number of STAs is increased from 4, 8, ... 24. TWT agreements always have same wake interval = 2*BI. SP duration = BI/12
# # this is non TB case. Duty cycle is always same
# nSTA=1
# loopIndex=30000
# simulationTime=15
# videoQuality=1
# useCase=twt
# twtNominalWakeDurationDivider=12
# # twtWakeIntervalMultiplier is constant = 2 BI
# twtWakeIntervalMultiplier=2
# nextStaTwtSpOffsetDivider=12
# twtTriggerBased=false
# scenario=Twt_dedicated_scenario2

# for randSeed in 400 500 600 700 800
# do
#     for nSTA in 4 8 12 16 20 24
#     do
#         echo "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=${twtTriggerBased} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
#         ./ns3 --run "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=${twtTriggerBased} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
#         loopIndex=$(($loopIndex+1))
#     done
# done


# logFileName=wcncTwt_dedicated_scenario3.txt

# # In this scenario, number of STAs is increased from 4, 8, ... 24. TWT SPs are stacked one after the other. SP duration = 2*BI/N
# # this is non TB case. Duty cycle depends on nSTA
# nSTA=1
# loopIndex=130000
# simulationTime=15
# videoQuality=1
# useCase=twt  
# twtNominalWakeDurationDivider=12    #dummy
# # twtWakeIntervalMultiplier is nSTA/twtNominalWakeDurationDivider; It is a double value
# twtWakeIntervalMultiplier=2
# nextStaTwtSpOffsetDivider=12    #dummy
# enablePcap=false
# enableVideoUplink=true
# twtTriggerBased=false
# scenario=Twt_dedicated_scenario3


# for randSeed in 400 500 600 700 800
# do
#     for nSTA in 4 8 12 16 20 24
#     do
#         twtNominalWakeDurationDivider=$(echo "scale=3; $nSTA/2" | bc -l)
#         nextStaTwtSpOffsetDivider=$(echo "scale=3; $nSTA/2" | bc -l)
#         echo "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=${enableVideoUplink} --enableVideoDownlink=false --enablePcap=${enablePcap} --useCase=${useCase} --twtTriggerBased=${twtTriggerBased} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
#         ./ns3 --run "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=${enableVideoUplink} --enableVideoDownlink=false --enablePcap=${enablePcap} --useCase=${useCase} --twtTriggerBased=${twtTriggerBased} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
#         loopIndex=$(($loopIndex+1))
#     done
# done


logFileName=wcncTwt_shared_TB_VaryMU_8StaTotal_scenario.txt

# In this scenario, nSTA=8. All 8 share the same TWT-SP. 50% duty cycle. BI/2 duration; interval=BI; MaxMU is varied
# this is TB case. Duty cycle is same. MU 1

nSTA=8
loopIndex=330000
simulationTime=15
videoQuality=1
useCase=twt
twtNominalWakeDurationDivider=2     # 50% duty cycle
twtWakeIntervalMultiplier=1
nextStaTwtSpOffsetDivider=1     #dummy - will not be used
staCountModulusForTwt=8     # all STAs will be in same SP
maxMuSta=1
twtTriggerBased=true
scenario=shared_TB_VaryMU_8StaTotal_scenario

for randSeed in 5000 4000 3000 2000 1000 500 400 300
do
    for maxMuSta in 1 2 4
    do
        echo "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=${twtTriggerBased} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --staCountModulusForTwt=${staCountModulusForTwt} --videoQuality=${videoQuality} --maxMuSta=${maxMuSta} --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
        ./ns3 --run "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=${twtTriggerBased} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --staCountModulusForTwt=${staCountModulusForTwt} --videoQuality=${videoQuality} --maxMuSta=${maxMuSta} --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
        loopIndex=$(($loopIndex+1))
    done
done


# logFileName=wcncTwt_shared_TB_MU1_4STAperSP_scenario.txt

# # In this scenario, number of STAs is increased from 4, 8, ... 24. TWT agreements always have same wake interval = 2*BI. SP duration = 4*BI/12
# # this is TB case. Duty cycle is same. MU 1

# nSTA=1
# loopIndex=30000
# simulationTime=15
# videoQuality=1
# useCase=twt
# twtNominalWakeDurationDivider=3
# twtWakeIntervalMultiplier=2
# nextStaTwtSpOffsetDivider=3
# staCountModulusForTwt=4
# maxMuSta=1
# twtTriggerBased=true
# scenario=Twt_shared4PerSP_TB_MU1_scenario

# for randSeed in 5000 4000 3000 2000 1000
# do
#     for nSTA in 4 8 12 16 20 24
#     do
#         echo "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=${twtTriggerBased} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --staCountModulusForTwt=${staCountModulusForTwt} --videoQuality=${videoQuality} --maxMuSta=${maxMuSta} --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
#         ./ns3 --run "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=${twtTriggerBased} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --staCountModulusForTwt=${staCountModulusForTwt} --videoQuality=${videoQuality} --maxMuSta=${maxMuSta} --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
#         loopIndex=$(($loopIndex+1))
#     done
# done

# logFileName=wcncTwt_shared_TB_MU1_4STAperSP_scenario.txt

# # In this scenario, number of STAs is increased from 4, 8, ... 24. TWT agreements always have same wake interval = 2*BI. SP duration = 4*BI/12
# # this is TB case. Duty cycle is same. MU 1

# nSTA=1
# loopIndex=40000
# simulationTime=15
# videoQuality=1
# useCase=twt
# twtNominalWakeDurationDivider=3
# twtWakeIntervalMultiplier=2
# nextStaTwtSpOffsetDivider=3
# staCountModulusForTwt=4
# maxMuSta=4
# twtTriggerBased=true
# scenario=Twt_shared4PerSP_TB_MU4_scenario

# for randSeed in 5000 4000 3000 2000 1000
# do
#     for nSTA in 4 8 12 16 20 24
#     do
#         echo "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=${twtTriggerBased} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --staCountModulusForTwt=${staCountModulusForTwt} --videoQuality=${videoQuality} --maxMuSta=${maxMuSta} --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
#         ./ns3 --run "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=${twtTriggerBased} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --staCountModulusForTwt=${staCountModulusForTwt} --videoQuality=${videoQuality} --maxMuSta=${maxMuSta} --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
#         loopIndex=$(($loopIndex+1))
#     done
# done


# # 10000 = no TWT contention case, UL Video TCP only
# logFileName=wcncCAMVideo.txt
# loopIndex=10000
# simulationTime=15
# videoQuality=1
# scenario=bv1_CAM_increase_nSTA

# for randSeed in 400 500 600 700 800
# do
#     for nSTA in 24 20 16 12 8 4
#     do
#         echo "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=noTwt --twtTriggerBased=true --twtWakeIntervalMultiplier=1 --twtNominalWakeDurationDivider=4 --nextStaTwtSpOffsetDivider=4 --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
#         ./ns3 --run "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=noTwt --twtTriggerBased=true --twtWakeIntervalMultiplier=1 --twtNominalWakeDurationDivider=4 --nextStaTwtSpOffsetDivider=4 --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
#         loopIndex=$(($loopIndex+1))
#     done
# done

# logFileName=wcncTwt_shared_nonTB_scenario1.txt

# # In this scenario, number of STAs is increased from 4, 8, ... 32. TWT agreements always have same wake interval = 2*BI. SP duration = 2*BI/4
# # this is non TB case. Duty cycle is same
# nSTA=1
# loopIndex=30000
# simulationTime=25
# videoQuality=1
# useCase=twt
# twtNominalWakeDurationDivider=2
# twtWakeIntervalMultiplier=2
# nextStaTwtSpOffsetDivider=2
# scenario=Twt_shared_nonTB_scenario1

# for randSeed in 5000 4000 3000 2000 1000
# do
#     for nSTA in 4 8 12 16 20 24 28 32
#     do
#         echo "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=false --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
#         ./ns3 --run "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=false --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
#         loopIndex=$(($loopIndex+1))
#     done
# done


# logFileName=wcncTwt_shared_nonTB_scenario2.txt

# # In this scenario, number of STAs is increased from 4, 8, ... 32. TWT agreements always have same wake interval = BI. SP duration = BI/4
# # this is non TB case. Duty cycle is same
# nSTA=1
# loopIndex=30000
# simulationTime=25
# videoQuality=1
# useCase=twt
# twtNominalWakeDurationDivider=4
# twtWakeIntervalMultiplier=1
# nextStaTwtSpOffsetDivider=4
# scenario=Twt_shared_nonTB_scenario2

# for randSeed in 5000 4000 3000 2000 1000
# do
#     for nSTA in 4 8 12 16 20 24 28 32
#     do
#         echo "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=false --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
#         ./ns3 --run "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=false --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
#         loopIndex=$(($loopIndex+1))
#     done
# done



# Bug in this scenario - do not run
# logFileName=wcncTwt_shared_TB_50msWakeDur_MU1.txt

# # In this scenario, number of STAs is increased from 4, 8, ... 32. TWT SPs are Shared. Duration = 50ms; STAs are equally assigned. Duty cycle is always 25%. There are 4 SPs in 2 BI
# # this is TB case with MU 1, 2, and 4
# nSTA=1
# loopIndex=63100
# simulationTime=15
# videoQuality=1
# useCase=twt
# # for 50ms SPs - below
# twtNominalWakeDurationDivider=2
# twtWakeIntervalMultiplier=2
# nextStaTwtSpOffsetDivider=2
# maxMuSta=4
# twtTriggerBased=true
# enablePcap=false
# scenario=sharedTwt_TB_50msWakeDur_MU4

# for randSeed in 1000 2000 3000 4000 5000
# do
#     for nSTA in 4 8 12 16 20 24 28 32
#     do
#         echo "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=${enablePcap} --useCase=${useCase} --twtTriggerBased=${twtTriggerBased} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --videoQuality=${videoQuality} --maxMuSta=${maxMuSta} --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
#         ./ns3 --run "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=${enablePcap} --useCase=${useCase} --twtTriggerBased=${twtTriggerBased} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --videoQuality=${videoQuality} --maxMuSta=${maxMuSta} --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
#         loopIndex=$(($loopIndex+1))
#     done
# done

# logFileName=wcncTwt_shared_SU_nonTB_50msWakeDur.txt

# # In this scenario, number of STAs is increased from 4, 8, ... 32. TWT SPs are Shared. Duration = 50ms; STAs are equally assigned. Duty cycle is always 25%. There are 4 SPs in 2 BI
# # this is non TB case
# nSTA=1
# loopIndex=50000
# simulationTime=25
# videoQuality=1
# useCase=twt
# twtNominalWakeDurationDivider=2
# # twtWakeIntervalMultiplier is nSTA/twtNominalWakeDurationDivider; It is a double value
# twtWakeIntervalMultiplier=2
# nextStaTwtSpOffsetDivider=2

# for randSeed in 1000 2000 3000 4000 5000
# do
#     for nSTA in 4 8 12 16 20 24 28 32
#     do
#         echo "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=false --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
#         ./ns3 --run "twtRedesignVideo --scenario=${scenario} --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=${useCase} --twtTriggerBased=false --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --twtNominalWakeDurationDivider=${twtNominalWakeDurationDivider} --nextStaTwtSpOffsetDivider=${nextStaTwtSpOffsetDivider} --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
#         loopIndex=$(($loopIndex+1))
#     done
# done


# logFileName=wcncPsmVideo.txt


# # 10000 = no TWT contention case, UL Video TCP only
# loopIndex=10000
# simulationTime=25
# videoQuality=1

# for randSeed in 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000
# do
#     for nSTA in 4 8 12 16 20 24 28 32 36 40
#     do
#         echo "twtRedesignVideo_PSM --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=PSM --twtTriggerBased=true --twtWakeIntervalMultiplier=1 --twtNominalWakeDurationDivider=4 --nextStaTwtSpOffsetDivider=4 --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" >> ${logFileName}
#         ./ns3 --run "twtRedesignVideo_PSM --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --enableVideoUplink=true --enableVideoDownlink=false --enablePcap=false --useCase=PSM --twtTriggerBased=true --twtWakeIntervalMultiplier=1 --twtNominalWakeDurationDivider=4 --nextStaTwtSpOffsetDivider=4 --videoQuality=${videoQuality} --maxMuSta=1 --p2pLinkDelay=0 --simulationTime=${simulationTime}" > logVideo.out 2>&1
#         loopIndex=$(($loopIndex+1))
#     done
# done