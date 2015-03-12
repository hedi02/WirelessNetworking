for round in `seq 5 10`;
do
   ../waf --run "wifi-simple-infra2 --n=$round"
  
  # mv "RX packets.pcap" "RxPackets_${round}.pcap"

done
