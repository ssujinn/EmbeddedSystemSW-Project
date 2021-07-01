* device info
driver name : /dev/dev_driver
major number : 242

* compile (both app and module)
$ make

* transfer to board
(in app dir)
adb push app /data/local/tmp
(in module dir)
adb push dev_driver.ko /data/local/tmp

* test (in board /data/local/tmp)
$ insmod dev_driver.ko
$ mknod /dev/dev_driver c 242 0
$ ./app TIMER_INTERVAL[1-100] TIMER_CNT[1-100] TIMER_INIT[0001-8000]
