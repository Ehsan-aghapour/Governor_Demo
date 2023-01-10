# Governor_Demo
This is a simple performance aware governor works with Pipe-All framework. In addition to frequency settings of CPU clusters, partitioning the 
CNN into pipeline stages and mapping stages into GPU, CPU big and CPU Little clusters in Pipe-All framework, affects the performance, power and 
energy consumption of the device. This basic governor explore design points and tries to find the design point that met the target FPS and latency
with minimum power consumption. It has implemented in two versoins C++ (Governor.cpp) and bash script (Governor.sh).

## Build the Governor
For cpp version, first you need to build the governor. 

### Linux device
If the target device is linux, compile it with g++ :
```
$ g++ -static-libstdc++ Governor.cpp -o Governor
$ chmod +x Governor
```

### Android device
If the target device is android you need to cross compile it using clang++ in linux and then push the binary into the android device:
```
$ arm-linux-androideabi-clang++ -static-libstdc++ Governor.cpp -o Governor
$ adb push Governor dir_inside_bord/
```
The *Build_CPP.sh* script sets the path, compiles the governor and push it into the android device:
```
$ ./Build_CPP.sh Governor.cpp
```
You need to define Android-NDK path in your machine into the *Android-NDK* variable and also change the path in your android device based on your desire. 


## Running the Governor
This governor takes four input arguments: target CNN (*Graph*), the number of total partitioning parts in this graph (*TotalParts*),
target FPS (*TargetFPS*), and target latency (*TargetLatency*) in the main function.
```
$ ./Governor Graph #TotalParts #TargetFPS #TargetLatency
```
