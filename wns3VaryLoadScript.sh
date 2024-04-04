logFileName=wns3VaryLoadListOfSims.txt
simulationTime=40


# 110000 - useCase=dTwt1
simId=110000
nStations=1
scenario=wns3VaryLoad_dTwt1
videoQuality=1
useCase=dTwt1

for randSeed in {1000..1099}
do
    for nStations in 4 8 12 16 20 24 28 32
    do
        echo "wns3VaryLoad --simulationTime=${simulationTime} --simId=${simId} --nStations=${nStations} --scenario=${scenario} --randSeed=${randSeed} --useCase=${useCase}  --videoQuality=${videoQuality} --parallelSim=false" >> ${logFileName}
        echo "wns3VaryLoad --simulationTime=${simulationTime} --simId=${simId} --nStations=${nStations} --scenario=${scenario} --randSeed=${randSeed} --useCase=${useCase}  --videoQuality=${videoQuality} --parallelSim=false"
        ./ns3 run "wns3VaryLoad --simulationTime=${simulationTime} --simId=${simId} --nStations=${nStations} --scenario=${scenario} --randSeed=${randSeed} --useCase=${useCase}  --videoQuality=${videoQuality} --parallelSim=false" > logwns3VaryLoad_dTwt1.out 2>&1
        simId=$(($simId+1))
    done
done


# 120000 - useCase=dTwt2
simId=120000
nStations=1
scenario=wns3VaryLoad_dTwt2
videoQuality=1
useCase=dTwt2

for randSeed in {1000..1099}
do
    for nStations in 4 8 12 16 20 24 28 32
    do
        echo "wns3VaryLoad --simulationTime=${simulationTime} --simId=${simId} --nStations=${nStations} --scenario=${scenario} --randSeed=${randSeed} --useCase=${useCase}  --videoQuality=${videoQuality} --parallelSim=false" >> ${logFileName}
        echo "wns3VaryLoad --simulationTime=${simulationTime} --simId=${simId} --nStations=${nStations} --scenario=${scenario} --randSeed=${randSeed} --useCase=${useCase}  --videoQuality=${videoQuality} --parallelSim=false"
        ./ns3 run "wns3VaryLoad --simulationTime=${simulationTime} --simId=${simId} --nStations=${nStations} --scenario=${scenario} --randSeed=${randSeed} --useCase=${useCase}  --videoQuality=${videoQuality} --parallelSim=false" > logwns3VaryLoad_dTwt2.out 2>&1
        simId=$(($simId+1))
    done
done


# 130000 - useCase=dTwt3
simId=130000
nStations=1
scenario=wns3VaryLoad_dTwt3
videoQuality=1
useCase=dTwt3

for randSeed in {1000..1099}
do
    for nStations in 4 8 12 16 20 24 28 32
    do
        echo "wns3VaryLoad --simulationTime=${simulationTime} --simId=${simId} --nStations=${nStations} --scenario=${scenario} --randSeed=${randSeed} --useCase=${useCase}  --videoQuality=${videoQuality} --parallelSim=false" >> ${logFileName}
        echo "wns3VaryLoad --simulationTime=${simulationTime} --simId=${simId} --nStations=${nStations} --scenario=${scenario} --randSeed=${randSeed} --useCase=${useCase}  --videoQuality=${videoQuality} --parallelSim=false"
        ./ns3 run "wns3VaryLoad --simulationTime=${simulationTime} --simId=${simId} --nStations=${nStations} --scenario=${scenario} --randSeed=${randSeed} --useCase=${useCase}  --videoQuality=${videoQuality} --parallelSim=false" > logwns3VaryLoad_dTwt3.out 2>&1
        simId=$(($simId+1))
    done
done



# 140000 - useCase=cam
simId=140000
nStations=1
scenario=wns3VaryLoad_cam
videoQuality=1
useCase=cam

for randSeed in {1000..1099}
do
    for nStations in 4 8 12 16 20 24 28 32
    do
        echo "wns3VaryLoad --simulationTime=${simulationTime} --simId=${simId} --nStations=${nStations} --scenario=${scenario} --randSeed=${randSeed} --useCase=${useCase}  --videoQuality=${videoQuality} --parallelSim=false" >> ${logFileName}
        echo "wns3VaryLoad --simulationTime=${simulationTime} --simId=${simId} --nStations=${nStations} --scenario=${scenario} --randSeed=${randSeed} --useCase=${useCase}  --videoQuality=${videoQuality} --parallelSim=false"
        ./ns3 run "wns3VaryLoad --simulationTime=${simulationTime} --simId=${simId} --nStations=${nStations} --scenario=${scenario} --randSeed=${randSeed} --useCase=${useCase}  --videoQuality=${videoQuality} --parallelSim=false" > logwns3VaryLoad_cam.out 2>&1
        simId=$(($simId+1))
    done
done
