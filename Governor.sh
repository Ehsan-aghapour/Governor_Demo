#Instructions to Run
# On Your Computer: adb push Governor.sh /data/local/Working_dir
# On the Board: chmod +x Governor.sh && ./Governor.sh graph_alexnet_all_pipe_sync #NumberOFPartitions #TargetFPS #TargetLatency


graph=$1
partitions=$2
Target_FPS=$3
Target_Latency=$4

LittleFrequencyTable=(500000 667000 1000000 1200000 1398000 1512000 1608000 1704000 1800000)
BigFrequencyTable=(500000 667000 1000000 1200000 1398000 1512000 1608000 1704000 1800000 1908000 2016000 2100000 2208000)

LittleFrequencyCounter=0
BigFrequencyCounter=0

MaxLittleFrequencyCounter=8
MaxBigFrequencyCounter=12


######## Export OpenCL library path #############
export LD_LIBRARY_PATH=/data/local/Working_dir

######## Setup Performance Governor (CPU) ##############
echo performance > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
echo performance > /sys/devices/system/cpu/cpufreq/policy2/scaling_governor

######## Initialize Little and Big CPU with Lowest Frequency ########
echo ${LittleFrequencyTable[0]} > /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq
echo ${BigFrequencyTable[0]} > /sys/devices/system/cpu/cpufreq/policy2/scaling_max_freq



LatencyCondition=0
FPSCondition=0


StageOneInferenceTime=0
StageTwoInferenceTime=0
StageThreeInferenceTime=0


######## Get feedback by parsing the results ################
######## Return 1 if constaraints on FPS and Latency are satisfied ################

ParseResults(){
	
	LatencyCondition=0
	FPSCondition=0
		
	FPS=`awk -F'[ :]' '/Frame rate is:/{FPS=$(NF-1)} /Test passed/{print FPS}' OFS=, output.txt`
	FPS=$( printf "%0.f" $FPS )
	
	
	Latency=`awk -F'[ :]' '/Frame latency is:/{Latency=$(NF-1)} /Test passed/{print Latency}' OFS=, output.txt`
	Latency=$( printf "%0.f" $Latency )
	
	printf "\n"
	printf "Throughput is: %s\n" "$FPS"
	printf "Latency is: %s\n" "$Latency"
	
	StageOneInferenceTime=`awk -F'[ :]' '/stage1_inference_time:/{t=$(NF-1)} /Test passed/{print t}' OFS=, output.txt`
	StageOneInferenceTime=$( printf "%0.f" $StageOneInferenceTime)
	
	
	StageTwoInferenceTime=`awk -F'[ :]' '/stage2_inference_time:/{t=$(NF-1)} /Test passed/{print t}' OFS=, output.txt`
	StageTwoInferenceTime=$( printf "%0.f" $StageTwoInferenceTime)
	
	
	StageThreeInferenceTime=`awk -F'[ :]' '/stage3_inference_time:/{t=$(NF-1)} /Test passed/{print t}' OFS=, output.txt`
	StageThreeInferenceTime=$( printf "%0.f" $StageThreeInferenceTime)
	
	
	
	if [ $Latency -le $Target_Latency ]; then
	    LatencyCondition=1; #Latency requirement was met.
	fi

	if [ $FPS -ge $Target_FPS ]; then
	    FPSCondition=1; #FPS requirement was met.
	fi

	
}




N_Frames=10
######## Start with running half network on Little CPU and half network on Big CPU with GPU empty in the middle ###########

((PartitionPoint1=partitions/2))
((PartitionPoint2=partitions/2))
Order="L-G-B"
while : ; do
	
	./$graph --threads=4  --threads2=2  --target=NEON --n=$N_Frames --partition_point=$PartitionPoint1 --partition_point2=$PartitionPoint2 --order=$Order > output.txt
	ParseResults
	if [ "$FPSCondition" = 1 ] && [ "$LatencyCondition" = 1 ] #Both Latency and Throughput Requirements are Met.
	then
		printf "Solution Was Found.\n TargetBigFrequency:%s \t TargetLittleFrequency:%s \t PartitionPoint1:%s \t PartitionPoint2:%s \t Order:%s \n" "${BigFrequencyTable[$BigFrequencyCounter]}" "${LittleFrequencyTable[$LittleFrequencyCounter]}" "${PartitionPoint1}" "${PartitionPoint2}" "$Order"
		break;	
	fi	
	
	printf "Target Perfromance Not Satisfied\n\n"

	if [ LittleFrequencyCounter -lt MaxLittleFrequencyCounter ];
	then
		#Push Frequency of Little Cluster Higher to Meet Target Performance
		((LittleFrequencyCounter=LittleFrequencyCounter+1))
		echo ${LittleFrequencyTable[$LittleFrequencyCounter]} > /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq
		echo "Increasing Frequency of Little Cores to ${LittleFrequencyTable[$LittleFrequencyCounter]}"
	else
		if [ BigFrequencyCounter -lt MaxBigFrequencyCounter ];
		then
			#Push Frequency of Small Cluster Higher to Meet Target Performance
			((BigFrequencyCounter=BigFrequencyCounter+1))
			echo ${BigFrequencyTable[$BigFrequencyCounter]} > /sys/devices/system/cpu/cpufreq/policy2/scaling_max_freq
			echo "Increasing Frequency of Big Cores to ${BigFrequencyTable[$BigFrequencyCounter]}"
		else
			if [ StageOneInferenceTime -lt StageThreeInferenceTime ];
			then
				if [ PartitionPoint2 -lt partitions ];
				then
					#Push Layers from Third Stage (Big CPU) to GPU to Meet Target Performance
					((PartitionPoint2=PartitionPoint2+1))
					echo "Reducing the Size of Pipeline Partition 3"
				else
					echo "No Solution Found"
					break
				fi
				
			else
				if [ PartitionPoint1 -gt 1 ];
				then
					#Push Layers from First Stage (Little CPU) to GPU to Meet Target Performance
					((PartitionPoint1=PartitionPoint1-1))
					echo "Reducing the Size of Pipeline Partition 1"
				else
					echo "No Solution Found"
					break
				fi
			fi		
		fi
	fi
	
done







