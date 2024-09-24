#!/bin/bash

#
# Scripts to install weathersense
#
if [ $# != 2 ]; then
    echo "Usage: install-weathersense.sh <directory>"
    exit 1
fi

dir=$1

version=`cat $dir/weathersense-version.txt`

mkdir /weathersense/$version

cp -r $dir/* /weathersense/$version

chown -R weathersense /weathersense/$version

cp $dir/rc/weathersense.rc /etc/init.d/weathersense
chmod +x /etc/init.d/weathersense

ln -s /etc/init.d/weathersense /etc/rc5.d/S99weathersense
ln -s /etc/init.d/weathersense /etc/rc5.d/K99weathersense

