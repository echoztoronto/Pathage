/* General overview of how this algorithm works:
 * 
 * Create an instance of the Pathfinder class, creating the origin and setting the destination
 * Visit the first node
 * Calculate heuristic values for adjacent nodes and place Path_options in heap
 * 
 * Until reached destination:
 * Pop Path_option of least total_cost from heap
 * Visit node
 * Check if node is destination
 * If not, add Path_options for children (that have yet to be visited) to heap
 * 
 * Return the results using get_results;
 * Clean up all dynamically allocated Graph_nodes in the destructor
 * 
 * Note: All actual pathfinding done in the constructor, just call the result function
 */

#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <list>
#include <unordered_map>
#include <queue>

#include "StreetsDatabaseAPI.h"
#include "m1.h"
#include "graph_node.h"
#include "map_data.h"

class Pathfinder {
private:
    double turn_penalty;
    Graph_node* origin;
    Graph_node* destination;
    
    //results is intersection ids (named for historic reasons)
    std::list<unsigned> results;
    //segment_path is segment ids
    std::list<unsigned> segment_path;
    //"list" of all intersection nodes
    std::unordered_map<unsigned, Graph_node*> intersections;
    //list of all potential paths, ordered by least predicted total cost
    std::priority_queue<Path_option, std::vector<Path_option>, Path_comparator> paths;
    
public:
    //intersection ids of the origin and destination intersections
    Pathfinder(unsigned origin_id, unsigned destination_id, double turn_penalty_);
    ~Pathfinder();
    
    //get the specified node, creating it if necessary
    Graph_node* get_node(unsigned id);
    
    double get_turn_penalty();
    
    //return an empty list if no results
    std::list<unsigned>& get_results();
    
    //return an empty list if no results
    std::list<unsigned>& get_segment_results();
    
    //return the cost in seconds from source to destination
    double get_final_cost();
};

//Pathfinder, but with multiple possible destination nodes
class Multi_Pathfinder {
private:
    double turn_penalty;
    Graph_node* origin;
    Graph_node* reached_destination = NULL;
    std::vector<Graph_node*> destinations;
    std::list<unsigned> results;
    std::list<unsigned> segment_path;
    std::unordered_map<unsigned, Graph_node*> intersections;
    std::priority_queue<Multi_Path_option, std::vector<Multi_Path_option>, Multi_Path_comparator> paths;
    
public:
    //intersection ids of the origin and destination intersections
    Multi_Pathfinder(unsigned origin_id, std::vector<unsigned> destination_ids, double turn_penalty_);
    ~Multi_Pathfinder();
    
    //get the specified node, creating it if necessary
    Graph_node* get_node(unsigned id);
    
    double get_turn_penalty();
    
    //return an empty list if no results
    std::list<unsigned>& get_results();
    
    //return an empty list if no results
    std::list<unsigned>& get_segment_results();
    
    //return the cost in seconds from source to destination
    double get_final_cost();
};

//a faster version of Pathfinder that checks if a path exists at all
class Pathfinder_boolean {
private:
    double turn_penalty;
    Graph_node* origin;
    Graph_node* destination;
    
    //"list" of all intersection nodes
    std::unordered_map<unsigned, Graph_node*> intersections;
    //list of all potential paths, ordered by least predicted total cost
    std::priority_queue<Path_option, std::vector<Path_option>, Path_comparator> paths;
    
public:
    bool found_path = false;
    //intersection ids of the origin and destination intersections
    Pathfinder_boolean(unsigned origin_id, unsigned destination_id, double turn_penalty_);
    ~Pathfinder_boolean();
    
    //get the specified node, creating it if necessary
    Graph_node* get_node(unsigned id);
    
    double get_turn_penalty();
    
    //return an empty list if no results
    std::list<unsigned>& get_results();
    
    //return an empty list if no results
    std::list<unsigned>& get_segment_results();
    
    //return the cost in seconds from source to destination
    double get_final_cost();
};

#endif /* PATHFINDER_H */