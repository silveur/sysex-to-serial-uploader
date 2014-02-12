Sysex to Serial uploader
==================

This software takes an argument any sysex file and echo it to an serial output with a delay control between each sysex block.

Ideal to update the firmware of your device via sysex

Compatible with any unix system.
```
Arguments:

-f: Absolute path file input
-o: Serial output device
-p: Pause between sysex blocks(100ms)
-b: Baud rate(115200)
-v: Verbose mode
```
Example:
```
./SysexUploader -f /Users/Roli/dsp.syx -o /dev/tty.mxc1  -p 100 -b 115200
```