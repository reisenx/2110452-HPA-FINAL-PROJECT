/**
 * @file optimize_v4.cpp
 * @author Worralop Srichainont
 * @brief Minimum Power Plant Coverage solution code with cache alignment and I/O optimization
 * @version 5.0
 * @date 12 Apr 2026
 */

#include<iostream>
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

uint128_t* adj_matrix = nullptr;

int min_n_power_plant = INT_MAX;
uint128_t best_solution = 0;

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
    if(n_nodes > 128)
    {
        cerr << "Error: The input graph exceeds 128 nodes.\n";
        exit(1);
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

    // Construct a graph from the input
    for(int i = 0; i < n_edges; ++i)
    {
        int u, v;
        parse_int(ptr, u);
        parse_int(ptr, v);

        adj_matrix[u] |= (((uint128_t)1) << v);
        adj_matrix[v] |= (((uint128_t)1) << u);
    }

    // Cleanup mmap and close the input file.
    munmap(file_data, sb.st_size);
    close(fd);
}

void output_solution(const char* file_path)
{
    // Convert 128-bit integer into a binary representation string
    string solution(n_nodes, '0');
    for(int i = 0; i < n_nodes; ++i)
    {
        if(best_solution & (((uint128_t)1) << i))
        {
            solution[i] = '1';
        }
    }

    // Write the solution to an output file
    FILE* output_file = fopen(file_path, "w");
    if(output_file)
    {
        if(is_display_min_n_power_plant)
        {
            fprintf(output_file, "%d\n", min_n_power_plant);
        }
        fprintf(output_file, "%s\n", solution.c_str());
        fclose(output_file);
        return;
    }
    cerr << "Error: Could not open the output file.\n";
    exit(1);
}

void place_power_plant(int current_n_power_plant, uint128_t is_not_covered, uint128_t current_solution)
{
    // Pruning the branch if it is worse than the current best solution.
    if(current_n_power_plant >= min_n_power_plant)
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
    uint128_t target_nodes = adj_matrix[current_node] | (((uint128_t)1) << current_node);

    // Iterate to every target node and try placing the power plant to each one.
    while(target_nodes > 0)
    {
        // Get the current node to try placing the power plant
        current_node = ctz128(target_nodes);

        // Get the coverage status if placing the power plant to the current node.
        uint128_t is_covered_by_current_node = adj_matrix[current_node] | (((uint128_t)1) << current_node);
        uint128_t next_is_not_covered = is_not_covered & ~is_covered_by_current_node;

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

    // Call the function to find the best solution
    place_power_plant(0, initial_is_not_covered, 0);

    // Write the solution to the output file.
    output_solution(argv[2]);

    // Free the adjacency matrix memory
    _mm_free(adj_matrix);
    return 0;
}
