#/* 
# * Author: Shyam  <shyam1@gatech.edu>
#

# Before running everytime, check the following:
# 1. Check file name for output text file. Update date and trial number.
# 2. Check file name for loop log file. Update date and trial number.
# 3. Check loop parameters - two sets.

mkdir -p MU_logs		# Creates directory if not already present
mkdir -p script_logs		# Creates directory if not already present


outputFileName=script_logs/consoleOutputApr26_5StaPsm50100loadUDP.txt		# changeThis
logFileName=script_logs/scriptLogApr26_5StaPsm50100loadUDP.txt			# changeThis
loopIndex=90000		# changeThis
otherStaCount=5	# changeThis


CURRENTDATE=`date +"%Y-%m-%d %T"`
echo "New Script started at ${CURRENTDATE}" >> ${outputFileName}
echo "loopIndex, simulationTime, enablePSM_flag, enableUdpUplink, dataPeriod, otherStaCount, dataRatebps_other,enableMulticast, multicastInterval_ms, randSeed" >> ${logFileName}


# simulationTime=30
simulationTime=30
enablePSM_flag=true
enableUdpUplink=true
dataPeriod=1

enableMulticast=false
dataRatebps_other_start=1	# Cannot be 0
dataRatebps_other_increment=100000
dataRatebps_other_end=1300001
multicastInterval_ms=40000
loopIndexDummy=0
randSeed=1

# printf "\nLoop parameters:\n\tdataRatebps_other_start: ${dataRatebps_other_start}\n\tdataRatebps_other_increment: ${dataRatebps_other_increment}\n\tdataRatebps_other_end: ${dataRatebps_other_end}">> ${outputFileName}


# for multicastInterval_ms in 20000 1000 700 500 300 200 100

for dataRatebps_other in 6500000 13000000	# changeThis	
# for dataRatebps_other in $(seq ${dataRatebps_other_start} ${dataRatebps_other_increment} ${dataRatebps_other_end})	
do
	for randSeed in 123 234 345 456 567 678 789 890 312 423		
	do
		dataRatebps_other_full=$(printf "%.1f" $dataRatebps_other)
		# echo "Iteration = $(($loopIndexDummy+1)) ;multicastInterval_ms = ${multicastInterval_ms}, dataRatebps_other = ${dataRatebps_other}/${dataRatebps_other_end};"
		echo "${loopIndex}, ${simulationTime}, ${enablePSM_flag},  ${enableUdpUplink}, ${dataPeriod}, ${otherStaCount}, ${dataRatebps_other_full}, ${enableMulticast}, ${multicastInterval_ms}, ${randSeed}" >> ${logFileName}
		echo "--loopIndex=${loopIndex} --simulationTime=${simulationTime} --enablePSM_flag=${enablePSM_flag} --otherStaCount=${otherStaCount} --enableUdpUplink=${enableUdpUplink} --dataPeriod=${dataPeriod} --dataRatebps_other=${dataRatebps_other_full} --enableMulticast=${enableMulticast} --multicastInterval_ms=${multicastInterval_ms} --randSeed=${randSeed}"
		./ns3 --run "PSM_MU_UDP --loopIndex=${loopIndex} --simulationTime=${simulationTime} --enablePSM_flag=${enablePSM_flag} --otherStaCount=${otherStaCount} --enableUdpUplink=${enableUdpUplink} --dataPeriod=${dataPeriod} --dataRatebps_other=${dataRatebps_other_full} --enableMulticast=${enableMulticast} --multicastInterval_ms=${multicastInterval_ms} --randSeed=${randSeed}" >>${outputFileName}
		loopIndex=$(($loopIndex+1))
		loopIndexDummy=$(($loopIndexDummy+1))
	done
done

# done
CURRENTDATE2=`date +"%Y-%m-%d %T"`
printf "Script ended at ${CURRENTDATE2}\n">>${outputFileName}
printf "\nScript started at ${CURRENTDATE}\n"
printf "Script ended at ${CURRENTDATE2}\n"
echo "Completed"
