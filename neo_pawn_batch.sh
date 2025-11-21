#!/bin/bash

# Batch script to run walker simulation for multiple N and M combos

# Output CSV file
OUTPUT_FILE="simulation_results.csv"

# Create CSV header
echo "N,M,max_steps,steps_taken,failed_to_finish" > $OUTPUT_FILE

# Define ranges for N and M
N_VALUES=(10 20 30 40 50)
M_VALUES=(5 10 15 20 25)
MAX_STEPS=100000
NUM_RUNS=10  # Number of runs per combination for averaging

echo "-"
echo "\\"
echo "|"
echo "/"
echo "-"
echo "Results will be written to: $OUTPUT_FILE"
echo ""

# Loop through all combinations
for N in "${N_VALUES[@]}"; do
    for M in "${M_VALUES[@]}"; do
        # Skip if M > N*N (more obstacles than cells)
        max_obstacles=$((N * N))
        if [ $M -gt $max_obstacles ]; then
            echo "Skipping N=$N, M=$M (M > N*N)"
            continue
        fi
        
        echo "Running: N=$N, M=$M (${NUM_RUNS} runs)"
        
        # Run multiple times for this combination
        for run in $(seq 1 $NUM_RUNS); do
            # Run the simulation and capture output
            output=$(./neopawn1 $N $M $MAX_STEPS)
            
            # Extract steps and failed status from output
            steps=$(echo "$output" | grep "Steps taken:" | awk '{print $3}')
            failed=$(echo "$output" | grep "Failed to finish:" | awk '{print $4}')
            
            # Write to CSV
            echo "$N,$M,$MAX_STEPS,$steps,$failed" >> $OUTPUT_FILE
        done
    done
done

echo ""
echo "Results saved to: $OUTPUT_FILE"
echo "Total runs: $(tail -n +2 $OUTPUT_FILE | wc -l)"
