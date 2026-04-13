/**
 * @file optimize_v8.cpp
 * @author Worralop Srichainont
 * @brief Minimum Power Plant Coverage solution code with graph reduction and atomic variable.
 * @version 9.0
 * @date 12 Apr 2026
 */

#include<iostream>
#include<vector>
#include<algorithm>
#include<string>
#include<cstdio>
#include<cstdlib>
#include<limits.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<unistd.h>
#include<mm_malloc.h>
#include<cstdint>
#include<immintrin.h>
#include<omp.h>
#include<atomic>

using namespace std;

#define MAX_NODE 128

// Define 128-bit integer (uint128_t)
typedef unsigned __int128 uint128_t;

// Implement 1-bit counting function for uint128_t
inline int popcount128(uint128_t x)
{
    return (
        __builtin_popcountll((uint64_t)(x >> 64))
        + __builtin_popcountll((uint64_t)x)
    );
}

// Implement the first 1-bit finding function for uint128_t
inline int ctz128(uint128_t x)
{
    uint64_t right = (uint64_t)x;
    return (right > 0) ? (__builtin_ctzll(right)) : (64 + __builtin_ctzll((uint64_t)(x >> 64)));
}

int n_nodes, n_edges;

uint128_t* adj_matrix = nullptr;
uint128_t* adj_matrix_two_hop = nullptr;

atomic<int> min_n_power_plant{INT_MAX};
uint128_t best_solution = 0;

vector<int> to_original_node_id;

// Manually set this to debug the result.
bool is_display_min_n_power_plant = true;

inline void parse_int(char*& ptr, int& value)
{
    // Set the initial value of the current integer
    value = 0;

    // Skip any character that is not an integer
    while(*ptr < '0' || *ptr > '9')
    {
        ptr++;
    }

    // Parsing by reading digits until the end of an integer.
    while(*ptr >= '0' && *ptr <= '9')
    {
        value = (value * 10) + (*ptr - '0');
        ptr++;
    }
}

void load_graph(const char* file_path)
{
    // Open the input file
    int fd = open(file_path, O_RDONLY);
    if(fd == -1)
    {
        cerr << "Error: Could not open the input file.\n";
        exit(1);
    }

    // OS takes the entire chunk of text and dump into RAM.
    // The pointer file_data points at the first char of the input file.
    struct stat sb;
    fstat(fd, &sb);
    char* file_data = static_cast<char*>(
        mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0)
    );

    // Initialize the pointer for parsing an integer from the input file.
    char* ptr = file_data;

    // Read the number of nodes and edges of the graph
    parse_int(ptr, n_nodes);
    parse_int(ptr, n_edges);
    if(n_nodes > MAX_NODE)
    {
        cerr << "Error: The input graph exceeds " << MAX_NODE << " nodes.\n";
        exit(1);
    }

    // Initialize variables for node sorting
    vector<int> degrees(n_nodes, 0);
    vector<pair<int, int>> edges(n_edges);
    vector<pair<int, int>> sorted_nodes(n_nodes);

    // Store all graph edges and count degrees
    for(int i = 0; i < n_edges; ++i)
    {
        parse_int(ptr, edges[i].first);
        parse_int(ptr, edges[i].second);

        degrees[edges[i].first]++;
        degrees[edges[i].second]++;
    }

    // Cleanup mmap and close the input file.
    munmap(file_data, sb.st_size);
    close(fd);

    // Sorting nodes by its degree in descending order.
    for(int i = 0; i < n_nodes; ++i)
    {
        sorted_nodes[i] = {degrees[i], i};
    }
    sort(sorted_nodes.rbegin(), sorted_nodes.rend());

    // Initialize variables for node mappin
    to_original_node_id.assign(n_nodes, 0);
    vector<int> to_new_node_id(n_nodes);

    // Construct node mappings
    for(int new_node_id = 0; new_node_id < n_nodes; new_node_id++)
    {
        int original_node_id = sorted_nodes[new_node_id].second;

        to_original_node_id[new_node_id] = original_node_id;
        to_new_node_id[original_node_id] = new_node_id;
    }

    // Calculate how many memory (bytes) use for an adjacency matrix.
    size_t adj_matrix_size = n_nodes * sizeof(uint128_t);

    // Allocate a contiguous memory block and cast it to the correct type
    adj_matrix = static_cast<uint128_t*>(_mm_malloc(adj_matrix_size, 64));

    // Initialize every bitmask of each node to 0
    for(size_t i = 0; i < n_nodes; ++i)
    {
        adj_matrix[i] = 0;
    }

    // Setup adjacency matrix to make every node connects to itself.
    for(int i = 0; i < n_nodes; i++)
    {
        adj_matrix[i] |= (((uint128_t)1) << i);
    }

    // Construct the adjacency matrix with the node mappings applied.
    for(int i = 0; i < n_edges; i++)
    {
        int u = to_new_node_id[edges[i].first];
        int v = to_new_node_id[edges[i].second];

        adj_matrix[u] |= (((uint128_t)1) << v);
        adj_matrix[v] |= (((uint128_t)1) << u);
    }

    // Allocate a contiguous memory block and cast it to the correct type
    adj_matrix_two_hop = static_cast<uint128_t*>(_mm_malloc(adj_matrix_size, 64));

    // Construct the two-hop adjacency matrix from the adjacency matrix.
    for(int i = 0; i < n_nodes; i++)
    {
        // Initialize with the normal adjacency matrix
        adj_matrix_two_hop[i] = adj_matrix[i];

        // Iterate to each adjacent node and updates adjacency matrix.
        uint128_t adj_nodes = adj_matrix[i];
        while(adj_nodes > 0)
        {
            // Choose a single adjacent node
            int current_node = ctz128(adj_nodes);

            // Add the adjacent node of the current node into a adjacency matrix.
            adj_matrix_two_hop[i] |= adj_matrix[current_node];

            // Remove the current node.
            adj_nodes &= (adj_nodes - 1);
        }
    }
}

void output_solution(const char* file_path)
{
    // Convert 128-bit integer into a binary representation string
    string solution(n_nodes, '0');
    for(int i = 0; i < n_nodes; ++i)
    {
        if(best_solution & (((uint128_t)1) << i))
        {
            // Mapping to the original node numbers.
            solution[to_original_node_id[i]] = '1';
        }
    }

    // Write the solution to an output file
    FILE* output_file = fopen(file_path, "w");
    if(output_file)
    {
        if(is_display_min_n_power_plant)
        {
            fprintf(output_file, "%d\n", min_n_power_plant.load());
        }
        fprintf(output_file, "%s\n", solution.c_str());
        fclose(output_file);
        return;
    }
    cerr << "Error: Could not open the output file.\n";
    exit(1);
}

void reduce_graph_leaves(int &initial_n_power_plant, uint128_t &initial_is_not_covered, uint128_t &initial_solution)
{
    // While the graph is reducible, try to pruning the leaf as much as possible.
    uint128_t target_nodes = initial_is_not_covered;
    while(target_nodes > 0)
    {
        // Find the first node that is currently not covered.
        int current_node = ctz128(target_nodes);

        // Get a bitmask of adjacent nodes and count them.
        uint128_t adj_nodes = adj_matrix[current_node] & ~(((uint128_t)1) << current_node);
        int n_adj_nodes = popcount128(adj_nodes);

        // Remove the current node from the target nodes
        target_nodes &= ~(((uint128_t)1) << current_node);

        // CASE 1: The current node is a leaf. Place the power plant to its parent.
        if(n_adj_nodes == 1)
        {
            // Get the parent node
            int parent_node = ctz128(adj_nodes);

            // If the parent node has no power plant yet, place it.
            if((initial_solution & (((uint128_t)1) << parent_node)) == 0)
            {
                // Add the parent node to the solution
                initial_solution |= (((uint128_t)1) << parent_node);
                initial_n_power_plant++;

                // Mark the parent node and its adjacent node as covered
                initial_is_not_covered &= ~adj_matrix[parent_node];
                target_nodes &= initial_is_not_covered;
            }
        }

        // CASE 2: The current node is a isolated. Place the power plant to it.
        else if(n_adj_nodes == 0)
        {
            // If the current node has no power plant yet, place it.
            if((initial_solution & (((uint128_t)1) << current_node)) == 0)
            {
                // Place the power plant to the current node
                initial_solution |= (((uint128_t)1) << current_node);
                initial_n_power_plant++;

                // Mark this node as covered
                initial_is_not_covered &= ~(((uint128_t)1) << current_node);
                target_nodes &= initial_is_not_covered;
            }
        }
    }
}

inline int get_lower_bound(uint128_t current_is_not_covered)
{
    int n_independent_node_set = 0;
    uint128_t is_not_covered = current_is_not_covered;

    // Count amount of independent node set.
    while(is_not_covered > 0)
    {
        // Choose the first uncovered node
        int current_node = ctz128(is_not_covered);

        // Remove the current_node along with two-hop adjacent nodes
        is_not_covered &= ~adj_matrix_two_hop[current_node];

        // Update independent node set counter
        n_independent_node_set++;
    }

    return n_independent_node_set;
}

void setup_greedy_solution(int initial_n_power_plant, uint128_t initial_is_not_covered, uint128_t initial_solution)
{
    int greedy_min_n_power_plant = initial_n_power_plant;
    uint128_t is_not_covered = initial_is_not_covered;
    uint128_t greedy_solution = initial_solution;

    // Loop until the greedy algorithm covers entire graph.
    while(is_not_covered > 0)
    {
        // Find a node that covers the most node in the current state.
        int best_node = -1;
        int max_n_covered = -1;

        for(int i = 0; i < n_nodes; ++i)
        {
            int n_covered = popcount128(is_not_covered & adj_matrix[i]);

            if(n_covered > max_n_covered)
            {
                max_n_covered = n_covered;
                best_node = i;
            }
        }

        // Place the power plant to the best node found by greedy algorithm.
        greedy_solution |= (((uint128_t)1) << best_node);
        greedy_min_n_power_plant++;

        // Update power coverage status
        is_not_covered &= ~adj_matrix[best_node];
    }

    // Set the current best solution to the greedy algorithm solution
    best_solution = greedy_solution;
    min_n_power_plant = greedy_min_n_power_plant;
}

void place_power_plant(int current_n_power_plant, uint128_t is_not_covered, uint128_t current_solution)
{
    // Pruning the branch if it is already worse than the current best solution.
    if(current_n_power_plant >= min_n_power_plant.load(memory_order_relaxed))
    {
        return;
    }

    // Pruning the branch if it is impossible to beat the best solution.
    int min_additional_n_power_plant = get_lower_bound(is_not_covered);
    if(current_n_power_plant + min_additional_n_power_plant >= min_n_power_plant.load(memory_order_relaxed))
    {
        return;
    }

    // Store the best solution if all nodes are covered
    if(is_not_covered == 0)
    {
        if(current_n_power_plant < min_n_power_plant.load(memory_order_relaxed))
        {
            // To ensure that only one thread can execute this code at a time.
            #pragma omp critical
            {
                if(current_n_power_plant < min_n_power_plant.load(memory_order_relaxed))
                {
                    min_n_power_plant.store(current_n_power_plant, memory_order_relaxed);
                    best_solution = current_solution;
                }
            }
        }
        return;
    }

    // Get the first node that is not covered and its adjacent node as a bitmask.
    int current_node = ctz128(is_not_covered);
    uint128_t target_nodes = adj_matrix[current_node];

    // Iterate to every target node and try placing the power plant to each one.
    while(target_nodes > 0)
    {
        // Get the current node to try placing the power plant
        current_node = ctz128(target_nodes);

        // Get the coverage status if placing the power plant to the current node.
        uint128_t next_is_not_covered = is_not_covered & ~adj_matrix[current_node];

        // Get the solution if placing the power plant to the current node.
        uint128_t next_solution = current_solution | (((uint128_t)1) << current_node);

        // Call recursive function to consider further
        place_power_plant(current_n_power_plant + 1, next_is_not_covered, next_solution);

        // Remove the current node from the target nodes
        target_nodes &= (target_nodes - ((uint128_t)1));
    }
}

int main(int argc, char* argv[])
{
    // Make standard C++ streams faster
    ios_base::sync_with_stdio(false);
    cin.tie(0);

    // Validate the command line
    if(argc < 3)
    {
        cerr << "Error: Invalid command line.\n";
        return 1;
    }

    // Construct the graph from the input
    load_graph(argv[1]);

    // Define initial variables
    int initial_n_power_plant = 0;
    uint128_t initial_solution = 0;
    uint128_t initial_is_not_covered = 0;

    // Set the initial coverage status where every node is not covered.
    for(int i = 0; i < n_nodes; ++i)
    {
        initial_is_not_covered |= (((uint128_t)1) << i);
    }

    // Perform the initial graph reduction and update the initial variables.
    reduce_graph_leaves(initial_n_power_plant, initial_is_not_covered, initial_solution);

    // CASE 1: The graph reduction process already covers all the nodes
    if(initial_is_not_covered == 0)
    {
        best_solution = initial_solution;
        min_n_power_plant = initial_n_power_plant;
    }

    // CASE 2: The graph is not all covered yet. Perform the greedy algorithm and branch and bound further.
    else
    {
        // Set the current best solution to the greedy algorithm solution
        setup_greedy_solution(initial_n_power_plant, initial_is_not_covered, initial_solution);

        // Get the first node to check of a graph
        int current_node = ctz128(initial_is_not_covered);
        uint128_t adj_nodes = adj_matrix[current_node];

        // Store the first level of searching tree to the vector.
        int root_branches[MAX_NODE];
        int n_root_branches = 0;
        while(adj_nodes > 0)
        {
            current_node = ctz128(adj_nodes);
            root_branches[n_root_branches++] = current_node;
            adj_nodes &= (adj_nodes - ((uint128_t)1));
        }

        // Create the OpenMP parallel region
        #pragma omp parallel
        {
            // Use only one thread to generate the initial recursive call
            #pragma omp single
            {
                for(size_t i = 0; i < n_root_branches; ++i)
                {
                    // Get the root of each subtree
                    current_node = root_branches[i];

                    // Get parameters for each searching subtrees
                    uint128_t next_is_not_covered = initial_is_not_covered & ~adj_matrix[current_node];
                    uint128_t next_solution = initial_solution | (((uint128_t)1) << current_node);

                    // Distribute each searching subtree to each thread by using OpenMP
                    #pragma omp task shared(min_n_power_plant, best_solution)
                    place_power_plant(initial_n_power_plant + 1, next_is_not_covered, next_solution);
                }
            }
        }
    }

    // Write the solution to the output file.
    output_solution(argv[2]);

    // Free the adjacency matrix memory
    _mm_free(adj_matrix);
    _mm_free(adj_matrix_two_hop);
    return 0;
}
