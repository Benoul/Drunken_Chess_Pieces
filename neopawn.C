#include <vector>
#include <tuple>
#include <random>
#include <algorithm>
#include <iostream>

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
pair<int, bool> time_finder(int N, int M, int max_steps = 1000) {
    random_device rd;
    mt19937 rng(rd());
    
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
    // default numbers
    int N = 10;
    int M = 5;
    int max_steps = 1000;
    
    // command-line arguments
    if (argc > 1) N = atoi(argv[1]);
    if (argc > 2) M = atoi(argv[2]);
    if (argc > 3) max_steps = atoi(argv[3]);
    
    cout << "N=" << N << ", M=" << M << ", max_steps=" << max_steps << endl;
    
    auto [steps, failed] = time_finder(N, M, max_steps);
    
    cout << "Steps taken: " << steps << endl;
    cout << "Failed to finish: " << (failed ? "true" : "false") << endl;
    
    return 0;
}
