/*
 * Implementation of the graph used in the Pathfinding class
 */

#ifndef GRAPH_NODE_H
#define GRAPH_NODE_H

#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"

#include "LatLon.h"
#include "m1.h"
#include "map_data.h"

//class used to store the actual node on the graph
class Graph_node {
private: 
    unsigned id;
    LatLon position;
    bool visited = false;
    bool is_origin = false;
    bool is_destination = false;
    double cost_to_node = 0.0;
    
    //pointer to the node used to visit this one
    //used to return the path results
    //set only when actually visited
    Graph_node *source = NULL;
    //similarly, segment used to get to this node
    unsigned source_segment = 0;
    
public:
    Graph_node (unsigned id_);
    ~Graph_node();
    
    //set this node as the start of the graph
    void set_as_origin();
    //set this node as a destination
    void set_as_destination();
    //mark visited, mark source, return whether visit is least-cost
    bool visit_node(Graph_node* source_, unsigned source_segment_, double cost_to_node_);
    
    bool get_is_origin();
    bool get_is_destination();
    //return whether visited or not
    bool get_visited();
    
    //return source node
    Graph_node* get_from_intersection();
    unsigned get_from_segment();
    unsigned get_id();
    LatLon get_position();
    double get_cost_to_node();
};

//class used to store potential (yet unvisited) nodes on the heap
class Path_option {
public:
    Graph_node *source_node;
    Graph_node *next_node;
    Graph_node *destination;
    unsigned source_to_next_segment;
    double origin_to_next_cost;
    double predicted_next_to_destination_cost;
    double total_cost;
    
    Path_option(double origin_to_source_cost, Graph_node* source_node_, 
        Graph_node* next_node_, Graph_node* destination_, 
        unsigned source_to_next_segment_, double turn_penalty);
    ~Path_option();
    
    double calculate_heuristic();
    std::vector<unsigned> get_connected_street_segments(unsigned intersection_id);
};

//used to implement min heap
class Path_comparator {
public:
    bool operator()(Path_option first, Path_option second) {
        return first.total_cost > second.total_cost;
    }
};

//Multi_Pathfinder version of Path_option
class Multi_Path_option {
public:
    Graph_node *source_node;
    Graph_node *next_node;
    unsigned source_to_next_segment;
    double origin_to_next_cost;
    double predicted_next_to_destination_cost;
    double total_cost;
    
    Multi_Path_option(double origin_to_source_cost, Graph_node* source_node_, 
        Graph_node* next_node_, std::vector<Graph_node*>& destinations, 
        unsigned source_to_next_segment_, double turn_penalty);
    ~Multi_Path_option();
    
    double calculate_heuristic(std::vector<Graph_node*>& destinations);
};

//used to implement min heap
class Multi_Path_comparator {
public:
    bool operator()(Multi_Path_option first, Multi_Path_option second) {
        return first.total_cost > second.total_cost;
    }
};

//helper function
unsigned get_best_segment(bool has_previous_segment, unsigned previous_segment_id, 
        unsigned from_intersection_id, unsigned to_intersection_id, double turn_penalty);

unsigned get_any_segment(unsigned from_intersection_id, unsigned to_intersection_id);

#endif /* GRAPH_NODE_H */

