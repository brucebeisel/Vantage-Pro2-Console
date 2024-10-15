#!/bin/bash

rootdir=/weathersense

#
# Scripts to install weathersense
#
if [ $# != 1 ]; then
    echo "Usage: install-weathersense.sh <directory>"
    exit 1
fi

dir=$1

version=`cat $dir/weathersense-version.txt`
echo Version: $version
installdir=$rootdir/$version

echo "Checking for weathersense user..."
id -u weathersense &> /dev/null

if [ $? == 1 ]; then
    echo "User \"weathersense\" must exist to install WeatherSense $version"
    echo "Aborting installation"
    exit 2
else
    echo "User weathersense exists"
fi

echo "Ensuring user \"weathersense\" is in group \"dialout\"..."
adduser -q weathersense dialout

if [ $? != 0 ]; then
    echo "Adding user weathersense to group dialout failed"
    echo "Aborting installation"
    exit 3
fi

echo "Checking node version..."
oldifs="$IFS"
IFS="."
read -ra nodeversion <<< `node --version | sed 's/^v//'`

if [[ $nodeversion < 18 ]]; then
    echo "WeatherSense require node version 18 or later. Installed version is `node --version`"
    echo "Aborting installation"
    exit 4
else
    echo "Node version $nodeversion confirmed"
fi

IFS="$oldifs"

echo "Checking that directory $rootdir already exists..."
if  ! [ -d $rootdir ]; then
    echo "Directory $rootdir must exist to install WeatherSense $version"
    echo "Aborting installation"
    exit 5
else
    echo "Directory $rootdir exists"
fi

echo Making installation directory $installdir
mkdir $installdir

if [ $? == 1 ]; then
    echo "Failed to make directory $installdir"
    echo "Aborting installation"
    exit 6
fi

sourcedirs="archive bin log node"

echo "Copying source directories..."
for sourcedir in $sourcedirs;
do
    echo "Copying source directory $sourcedir"
    cp -r $dir/$sourcedir $installdir
done

chown -R weathersense $installdir

echo "Copying files to /etc/init.d and /etc/rc5.d..."
cp $dir/rc/weathersense.rc /etc/init.d/weathersense
chmod +x /etc/init.d/weathersense

ln -s -f /etc/init.d/weathersense /etc/rc5.d/S99weathersense
ln -s -f /etc/init.d/weathersense /etc/rc5.d/K99weathersense

echo "Creating cron entry for data backup..."
crontab -u weathersense -e $dir/weathersense.cron

echo "Checking for old archive data..."
#archives=`ls -r $rootdir/*/archive`

#numarchives=`wc -w <<< $archives`

#copyarchive=""

#if [[ $numarchives -gt 0 ]]; then
#    echo "A number of existing archives were found in the $rootdir directory"
#    echo "Would you like to copy the existing archive files to the new installation?"
#    read -p "[yes/no]: " answer
#    if [ $answer != "yes" ]; then
#        exit 5
#    fi
#    echo "The choices are (newest first): "
#    cat <<< $archives
#    echo "Choose one of the following files by entering "y" at the prompt"
#
#    for archive in $archives
#    do
#        read -p "$archive " answer
#        if [ $answer == "y" ]; then
#            copyarchive=$archive
#            break
#        fi
#    done
#fi

#if [ $copyarchive != "" ]; then
#    cp -r $copyarchive/* $installdir/archive
#    cp $copyarchive/../node/VantageConsole/UserData/* $installdir/node/VantageConsole/UserData
#fi
