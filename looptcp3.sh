# usage: sh looptcp3.sh 1 50

tempfile=../log/looptcp3.$(date +%m%d%Y_%H%M%S)

for rts in 150 2000
do
 for node in `seq $1 $2`;	#
 do
  for iteration in `seq 1 10`;	#number of iteration
  do
   ../waf --run "wifitcpm3 --n=$node --rtsCts=$rts" >> $tempfile
  done
 done
done
