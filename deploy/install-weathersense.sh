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

id -u weathersense &> /dev/null

if [ $? == 1 ]; then
    echo "User \"weathersense\" must exist to install WeatherSense $version"
    exit 2
fi

if [ -d /weathersense ]; then
    echo "Directory /weathersense must exist to install WeatherSense $version"
    exit 3
fi

mkdir /weathersense/$version

cp -r $dir/* /weathersense/$version

chown -R weathersense /weathersense/$version

cp $dir/rc/weathersense.rc /etc/init.d/weathersense
chmod +x /etc/init.d/weathersense

ln -s /etc/init.d/weathersense /etc/rc5.d/S99weathersense
ln -s /etc/init.d/weathersense /etc/rc5.d/K99weathersense

archives=`ls -r /weathersense/*/archive`

numarchives=`wc -w <<< $archives`

copyarchive=""

if [ $numarchives -gt 0 ]; then
    echo "A number of existing archives were found in the /weathersense directory"
    echo "Would you like to copy the existing archive files to the new installation?"
    read -p "[yes/no]: " answer
    if [ $answer != "yes" ]; then
        exit 4
    fi
    echo "The choices are (newest first): "
    cat <<< $archives
    echo "Choose one of the following files by entering "y" at the prompt"

    for archive in $archives
    do
        read -p "$archive " answer
        if [ $answer == "y" ]; then
            copyarchive=$archive
            break
        fi
    done
fi

if [ $copyarchive != "" ]; then
    cp -r $copyarchive/* /weathersense/$version/archive
fi
