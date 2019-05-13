set terminal png enhanced size 1024,768
set datafile missing 'nan'



set output 'NxN.png'
set title 'Scaling #Nodes'
set xlabel 'n'
#set xrange [0:12]
set ylabel 'bandwidth[Gb/s] / Node'
#set yrange [21:23]

#dont try this at home!
#f1(x) = a*exp(b*x)
#fit f1(x) 'NxN_10GB.dat' using 3:11 via a,b
plot 'NxN_10GB.dat' using 3:11 title 'effective throughput',\ 
#        f1(x)

set output 'NxN3D.png'
set title 'individual bandwidth distribution'
set zlabel 'n'
#set xrange [0:12]
set ylabel 'bandwidth[Gb/s] / Node'
#set yrange [0:40]
set xlabel 'node id'

# plot 'NxN_10GB.dat' using ($3 ==1  ? $1 : 1/0):11 title '1x1',\
# 'NxN_10GB.dat' using ($3 ==2  ? $1 : 1/0):11 title '2x2',\
# 'NxN_10GB.dat' using ($3 ==3  ? $1 : 1/0):11 title '3x3',\
# 'NxN_10GB.dat' using ($3 ==4  ? $1 : 1/0):11 title '4x4',\
# 'NxN_10GB.dat' using ($3 ==6  ? $1 : 1/0):11 title '6x6',\
# 'NxN_10GB.dat' using ($3 ==8  ? $1 : 1/0):11 title '8x8',\
# 'NxN_10GB.dat' using ($3 ==10  ? $1 : 1/0):11 title '10x10',\





set output 'heatmap.png'
set title 'heatmap'
set xlabel 'Sender'
set ylabel 'Receiver'
#figure out id range [0:max_en]
#note: id: 1 != pn01 instead the id describes the list entry in the client and server list within the slurm startup script.
#this could probably also be achieved by using column 3/4
stats 'NxN_10GB.dat' using (column(1)) nooutput
max_en = STATS_max
num_en = max_en + 1
stats 'NxN_10GB.dat' using (column(2)) nooutput
max_pn = STATS_max
num_pn = max_pn + 1
print sprintf("num_en = %d", num_en)
print sprintf("num_pn = %d", num_pn)
# Color runs from white to green
# TODO: find something more appropriate
set palette rgbformula -7,2,-7
set cblabel sprintf("Score: 0 to %d", 32/num_en)
unset cbtics
# one node has a theoretical max of 32 Gbit with PCIe 2x8. Divide by number of connections
set cbrange [0:(32/num_en)]

#give the heatmap some space
set xrange [-2:num_en+1]
set yrange [-2:num_pn+1]

#filter out connection pair (i,j) and get mean over all entries. Store in array
array Avg[num_en * num_pn]
ind(a,b) = 1+a+b*num_pn
getavg(a,b) = Avg[ind(a,b)]
do for [i=0:max_en] {
    do for [j=0:max_pn] {
    index = ind(i,j)
    stats 'NxN_10GB.dat' using ($1 == i && $2 == j && $3 == 5 ? column(11) : 1/0) nooutput
    Avg[index] = STATS_mean
    }
}
#use helper.dat to properly iterate i,j pairs once.
plot 'NxN_helper.dat' using 1 : 2 : ($1<num_en && $2<num_pn ? Avg[1+$1+$2*num_pn] : 1/0) with image title "NxN Heatmap" , \
     'NxN_helper.dat' using 1 : 2 : ($1<num_en && $2<num_pn ? sprintf("%g",Avg[1+$1+$2*num_pn]) : "") with labels title "effective throughput per connection pair"
