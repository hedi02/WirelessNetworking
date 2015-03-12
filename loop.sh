rm -f cwnd
#$random=1 + $[ RANDOM % 10 ]
#mv -f cwnd bck/cwnd
for round in `seq 1 $1`;
do
 for iteration in `seq 1 10`;
 do
  ../waf --run "wifi-simple-infra2 --n=$round" >> cwnd  
 done
done
