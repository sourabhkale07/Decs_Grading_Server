#!/bin/bash

numberOfClients=$1
initialClients=$1
loopCount=$2
sleeptime=$3
time_out_sec=$4
ip=$5
port=$6
file_to_be_graded=$7

# Create a file to store the Throughput and response time values
throughput_file="throughput_data.txt"
response_time_file="response_time_data.txt"
goodput_file="goodput_data.txt"
timed_out_rate_file="timed_out_data.txt"
error_rate_file="error_rate_data.txt"

echo "Clients Throughput" > "$throughput_file"
echo "Clients AverageResponseTime" > "$response_time_file"
echo "Clients Goodput" > "$goodput_file"
echo "Clients Timed_out_rate" > "$timed_out_rate_file"
echo "Clients error_rate"> "$error_rate_file"

count=0;

mkdir -p performance_matrics_files

# Loop for varying the number of clients
for ((j=1; j<=20; j++)); do
    for ((i=1; i<=$numberOfClients; i++)); do
        otpt=$(./client $ip $port $file_to_be_graded $loopCount $sleeptime $time_out_sec| tail -1 &)
        f_name="performance_matrics_files/otpt_$j$i.txt"
        echo "$otpt" > "$f_name"
    done
    echo $count
    count=$(($count+1))
    # Increase the number of clients for the next iteration by a step size of 5
    numberOfClients=$(($numberOfClients + 5))
done

for ((j=1; j<=20; j++)); do
    th=0
    overallThroughput=0
    resTime=0
    res=0
    totalN=0
    gp=0
    overallGoodput=0

    individual_timed_out_rate=0
    overall_timed_out_rate=0

    individual_error_rate=0
    overall_error_rate=0

    for ((i=1; i<=$initialClients; i++)); do
        th=$(awk '{print $12}' "performance_matrics_files/otpt_$j$i.txt")
        overallThroughput=$(echo "$overallThroughput + $th" | bc -l)

        gp=$(awk '{print  $3}' "performance_matrics_files/otpt_$j$i.txt")
        overallGoodput=$(echo "$overallGoodput + $gp" | bc -l)

        individual_timed_out_rate=$(awk '{print $25}' "performance_matrics_files/otpt_$j$i.txt")
        overall_timed_out_rate=$(echo "$overall_timed_out_rate + $individual_timed_out_rate" | bc -l)

        individual_error_rate=$(awk '{print $22}' "performance_matrics_files/otpt_$j$i.txt")
        overall_error_rate=$(echo "$overall_error_rate + $individual_error_rate" | bc -l)

        n=$(awk '{print $3}' "performance_matrics_files/otpt_$j$i.txt")
        res=$(awk '{print $8}' "performance_matrics_files/otpt_$j$i.txt")
        resTime=$(echo "$res + $resTime" | bc -l)
        totalN=$((totalN + n))   
    done

    th=$(echo "scale=6; $overallThroughput" | bc -l)
    gp=$(echo "$overallGoodput")
    timed_out_rate=$(echo "$overall_timed_out_rate")
    error_rate=$(echo "$overall_error_rate")
    res=$(echo "scale=6; $resTime" | bc -l)

    # Append data to throughput file
    echo "$(($initialClients)) $th" >> "$throughput_file"
    echo "$(($initialClients)) $gp" >> "$goodput_file"
    echo "$(($initialClients)) $timed_out_rate" >> "$timed_out_rate_file"
    echo "$(($initialClients)) $error_rate" >> "$error_rate_file"

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
set ylabel "Average Response Time in microseconds"
set grid
plot "$response_time_file" using 1:2 with linespoints title "Average Response Time"
EOF

# Use Gnuplot to create the goodput graph
gnuplot <<EOF
set terminal png
set output "graphs/goodput_graph.png"
set title "Number of Clients vs. Goodput"
set xlabel "Number of Clients"
set ylabel "Goodput"
set grid
plot "$goodput_file" using 1:2 with linespoints title "Goodput"
EOF


# Use Gnuplot to create the timed_out graph
gnuplot <<EOF
set terminal png
set output "graphs/timed_out_graph.png"
set title "Number of Clients vs. timed out rate"
set xlabel "Number of Clients"
set ylabel "timed out rate"
set grid
plot "$timed_out_rate_file" using 1:2 with linespoints title "timed out rate"
EOF

# Use Gnuplot to create the timed_out graph
gnuplot <<EOF
set terminal png
set output "graphs/error_rate_graph.png"
set title "Number of Clients vs. error rate"
set xlabel "Number of Clients"
set ylabel "error rate"
set grid
plot "$error_rate_file" using 1:2 with linespoints title "error rate"
EOF

# echo "Graphs created: throughput_graph.png, response_time_graph.png and  goodput_graph.png error_rate_graph.png"

