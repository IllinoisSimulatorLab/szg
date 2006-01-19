#!/bin/sh
ssh-add remote.private.key
cd /home/public/schaeffr/bin
rm -f linux.tar linux.tar.gz
echo Tar-ing linux executables...
tar -cf linux.tar ./linux
echo Compressing archive...
gzip linux.tar
echo Copying files...
scp linux.tar.gz $1:
rm linux.tar.gz
cd ~/remote
echo Copying unpacking script...
scp szg_remote.sh $1:scratch
echo Executing unpacking script...
ssh $1 /home/$2/scratch/szg_remote.sh




