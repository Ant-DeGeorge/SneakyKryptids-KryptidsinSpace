# hUGEDriver

Music driver for hUGETracker songs, by SuperDisk and contributors: https://github.com/SuperDisk/hUGEDriver
(commit d6863d29). hUGETracker and hUGEDriver are dedicated to the public domain.

`src/hUGE/hUGEDriver.o` is prebuilt from these files, so the game builds with GBDK alone. To rebuild it
(needs RGBDS **0.6.x**: newer RGBDS object files are not accepted by rgb2sdas.py):

    rgbasm -i. -DGBDK -o hUGEDriver.obj hUGEDriver.asm
    python3 rgb2sdas.py -b 0 -o ../../src/hUGE/hUGEDriver.o hUGEDriver.obj
