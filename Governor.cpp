/*Instructions to Run
On Your Computer: 
	arm-linux-androideabi-clang++ -static-libstdc++ Governor.cpp -o Governor 
	adb push Governor /data/local/Working_dir
On the Board: 
	chmod +x Governor.sh
	./Governor graph_alexnet_all_pipe_sync #NumberOFPartitions #TargetFPS #TargetLatency
*/


#include <stdio.h>      /* printf */
#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

int LittleFrequencyTable[]={500000, 667000, 1000000, 1200000, 1398000, 1512000, 1608000, 1704000, 1800000};
int BigFrequencyTable[]={500000, 667000, 1000000, 1200000, 1398000, 1512000, 1608000, 1704000, 1800000, 1908000, 2016000, 2100000, 2208000};

int LittleFrequencyCounter=0;
int BigFrequencyCounter=0;

int MaxLittleFrequencyCounter=8;
int MaxBigFrequencyCounter=12;

bool LatencyCondition=0;
bool FPSCondition=0;


float StageOneInferenceTime=0;
float StageTwoInferenceTime=0;
float StageThreeInferenceTime=0;

int partitions=0;
int Target_FPS=0;
int Target_Latency=0;



/* Get feedback by parsing the results */
void ParseResults(){
	float FPS;
	float Latency;
	ifstream myfile("output.txt");
	cout<<endl;
	/* Read Output.txt File and Extract Data */
	for( std::string line; getline( myfile, line ); )
	{
		string temp;
		/* Extract Frame Rate */
		if ( line.find("Frame rate is:")==0 ){
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> FPS){
					printf("Throughput is: %f FPS\n", FPS);
					break;
				}
				temp = "";
			}
		}
		/* Extract Frame Latency */
		if ( line.find("Frame latency is:")==0 ){
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> Latency){
					printf("Latency is: %f ms\n", Latency);
					break;
				}
				temp = "";
			}
		}
		/* Extract Stage One Inference Time */
		if ( line.find("stage1_inference_time:")==0 ){
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> StageOneInferenceTime){
					//printf("StageOneInferenceTime is: %f ms\n", StageOneInferenceTime);
					break;
				}
				temp = "";
			}
		}
		/* Extract Stage Two Inference Time */
		if ( line.find("stage2_inference_time:")==0 ){
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> StageTwoInferenceTime){
					//printf("StageTwoInferenceTime is: %f ms\n", StageTwoInferenceTime);
					break;
				}
				temp = "";
			}
		}
		/* Extract Stage Three Inference Time */
		if ( line.find("stage3_inference_time:")==0 ){
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> StageThreeInferenceTime){
					//printf("StageThreeInferenceTime is: %f ms\n", StageThreeInferenceTime);
					break;
				}
				temp = "";
			}
		}
	}
	/* Check Throughput and Latency Constraints */
	if ( Latency <= Target_Latency ){
		LatencyCondition=1; //Latency requirement was met.
	}
	if ( FPS >= Target_FPS ){
		FPSCondition=1; //FPS requirement was met.
	}
}


int main (int argc, char *argv[])
{
	if ( argc < 5 ){
		std::cout<<"Wrong number of input arguments.\n";
		return -1;
	}
	string graph=argv[1];
	partitions=atoi(argv[2]);
	Target_FPS=atoi(argv[3]);
	Target_Latency=atoi(argv[4]);

	string Command="";

	/* Checking if processor is available */
	if (system(NULL)) puts ("Ok");
	else exit (EXIT_FAILURE);


	/* Export OpenCL library path */
	system("export LD_LIBRARY_PATH=/data/local/Working_dir");
	setenv("LD_LIBRARY_PATH","/data/local/Working_dir",1);

	/* Setup Performance Governor (CPU) */
	system("echo performance > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor");
	system("echo performance > /sys/devices/system/cpu/cpufreq/policy2/scaling_governor");

	/* Initialize Little and Big CPU with Lowest Frequency */
	Command="echo " + to_string(LittleFrequencyTable[0]) + " > /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq";
	system(Command.c_str());
	Command="echo " + to_string(BigFrequencyTable[0]) + " > /sys/devices/system/cpu/cpufreq/policy2/scaling_max_freq";	
	system(Command.c_str());

	int N_Frames=10;
	/* Start with running half network on Little CPU and half network on Big CPU with GPU empty in the middle */
	int PartitionPoint1=partitions/2;
	int PartitionPoint2=partitions/2;
	string Order="L-G-B";
	while(true){
		char Run_Command[150];		
		sprintf(Run_Command,"./%s --threads=4 --threads2=2 --target=NEON --n=%d --partition_point=%d --partition_point2=%d --order=%s > output.txt",
		graph.c_str(), N_Frames, PartitionPoint1, PartitionPoint2, Order.c_str());
		system(Run_Command);
		ParseResults();
		if ( FPSCondition && LatencyCondition ){//Both Latency and Throughput Requirements are Met.
			printf("Solution Was Found.\n TargetBigFrequency:%d \t TargetLittleFrequency:%d \t PartitionPoint1:%d \t PartitionPoint2:%d \t Order:%s\n", 
			BigFrequencyTable[BigFrequencyCounter],LittleFrequencyTable[LittleFrequencyCounter], PartitionPoint1, PartitionPoint2, Order.c_str());
			break;	
		}

		printf("Target Perfromance Not Satisfied\n\n");

		if ( LittleFrequencyCounter < MaxLittleFrequencyCounter ){
			/* Push Frequency of Little Cluster Higher to Meet Target Performance */
			LittleFrequencyCounter=LittleFrequencyCounter+1;
			Command="echo " + to_string(LittleFrequencyTable[LittleFrequencyCounter]) + " > /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq";
			system(Command.c_str());
			printf("Increasing Frequency of Little Cores to %d\n", LittleFrequencyTable[LittleFrequencyCounter]);
		}
		else{
			if ( BigFrequencyCounter < MaxBigFrequencyCounter ){
				/* Push Frequency of Small Cluster Higher to Meet Target Performance */
				BigFrequencyCounter=BigFrequencyCounter+1;
				Command="echo " + to_string(BigFrequencyTable[BigFrequencyCounter]) + " > /sys/devices/system/cpu/cpufreq/policy2/scaling_max_freq";
				system(Command.c_str());
				printf("Increasing Frequency of Big Cores to %d\n", BigFrequencyTable[BigFrequencyCounter]);
			}
			else{
				if ( StageOneInferenceTime < StageThreeInferenceTime ){
					if ( PartitionPoint2 < partitions ){
						/* Push Layers from Third Stage (Big CPU) to GPU to Meet Target Performance */
						PartitionPoint2=PartitionPoint2+1;
						printf("Reducing the Size of Pipeline Partition 3\n");
					}
					else{
						printf("No Solution Found\n");
						break;
					}
				}
				else{
					if ( PartitionPoint1 > 1 ){
						/* Push Layers from First Stage (Little CPU) to GPU to Meet Target Performance */
						PartitionPoint1=PartitionPoint1-1;
						printf("Reducing the Size of Pipeline Partition 1\n");
					}
					else{
						printf("No Solution Found\n");
						break;
					}
				}
			}
		}
	}
  
  return 0;
}
