* Device information
- device name : /dev/stopwatch
- major number : 242

* Compile & Transfer to Board
- app directory
	$make
	$adb push app /data/local/tmp
- module directory
	$make
	$adb push stopwatch.ko /data/local/tmp

* Module Init & Remove / Application Run
- Target Board /data/local/tmp
	$insmod stopwatch.ko
	$mknod /dev/stopwatch c 242 0
	$./app
	$rmmod stopwatch
