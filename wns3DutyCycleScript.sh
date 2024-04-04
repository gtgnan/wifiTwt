logFileName=wns3DutyCycleListOfSims.txt
simulationTime=40


# 100000 - twtWakeIntervalMultiplier=1; videoQuality=1
simId=100000
nStations=1
scenario=wns3DutyCycle_case1
videoQuality=1
twtWakeIntervalMultiplier=1

for randSeed in {1000..1099}
do
    for dutyCycle in 0.005 0.01 0.015 0.02 0.025 0.03 0.04 0.05 0.06 0.07 0.08 0.09 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1
    do
        echo "wns3DutyCycle --simulationTime=${simulationTime} --simId=${simId} --scenario=${scenario} --randSeed=${randSeed}  --videoQuality=${videoQuality} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --dutyCycle=$dutyCycle --parallelSim=false" >> ${logFileName}
        echo "wns3DutyCycle --simulationTime=${simulationTime} --simId=${simId} --scenario=${scenario} --randSeed=${randSeed}  --videoQuality=${videoQuality} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --dutyCycle=$dutyCycle --parallelSim=false"
        ./ns3 run "wns3DutyCycle --simulationTime=${simulationTime} --simId=${simId} --scenario=${scenario} --randSeed=${randSeed}  --videoQuality=${videoQuality} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --dutyCycle=$dutyCycle --parallelSim=false" > logwns3DutyCycle_case1.out 2>&1
        simId=$(($simId+1))
    done
done

# 200000 - twtWakeIntervalMultiplier=0.5; videoQuality=1
simId=200000
nStations=1
scenario=wns3DutyCycle_case2
videoQuality=1
twtWakeIntervalMultiplier=0.5

for randSeed in {1000..1099}
do
    for dutyCycle in 0.005 0.01 0.015 0.02 0.025 0.03 0.04 0.05 0.06 0.07 0.08 0.09 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1
    do
        echo "wns3DutyCycle --simulationTime=${simulationTime} --simId=${simId} --scenario=${scenario} --randSeed=${randSeed}  --videoQuality=${videoQuality} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --dutyCycle=$dutyCycle --parallelSim=false" >> ${logFileName}
        echo "wns3DutyCycle --simulationTime=${simulationTime} --simId=${simId} --scenario=${scenario} --randSeed=${randSeed}  --videoQuality=${videoQuality} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --dutyCycle=$dutyCycle --parallelSim=false"
        ./ns3 run "wns3DutyCycle --simulationTime=${simulationTime} --simId=${simId} --scenario=${scenario} --randSeed=${randSeed}  --videoQuality=${videoQuality} --twtWakeIntervalMultiplier=${twtWakeIntervalMultiplier} --dutyCycle=$dutyCycle --parallelSim=false" > logwns3DutyCycle_case2.out 2>&1
        simId=$(($simId+1))
    done
done
