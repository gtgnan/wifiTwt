#/* 
# * Author: Shyam  <shyam1@gatech.edu>
#

# Before running everytime, check the following:
# 1. Check file name for output text file. Update date and trial number.
# 2. Check file name for loop log file. Update date and trial number.
# 3. Check loop parameters - two sets.

mkdir -p twtLogs		# Creates directory if not already present
mkdir -p script_logs		# Creates directory if not already present


outputFileName=script_logs/psmMultiSta100.txt		# changeThis
# logFileName=script_logs/scriptLogApr26_5StaPsm50100loadTcp.txt			# changeThis
loopIndex=10501		# changeThis
nStations=100		# changeThis



# enableMulticast=false
# dataRatebps_other_start=1	# Cannot be 0
# dataRatebps_other_increment=100000
# dataRatebps_other_end=1300001
# multicastInterval_ms=40000
loopIndexDummy=0
randSeed=1


# for dataRatebps_other in 6500000 13000000 	# changeThis	
# # for dataRatebps_other in $(seq ${dataRatebps_other_start} ${dataRatebps_other_increment} ${dataRatebps_other_end})	
# do
for randSeed in 100
do
	echo "--loopIndex=${loopIndex} --nStations=${nStations} --randSeed=${randSeed}"
	./ns3 --run "wifi6Net --loopIndex=${loopIndex} --nStations=${nStations} --randSeed=${randSeed}" >>${outputFileName}
	# ./ns3 --run "PSM_MU --loopIndex=${loopIndex} --simulationTime=${simulationTime} --enablePSM_flag=${enablePSM_flag} --otherStaCount=${otherStaCount} --enableTcpUplink=${enableTcpUplink} --dataPeriod=${dataPeriod} --dataRatebps_other=${dataRatebps_other_full} --enableMulticast=${enableMulticast} --multicastInterval_ms=${multicastInterval_ms} --randSeed=${randSeed}" >>${outputFileName}
	# ./ns3 --run "collisionEval --loopIndex=${loopIndex} --simulationTime=${simulationTime} --enablePSM_flag=${enablePSM_flag} --otherStaCount=${otherStaCount} --enableTcpUplink=${enableTcpUplink} --dataPeriod=${dataPeriod} --dataRatebps_other=${dataRatebps_other_full} --enableMulticast=${enableMulticast} --multicastInterval_ms=${multicastInterval_ms} --randSeed=${randSeed}" >>${outputFileName}
	loopIndex=$(($loopIndex+1))
	loopIndexDummy=$(($loopIndexDummy+1))
done
# done

# done
CURRENTDATE2=`date +"%Y-%m-%d %T"`
printf "Script ended at ${CURRENTDATE2}\n">>${outputFileName}
printf "\nScript started at ${CURRENTDATE}\n"
printf "Script ended at ${CURRENTDATE2}\n"
echo "Completed"
