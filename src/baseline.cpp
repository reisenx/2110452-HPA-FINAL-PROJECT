/**
 * @file baseline.cpp
 * @author Worralop Srichainont
 * @brief Minimum Power Plant Coverage baseline solution code
 * @version 1.0
 * @date 12 Apr 2026
 */

#include<iostream>
#include<vector>
#include<limits.h>
#include<fstream>
using namespace std;

int n_nodes, n_edges;

int min_n_power_plant = INT_MAX;
vector<vector<int>> adj_list;
vector<int> is_covered;
vector<int> current_solution;
vector<int> best_solution;

// Manually set this to debug the result.
bool is_display_min_n_power_plant = true;

void place_power_plant(int current_node, int current_n_power_plant)
{
    // Pruning the branch if it is worse than the current best solution.
    if(current_n_power_plant >= min_n_power_plant)
    {
        return;
    }

    // Validate current solution after iterating all nodes.
    if(current_node == n_nodes)
    {
        // Check if current solution covers all nodes.
        bool is_all_covered = true;
        for(int i = 0; i < n_nodes; i++)
        {
            if(is_covered[i] == 0)
            {
                is_all_covered = false;
                break;
            }
        }

        // Store the best solution
        if(is_all_covered)
        {
            min_n_power_plant = current_n_power_plant;
            best_solution.clear();
            for(int i = 0; i < n_nodes; i++)
            {
                best_solution.push_back(current_solution[i]);
            }
        }

        // Terminate the branch
        return;
    }

    // ========================= BRANCH 1 =========================
    // Choose to place the power plant to the current node
    current_solution.push_back(1);
    is_covered[current_node]++;
    for(int &adj_node : adj_list[current_node])
    {
        is_covered[adj_node]++;
    }

    // Call recursive function to consider the next node
    place_power_plant(current_node + 1, current_n_power_plant + 1);

    // Backtracking
    current_solution.pop_back();
    is_covered[current_node]--;
    for(int &adj_node : adj_list[current_node])
    {
        is_covered[adj_node]--;
    }
    // ==========================================================

    // ========================= BRANCH 2 =========================
    // Choose to not place the power plant to the current node
    current_solution.push_back(0);

    // Call recursive function to consider the next node
    place_power_plant(current_node + 1, current_n_power_plant);

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

    // Expand the graph adjacency list and coverage vector
    adj_list.resize(n_nodes);
    is_covered.assign(n_nodes, 0);

    // Construct a graph
    for(int i = 0; i < n_edges; i++)
    {
        int u, v;
        input_file >> u >> v;
        adj_list[u].push_back(v);
        adj_list[v].push_back(u);
    }

    // Close the input file
    input_file.close();

    // Call the function to find the best solution
    place_power_plant(0, 0);

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
