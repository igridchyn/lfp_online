#!/usr/bin/bash
i=0
grab-screenshot -w -d 10
while :
do
   #gnome-screenshot -w -f "ss/ss$i.png"&
   import -window pf ss/ss$i.png&
   sleep 0.05
   let i=i+1
   echo $i
done
