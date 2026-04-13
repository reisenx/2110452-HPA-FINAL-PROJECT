/**
 * @file optimize_v5.cpp
 * @author Worralop Srichainont
 * @brief Minimum Power Plant Coverage solution code with node sorting and greedy algorithm.
 * @version 6.0
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

using namespace std;

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
    // Case 1: the first 1-bit is in the right 64-bit chunk
    uint64_t right = (uint64_t)x;
    if(right > 0)
    {
        return __builtin_ctzll(right);
    }

    // Case 2: the first 1-bit is in the left 64-bit chunk
    return 64 + __builtin_ctzll((uint64_t)(x >> 64));
}

int n_nodes, n_edges;

int max_degree = 0;
uint128_t* adj_matrix = nullptr;

int min_n_power_plant = INT_MAX;
uint128_t best_solution = 0;

vector<int> to_original_node_id;

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
    if(n_nodes > 128)
    {
        cerr << "Error: The input graph exceeds 128 nodes.\n";
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

    // Sorting nodes by its degree in descending order and find the maximum degree.
    for(int i = 0; i < n_nodes; ++i)
    {
        sorted_nodes[i] = {degrees[i], i};
        if(degrees[i] > max_degree)
        {
            max_degree = degrees[i];
        }
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

    // Construct the adjacency list with the node mappings applied.
    for(int i = 0; i < n_edges; i++)
    {
        int u = to_new_node_id[edges[i].first];
        int v = to_new_node_id[edges[i].second];

        adj_matrix[u] |= (((uint128_t)1) << v);
        adj_matrix[v] |= (((uint128_t)1) << u);
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
        fprintf(output_file, "%s\n", solution.c_str());
        fclose(output_file);
        return;
    }
    cerr << "Error: Could not open the output file.\n";
    exit(1);
}

void setup_greedy_solution(uint128_t initial_is_not_covered)
{
    uint128_t is_not_covered = initial_is_not_covered;
    uint128_t greedy_solution = 0;
    int greedy_min_n_power_plant = 0;

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
    if(current_n_power_plant >= min_n_power_plant)
    {
        return;
    }

    // Pruning the branch if it is impossible to beat the best solution.
    int n_uncovered = popcount128(is_not_covered);
    int min_additional_n_power_plant = (n_uncovered + max_degree) / (max_degree + 1);
    if(current_n_power_plant + min_additional_n_power_plant >= min_n_power_plant)
    {
        return;
    }

    // Store the best solution if all nodes are covered
    if(is_not_covered == 0)
    {
        min_n_power_plant = current_n_power_plant;
        best_solution = current_solution;
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
        target_nodes &= (target_nodes - 1);
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

    // Create the initial coverage status where every node is not covered.
    uint128_t initial_is_not_covered = 0;
    for(int i = 0; i < n_nodes; ++i)
    {
        initial_is_not_covered |= (((uint128_t)1) << i);
    }

    // Set the current best solution to the greedy algorithm solution
    setup_greedy_solution(initial_is_not_covered);

    // Call the function to find the best solution
    place_power_plant(0, initial_is_not_covered, 0);

    // Write the solution to the output file.
    output_solution(argv[2]);

    // Free the adjacency matrix memory
    _mm_free(adj_matrix);
    return 0;
}
