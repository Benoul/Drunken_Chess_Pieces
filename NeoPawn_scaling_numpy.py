import numpy as np
import matplotlib.pyplot as plt
import copy
import random

def get_next_move(current_pos, simulation_lattice, N, possible_moves):
    current_r, current_c = current_pos
    diagonal_moves_only = [(-1, -1), (-1, 1), (1, -1), (1, 1)]

    #to make sure we can return if trapped
    trap_status = False

    occupied_diagonal_targets = []
    for dr, dc in diagonal_moves_only:
        next_r, next_c = current_r + dr, current_c + dc

        if 0 <= next_r < N and 0 <= next_c < N:

            if simulation_lattice[next_r, next_c] == 1:
                occupied_diagonal_targets.append((next_r, next_c))

    next_pos = None
    move_was_jump = False

    if occupied_diagonal_targets:
        next_pos = random.choice(occupied_diagonal_targets)
        move_was_jump = True
    else:
        all_valid_targets = []
        for dr, dc in possible_moves:
            next_r, next_c = current_r + dr, current_c + dc
            if 0 <= next_r < N and 0 <= next_c < N:
                if simulation_lattice[next_r, next_c] == 0:
                    all_valid_targets.append((next_r, next_c))
        if not all_valid_targets:
            #The poor walker is trapped!
            return current_pos, False, True

        next_pos = random.choice(all_valid_targets)
        move_was_jump = False
    return next_pos, move_was_jump, trap_status


def time_finder(N, M, max_steps=1000):
    start_pos = (0, 0)
    num_steps = max_steps
    possible_moves = [(-1, 0), (1, 0), (0, -1), (0, 1)]
    lattice = np.zeros((N, N), dtype=int)
    flat_indices = np.random.choice(N*N, M, replace=False)
    rows = flat_indices // N
    cols = flat_indices % N
    lattice[rows, cols] = 1
    simulation_lattice = copy.deepcopy(lattice)

    current_pos = start_pos
    m_counter = M

    for step in range(num_steps):
        #how often it gets trapped...
        num_traps = 0
        next_pos, move_was_jump, trap_status = get_next_move(current_pos, simulation_lattice, N, possible_moves)
        if trap_status:
            # if the walker gets trapped then we will return it to the previous position and continue
            # note that we do this in on single step so we have to add a correction at the end of the function
            current_pos = current_pos
            num_traps += 1
        else:
            current_pos = next_pos
        if move_was_jump:
            simulation_lattice[next_pos] = 0
            m_counter -= 1
        if m_counter == 0:
            return step + num_traps
            break
    
    if m_counter > 0:
        print('The walker has not yet eaten all of the pieces')
        return num_steps
    

print(time_finder(10, 10))