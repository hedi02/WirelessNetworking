set terminal png
set output "wifi.png"
set title "Number of Wifi Nodes vs Throughput"
set xlabel "Number of nodes"
set ylabel "Throughput (Mbps)"

set key outside center below
set datafile missing "-nan"
set style line 1 lc rgb '#0060ad' lt 1 lw 2 pt 7 ps 1.5   # --- blue
plot 'cwnd' with linepoints, "cwnd" index 1 with linepoints
