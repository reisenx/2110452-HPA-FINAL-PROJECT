/**
 * @file optimize_v2.cpp
 * @author Worralop Srichainont
 * @brief Minimum Power Plant Coverage solution code with bitmasking.
 * @version 3.0
 * @date 12 Apr 2026
 */

#include<iostream>
#include<vector>
#include<limits.h>
#include<fstream>
#include<cstdint>

using namespace std;

struct uint128_t
{
    // Struct attributes
    uint64_t left = 0;
    uint64_t right = 0;

    // Overloading AND operation
    uint128_t operator&(const uint128_t& other) const
    {
        return {left & other.left, right & other.right};
    }

    // Overloading NOT operation
    uint128_t operator~() const
    {
        return {~left, ~right};
    }

    // Utility function to check if all bits are 0
    bool is_zero() const
    {
        return left == 0 && right == 0;
    }

    // Utility function to set the specific bit to 1
    void set_bit(int idx)
    {
        if(idx < 64)
        {
            right |= (1ULL << idx);
        }
        else
        {
            left |= (1ULL << (idx - 64));
        }
    }
};

int n_nodes, n_edges;

int min_n_power_plant = INT_MAX;
vector<uint128_t> adj_matrix;
vector<int> current_solution;
vector<int> best_solution;

// Manually set this to debug the result.
bool is_display_min_n_power_plant = true;

void place_power_plant(int current_node, int current_n_power_plant, uint128_t is_not_covered)
{
    // Pruning the branch if it worse than the current best solution.
    if(current_n_power_plant >= min_n_power_plant)
    {
        return;
    }

    // Store the best solution if all nodes are covered
    if(is_not_covered.is_zero())
    {
        min_n_power_plant = current_n_power_plant;
        best_solution = current_solution;
        
        // Add 0 for all remaining nodes
        while(best_solution.size() < n_nodes)
        {
            best_solution.push_back(0);
        }

        // Terminate the branch
        return;
    }

    // Terminate the branch, if all nodes are checked but haven't covered every node.
    if(current_node == n_nodes)
    {
        return;
    }

    // ========================= BRANCH 1 =========================
    // Choose to place the power plant to the current node
    current_solution.push_back(1);
    
    // Update power coverage status
    uint128_t next_is_not_covered = is_not_covered & ~adj_matrix[current_node];

    // Call recursive function to consider the next node
    place_power_plant(current_node + 1, current_n_power_plant + 1, next_is_not_covered);

    // Backtracking
    current_solution.pop_back();
    // ==========================================================

    // ========================= BRANCH 2 =========================
    // Choose to not place the power plant to the current node
    current_solution.push_back(0);

    // Call recursive function to consider the next node
    place_power_plant(current_node + 1, current_n_power_plant, is_not_covered);

    // Backtracking
    current_solution.pop_back();
    // ==========================================================
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

    // Open the input file
    ifstream input_file(argv[1]);
    if(!input_file.is_open())
    {
        cerr << "Error: Could not open the input file.\n";
        return 1;
    }

    // Read the number of nodes and edges of the graph
    input_file >> n_nodes >> n_edges;

    // Expand the graph adjacency matrix
    adj_matrix.assign(n_nodes, uint128_t());

    // Setup adjacency matrix to make every node connects to itself.
    for(int i = 0; i < n_nodes; i++)
    {
        adj_matrix[i].set_bit(i);
    }

    // Construct a graph from the input
    for(int i = 0; i < n_edges; i++)
    {
        int u, v;
        input_file >> u >> v;
        adj_matrix[u].set_bit(v);
        adj_matrix[v].set_bit(u);
    }

    // Close the input file
    input_file.close();

    // Call the function to find the best solution
    uint128_t initial_is_not_covered;
    for(int i = 0; i < n_nodes; i++)
    {
        initial_is_not_covered.set_bit(i);
    }
    place_power_plant(0, 0, initial_is_not_covered);

    // Open the output file
    ofstream output_file(argv[2]);
    if(!output_file.is_open())
    {
        cerr << "Error: Could not open the output file.\n";
        return 1;
    }

    // Write the best solution to the output file
    if(is_display_min_n_power_plant)
    {
        output_file << min_n_power_plant << "\n";
    }
    for(int &node : best_solution)
    {
        output_file << node;
    }
    output_file << "\n";

    // Close the output file
    output_file.close();
    return 0;
}
