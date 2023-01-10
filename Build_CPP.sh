# The path of Android-NDK in your machine
Android-NDK=/home/ehsan/UvA/ARMCL/android-ndk-r21e-linux-x86_64/
Bin_Dir=${Android-NDK}/android-ndk-r21e/toolchains/llvm/prebuilt/linux-x86_64/bin/
export PATH="${Bin_Dir}:$PATH"

Target_Compiler=armv7a-linux-androideabi23-clang++
Compiler=arm-linux-androideabi-clang++
cp ${Bin_Dir}/${Target_Compiler} ${Bin_Dir}/${Compiler}



$Compiler -static-libstdc++ $1 -o governor
 


adb push governor /data/local/Working_dir/
adb shell chmod +x /data/local/Working_dir/governor
adb shell /data/local/Working_dir/governor


