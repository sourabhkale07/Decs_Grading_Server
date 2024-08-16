#!/bin/bash
numberOfClients=$1
initialClients=$1
loopCount=$2
sleeptime=$3
ip=$4
port=$5
file_to_be_graded=$6

# Create a file to store the Throughput and response time values
throughput_file="throughput_data.txt"
response_time_file="response_time_data.txt"
echo "Clients Throughput AverageResponseTime" > "$throughput_file"
echo "Clients AverageResponseTime" > "$response_time_file"

count=0;

mkdir -p performance_matrics_files

# Loop for varying the number of clients
for ((j=1; j<=20; j++)); do
    for ((i=1; i<=$numberOfClients; i++)); do
        otpt=$(./client $ip $port $file_to_be_graded $loopCount $sleeptime | tail -1)
        f_name="performance_matrics_files/otpt_$j$i.txt"
        echo "$otpt" > "$f_name"
    done
    count=$(($count+1))
    echo $count
    # Increase the number of clients for the next iteration by a step size of 5
    numberOfClients=$(($numberOfClients + 5))
done

for ((j=1; j<=20; j++)); do
    th=0
    overallThroughput=0
    resTime=0
    res=0
    totalN=0

    for ((i=1; i<=$initialClients; i++)); do
        th=$(awk '{print $12}' "performance_matrics_files/otpt_$j$i.txt")
        overallThroughput=$(echo "$overallThroughput + $th" | bc -l)
        n=$(awk '{print $3}' "performance_matrics_files/otpt_$j$i.txt")
        res=$(awk '{print $8}' "performance_matrics_files/otpt_$j$i.txt")
        resTime=$(echo "$res + $resTime" | bc -l)
        totalN=$((totalN + n))   
    done
    th=$(echo "scale=6; $overallThroughput" | bc -l)
    res=$(echo "scale=6; $resTime" | bc -l)
    # Append data to throughput file
    echo "$(($initialClients)) $th" >> "$throughput_file"
    # Append data to response time file
    echo "$(($initialClients)) $res" >> "$response_time_file"
    initialClients=$((initialClients + 5))
done

mkdir -p graphs

# Use Gnuplot to create the throughput graph
gnuplot <<EOF
set terminal png
set output "graphs/throughput_graph.png"
set title "Number of Clients vs. Throughput"
set xlabel "Number of Clients"
set ylabel "Throughput"
set grid
plot "$throughput_file" using 1:2 with linespoints title "Throughput"
EOF

# Use Gnuplot to create the response time graph
gnuplot <<EOF
set terminal png
set output "graphs/response_time_graph.png"
set title "Number of Clients vs. Average Response Time"
set xlabel "Number of Clients"
set ylabel "Average Response Time (in microseconds)"
set grid
plot "$response_time_file" using 1:2 with linespoints title "Average Response Time"
EOF

echo "Graphs created: throughput_graph.png and response_time_graph.png"

