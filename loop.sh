rm -f cwnd
#$random=1 + $[ RANDOM % 10 ]
#mv -f cwnd bck/cwnd
for rts in 50 5000
do
 for node in `seq $1 $2`;
 do
  for iteration in `seq 1`;
  do
   ../waf --run "wifi-simple-infra2 --n=$node --rtsCts=$rts" >> cwnd  
  done
 done
done
