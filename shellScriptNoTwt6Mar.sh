# sta count = 10
# pkt/s = 10, 50, 100

mkdir -p MobicomTwtLogs		# Creates directory if not already present
randSeed=1
logFileName=scriptLogTwt5PerSPMar6.txt
packetsPerSecond=10



# UDP------------------------------------------

loopIndex=12000
# for packetsPerSecond in 10 50 100 200 300 400 500 600 700 800 900 1000
# for randSeed in 100 200 300 400 500 600 700 800 900 1000
for packetsPerSecond in 10 50 100 200 300 400 500 600 700 800 900 1000
do
    for randSeed in 200 300 400 500 600
    do
        echo "--loopIndex=${loopIndex} --randSeed=${randSeed} --packetsPerSecond=${packetsPerSecond} --simulationTime=70" >> ${logFileName}
        # ./ns3 --run "wifi6Udp --loopIndex=${loopIndex} --randSeed=${randSeed} --packetsPerSecond=${packetsPerSecond} --simulationTime=70"
        ./ns3 --run "wifi6Udp --loopIndex=${loopIndex} --randSeed=${randSeed} --packetsPerSecond=${packetsPerSecond} --simulationTime=70" >>logMar6Twt5PerSp.out
        loopIndex=$(($loopIndex+1))
        loopIndexDummy=$(($loopIndexDummy+1))
    done
done
