cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build

# clear logs
adb logcat -c
# read logs
adb logcat | rg -n "SDL/APP|F libc| SDL| libapp"
