randSeed=1
logFileName=mswim2023Jun10Sims.txt
packetsPerSecond=10


# 10000 = no TWT contention case, UL only
loopIndex=10000
simulationTime=40
for nSTA in 5 10 15 20
do
    for randSeed in 1000 2000 3000 4000 5000 6000 7000 8000 9000 9999 
    do
        echo "--loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --simulationTime=${simulationTime} --maxMuSta=1 --enableUplink=true --enableDownlink=false --videoQuality=1 --useCase=noPs --enablePcap=false --twtTriggerBased=false" >> ${logFileName}
        ./ns3 --run "videoBidirectionalTWT --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --simulationTime=${simulationTime} --maxMuSta=1 --enableUplink=true --enableDownlink=false --videoQuality=1 --useCase=noPs --enablePcap=false --twtTriggerBased=false" >> logSet1Jun10.out 
        loopIndex=$(($loopIndex+1))
        # loopIndexDummy=$(($loopIndexDummy+1))
    done
done

# 11000 = TWT 1-MU case, UL only
loopIndex=11000
simulationTime=40
for nSTA in 5 10 15 20
do
    for randSeed in 1000 2000 3000 4000 5000 6000 7000 8000 9000 9999 
    do
        echo "--loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --simulationTime=${simulationTime} --maxMuSta=1 --enableUplink=true --enableDownlink=false --videoQuality=1 --useCase=twt --enablePcap=false --twtTriggerBased=true" >> ${logFileName}
        ./ns3 --run "videoBidirectionalTWT --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --simulationTime=${simulationTime} --maxMuSta=1 --enableUplink=true --enableDownlink=false --videoQuality=1 --useCase=twt --enablePcap=false --twtTriggerBased=true" >> logSet1Jun10.out 
        loopIndex=$(($loopIndex+1))
        # loopIndexDummy=$(($loopIndexDummy+1))
    done
done


# 12000 = TWT 4-MU case, UL only
loopIndex=12000
simulationTime=40
for nSTA in 5 10 15 20
do
    for randSeed in 1000 2000 3000 4000 5000 6000 7000 8000 9000 9999 
    do
        echo "--loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --simulationTime=${simulationTime} --maxMuSta=4 --enableUplink=true --enableDownlink=false --videoQuality=1 --useCase=twt --enablePcap=false --twtTriggerBased=true" >> ${logFileName}
        ./ns3 --run "videoBidirectionalTWT --loopIndex=${loopIndex} --randSeed=${randSeed} --nStations=${nSTA} --simulationTime=${simulationTime} --maxMuSta=4 --enableUplink=true --enableDownlink=false --videoQuality=1 --useCase=twt --enablePcap=false --twtTriggerBased=true" >> logSet1Jun10.out 
        loopIndex=$(($loopIndex+1))
        # loopIndexDummy=$(($loopIndexDummy+1))
    done
done