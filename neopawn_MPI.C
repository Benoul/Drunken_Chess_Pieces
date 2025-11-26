#include <mpi.h>
#include <vector>
#include <tuple>
#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>


// How do we speed this up?

// let's run multiple replicas across processes
// each process runs its own independent simulations

// how do I compile it?
// mpic++ -o neopawn_MPI neopawn_MPI.C -std=c++17

// to run with 4 processes, N M max_steps num_simulations
// mpirun -np 4 ./neopawn_MPI 15 10 2000 10

// also note that previously the code only ran once
// this code runs multiple copies at the same
// effectively doing what the bash script did before
// hence the corresponding bash script for this is different...

using namespace std;

// Returns: (next_pos, move_was_jump, trap_status)
tuple<pair<int, int>, bool, bool> get_next_move(
    pair<int, int> current_pos,
    vector<vector<int>>& simulation_lattice,
    int N,
    const vector<pair<int, int>>& possible_moves,
    mt19937& rng)
{
    int current_r = current_pos.first;
    int current_c = current_pos.second;
    
    vector<pair<int, int>> diagonal_moves_only = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    bool trap_status = false;
    
    vector<pair<int, int>> occupied_diagonal_targets;
    for (const auto& [dr, dc] : diagonal_moves_only) {
        int next_r = current_r + dr;
        int next_c = current_c + dc;
        
        if (next_r >= 0 && next_r < N && next_c >= 0 && next_c < N) {
            if (simulation_lattice[next_r][next_c] == 1) {
                occupied_diagonal_targets.push_back({next_r, next_c});
            }
        }
    }
    
    pair<int, int> next_pos;
    bool move_was_jump = false;
    
    if (!occupied_diagonal_targets.empty()) {
        uniform_int_distribution<int> dist(0, occupied_diagonal_targets.size() - 1);
        next_pos = occupied_diagonal_targets[dist(rng)];
        move_was_jump = true;
    } else {
        vector<pair<int, int>> all_valid_targets;
        for (const auto& [dr, dc] : possible_moves) {
            int next_r = current_r + dr;
            int next_c = current_c + dc;
            
            if (next_r >= 0 && next_r < N && next_c >= 0 && next_c < N) {
                if (simulation_lattice[next_r][next_c] == 0) {
                    all_valid_targets.push_back({next_r, next_c});
                }
            }
        }
        
        if (all_valid_targets.empty()) {
            // The poor walker is trapped!
            return {current_pos, false, true};
        }
        
        uniform_int_distribution<int> dist(0, all_valid_targets.size() - 1);
        next_pos = all_valid_targets[dist(rng)];
        move_was_jump = false;
    }
    
    return {next_pos, move_was_jump, trap_status};
}

// Returns: (num_steps, failure_to_finish)
pair<int, bool> time_finder(int N, int M, int max_steps, int seed) {
    // note that the seed is now an input 
    // as opposed to using a random number as the seed...
    mt19937 rng(seed);
    
    pair<int, int> start_pos = {0, 0};
    vector<pair<int, int>> possible_moves = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    
    // Initialize lattice with zeros
    vector<vector<int>> lattice(N, vector<int>(N, 0));
    
    // Randomly place M obstacles
    vector<int> flat_indices(N * N);
    for (int i = 0; i < N * N; ++i) {
        flat_indices[i] = i;
    }
    shuffle(flat_indices.begin(), flat_indices.end(), rng);
    
    for (int i = 0; i < M; ++i) {
        int idx = flat_indices[i];
        int row = idx / N;
        int col = idx % N;
        lattice[row][col] = 1;
    }
    
    // copy for simulation
    vector<vector<int>> simulation_lattice = lattice;
    
    pair<int, int> current_pos = start_pos;
    int m_counter = M;
    int num_traps = 0;
    bool failure_to_finish = false;
    
    for (int step = 0; step < max_steps; ++step) {
        auto [next_pos, move_was_jump, trap_status] = get_next_move(
            current_pos, simulation_lattice, N, possible_moves, rng);
        
        if (trap_status) {
            // Walker is trapped, stay at current position
            num_traps++;
        } else {
            current_pos = next_pos;
        }
        
        if (move_was_jump) {
            simulation_lattice[next_pos.first][next_pos.second] = 0;
            m_counter--;
        }
        
        if (m_counter == 0) {
            return {step + num_traps, failure_to_finish};
        }
    }
    
    if (m_counter > 0) {
        failure_to_finish = true;
    }
    
    return {max_steps, failure_to_finish};
}

int main(int argc, char* argv[]) {
    // Initialize MPI
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // default numbers, note now we also have total_simulations
    int N = 10;
    int M = 5;
    int max_steps = 1000;
    int total_simulations = 100;
    
    // command-line arguments
    if (argc > 1) N = atoi(argv[1]);
    if (argc > 2) M = atoi(argv[2]);
    if (argc > 3) max_steps = atoi(argv[3]);
    if (argc > 4) total_simulations = atoi(argv[4]);

    int sims_per_process = total_simulations / size;
    int remainder = total_simulations % size;
    int my_num_sims = sims_per_process + (rank < remainder ? 1 : 0);

    if (rank == 0) {
        cout << "N: " << N << endl;
        cout << "M: " << M << endl;
        cout << "Max steps: " << max_steps << endl;
        cout << "Total simulations: " << total_simulations << endl;
        cout << "MPI processes: " << size << endl;
    }

    vector<int> local_steps;
    int local_failures = 0;
    
    random_device rd;
    int base_seed = rd();

    for (int i = 0; i < my_num_sims; ++i) {
        // unique seed for each simulation
        // this is stochastic process bro
        int seed = base_seed + rank * 10000 + i;
        
        auto [steps, failed] = time_finder(N, M, max_steps, seed);
        
        local_steps.push_back(steps);
        if (failed) local_failures++;
    }

    // Calculate local statistics
    double local_sum = accumulate(local_steps.begin(), local_steps.end(), 0.0);
    double local_mean = my_num_sims > 0 ? local_sum / my_num_sims : 0.0;

    // Gather results to rank 0
    vector<int> all_steps;
    vector<int> recvcounts(size);
    vector<int> displs(size);

    // Gather the number of simulations each process ran
    MPI_Gather(&my_num_sims, 1, MPI_INT, recvcounts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        // Calculate displacements for Gatherv
        displs[0] = 0;
        for (int i = 1; i < size; ++i) {
            displs[i] = displs[i-1] + recvcounts[i-1];
        }
        all_steps.resize(total_simulations);
    }
    
    // Gather all step counts to rank 0
    int* send_ptr = my_num_sims > 0 ? local_steps.data() : nullptr;
    int* recv_ptr = rank == 0 ? all_steps.data() : nullptr;
    int* recv_counts = rank == 0 ? recvcounts.data() : nullptr;
    int* recv_displs = rank == 0 ? displs.data() : nullptr;

    MPI_Gatherv(send_ptr, my_num_sims, MPI_INT,
            recv_ptr, recv_counts, recv_displs, MPI_INT,
            0, MPI_COMM_WORLD);
    
    // Gather failure counts
    int total_failures = 0;
    MPI_Reduce(&local_failures, &total_failures, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);


    // Rank 0 computes and displays final statistics
    if (rank == 0) {
        double global_mean = accumulate(all_steps.begin(), all_steps.end(), 0.0) / total_simulations;
        
        // standard deviation
        double sq_sum = 0.0;
        for (int steps : all_steps) {
            sq_sum += (steps - global_mean) * (steps - global_mean);
        }
        double std_dev = sqrt(sq_sum / total_simulations);
        
        cout << "\nFinal Results" << endl;
        cout << "Mean steps: " << global_mean << endl;
        cout << "Std deviation: " << std_dev << endl;
        cout << "Failures: " << total_failures << endl;

    }


    return 0;
}
