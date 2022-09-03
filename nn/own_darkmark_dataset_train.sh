#!/bin/bash -e

cd /home/olokos/OCR_STUFF/own_darkmark_dataset

# Warning: this file is automatically created/updated by DarkMark v1.7.5-1!
# Created on Sat 2022-08-27 20:36:40 CEST by olokos@olokos-Ryzen-Ubu.

rm -f output.log
rm -f chart.png

echo "creating new log file" > output.log
date >> output.log

ts1=$(date)
ts2=$(date +%s)
echo "initial ts1: ${ts1}" >> output.log
echo "initial ts2: ${ts2}" >> output.log
echo "cmd: /home/olokos/OCR_STUFF/darknet_source/darknet/darknet detector -map -dont_show train /home/olokos/OCR_STUFF/own_darkmark_dataset/own_darkmark_dataset.data /home/olokos/OCR_STUFF/own_darkmark_dataset/own_darkmark_dataset.cfg" >> output.log

/usr/bin/time --verbose /home/olokos/OCR_STUFF/darknet_source/darknet/darknet detector -map -dont_show train /home/olokos/OCR_STUFF/own_darkmark_dataset/own_darkmark_dataset.data /home/olokos/OCR_STUFF/own_darkmark_dataset/own_darkmark_dataset.cfg 2>&1 | tee --append output.log

ts3=$(date)
ts4=$(date +%s)
echo "ts1: ${ts1}" >> output.log
echo "ts2: ${ts2}" >> output.log
echo "ts3: ${ts3}" >> output.log
echo "ts4: ${ts4}" >> output.log

find /home/olokos/OCR_STUFF/own_darkmark_dataset -maxdepth 1 -regex ".+_[0-9]+\.weights" -print -delete >> output.log

