set terminal pdf
set output "cwExpt.pdf"
set title "Varying CW sizes"

set xlabel 'cwMin'
set ylabel 'Throughput (Mbps)'
set key outside
plot "-"  title "cwMax : 64" with lines, "-"  title "cwMax : 128" with lines, "-"  title "cwMax : 256" with lines
16 0.352373
32 0.379721
64 0.385253
e
16 0.358
32 0.378781
64 0.384104
e
16 0.357475
32 0.371292
64 0.386107
e
