/*
 * Implementation file for the pathfinder class
 */

#include "pathfinder.h"
#include "graph_node.h"
#include <chrono>

extern Map_data const *map_data;

/****************************************************************************************
                                  Pathfinder
 ****************************************************************************************/

//intersection ids of the origin and destination intersections
Pathfinder::Pathfinder(unsigned origin_id, unsigned destination_id, double turn_penalty_) {
    turn_penalty = turn_penalty_;
    
    destination = new Graph_node(destination_id);
    destination->set_as_destination();
    
    origin = new Graph_node(origin_id);
    origin->set_as_origin();
    
    //add the start and end to the "list" of known nodes
    intersections[origin_id] = origin;
    intersections[destination_id] = destination;
    
    //keep time to keep runtime for more complicated queries reasonable
    std::chrono::time_point<std::chrono::system_clock> start_time, current_time;
    start_time = std::chrono::system_clock::now();
    current_time = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = current_time - start_time;
    
    //set first path and enter loop
    paths.push(Path_option(0.0, NULL, origin, destination, 0, turn_penalty));
    bool found_path = false;
    double current_origin_to_destination_cost;
    
    while (!paths.empty() && elapsed_seconds.count() < 1.0) {
        //get and remove the top path
        Path_option current_path = paths.top();
        paths.pop();
        
        //once found a path to the destination, cut off other partial paths if they exceed the already-found cost
        if (!found_path || current_path.total_cost < current_origin_to_destination_cost) {
            
            //if first visit to node or shorter path than previous visit
            if (current_path.next_node->visit_node(current_path.source_node, current_path.source_to_next_segment, current_path.origin_to_next_cost)) {
                
                //mark when found a path
                if (current_path.next_node->get_is_destination()) {
                    current_origin_to_destination_cost = current_path.next_node->get_cost_to_node();
                    found_path = true;
                }
                
                //don't expand around the destination node
                else {
                    //get all connected intersections and add nodes for each, excluding node we came from
                    std::vector<unsigned> adjacent_intersections_ids = find_adjacent_intersections(current_path.next_node->get_id());
                    unsigned source_id = 0;
                    if (current_path.source_node != NULL) {
                        source_id = current_path.source_node->get_id();
                    }
                    for (auto i = adjacent_intersections_ids.begin(); i != adjacent_intersections_ids.end(); i++) {
                        //remove path we came from and one-way streets in the wrong direction
                        if (current_path.source_node == NULL || (*i != source_id && are_directly_connected(current_path.next_node->get_id(), *i))) {
                            Graph_node* candidate_node = get_node(*i);
                            unsigned segment_to_use = get_best_segment(
                                !(current_path.next_node->get_is_origin()) && !(current_path.source_node->get_is_origin()), 
                                current_path.next_node->get_from_segment(), 
                                current_path.next_node->get_id(), 
                                candidate_node->get_id(), turn_penalty
                                );
                            paths.push(Path_option(current_path.origin_to_next_cost, current_path.next_node, candidate_node, destination, segment_to_use, turn_penalty) );
                        }
                    }
                }
            }
        }
        //stop the computation at 0.5s
        current_time = std::chrono::system_clock::now();
        elapsed_seconds = current_time - start_time;
    }
    //backtrack through the nodes and fill out the results
    if (found_path) {
        Graph_node* trace_back = destination;
        while (!trace_back->get_is_origin()) {
            results.push_front(trace_back->get_id());
            segment_path.push_front(trace_back->get_from_segment());
            trace_back = trace_back->get_from_intersection();
        }
        results.push_front(origin_id);
    }
}

//clean up all the dynamically allocated nodes
Pathfinder::~Pathfinder() {
    for (auto i = intersections.begin(); i != intersections.end(); i++) {
        delete i->second;
    }
}

//get the specified node from the unordered_map, creating it if necessary
Graph_node* Pathfinder::get_node(unsigned id) {
    if (intersections.count(id) != 0) {
        return intersections[id];
    }
    else {
        Graph_node* new_node = new Graph_node(id);
        intersections[id] = new_node;
        return new_node;
    }
}

double Pathfinder::get_turn_penalty() {
    return turn_penalty;
}

//return an empty list if no results
std::list<unsigned>& Pathfinder::get_results() {
    return results;
}

 //return an empty list if no results
std::list<unsigned>& Pathfinder::get_segment_results() {
    return segment_path;
}

//return the cost in seconds from source to destination
double Pathfinder::get_final_cost() {
    return destination->get_cost_to_node();
}

/****************************************************************************************
                                  Multi_Pathfinder
 ****************************************************************************************/

//intersection ids of the origin and destination intersections
Multi_Pathfinder::Multi_Pathfinder(unsigned origin_id, std::vector<unsigned> destination_ids, double turn_penalty_) {
    turn_penalty = turn_penalty_;
    
    //initialize start and end nodes, and add them to the "list" of known nodes
    
    origin = new Graph_node(origin_id);
    origin->set_as_origin();
    intersections[origin_id] = origin;
    
    //create a node for each destination and mark each as a target to get to
    for (auto i = destination_ids.begin(); i != destination_ids.end(); i++) {
        Graph_node* destination = new Graph_node(*i);
        destination->set_as_destination();
        intersections[*i] = destination;
        destinations.push_back(destination);
    }   
    
    std::chrono::time_point<std::chrono::system_clock> start_time, current_time;
    start_time = std::chrono::system_clock::now();
    current_time = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = current_time - start_time;
    
    //set first path and enter loop
    paths.push(Multi_Path_option(0.0, NULL, origin, destinations, 0, turn_penalty));
    bool found_path = false;
    double current_origin_to_destination_cost = 0.0;
    
    while (!paths.empty() && elapsed_seconds.count() < 5.0) {
        //get and remove the top path
        Multi_Path_option current_path = paths.top();
        paths.pop();
        
        //once found a path to the destination, cut off other partial paths if they exceed the already-found cost
        if (!found_path || current_path.total_cost < current_origin_to_destination_cost) {
            
            //if first visit to node or shorter path than previous visit
            if (current_path.next_node->visit_node(current_path.source_node, current_path.source_to_next_segment, current_path.origin_to_next_cost)) {
                
                //mark when found a path
                if (current_path.next_node->get_is_destination()) {
                    if (!found_path || current_path.total_cost < current_origin_to_destination_cost) {
                        current_origin_to_destination_cost = current_path.next_node->get_cost_to_node();
                        reached_destination = current_path.next_node;
                        found_path = true;
                    }
                }                
                //don't expand around a destination node
                else {
                    //get all connected intersections and add nodes for each, excluding node we came from
                    std::vector<unsigned> adjacent_intersections_ids = find_adjacent_intersections(current_path.next_node->get_id());
                    unsigned source_id = 0;
                    if (current_path.source_node != NULL) {
                        source_id = current_path.source_node->get_id();
                    }
                    for (auto i = adjacent_intersections_ids.begin(); i != adjacent_intersections_ids.end(); i++) {
                        //remove path we came from and one-way streets in the wrong direction
                        if (current_path.source_node == NULL || (*i != source_id && are_directly_connected(current_path.next_node->get_id(), *i))) {
                            Graph_node* candidate_node = get_node(*i);
                            unsigned segment_to_use = get_best_segment(
                                !(current_path.next_node->get_is_origin()) && !(current_path.source_node->get_is_origin()), 
                                current_path.next_node->get_from_segment(), 
                                current_path.next_node->get_id(), 
                                candidate_node->get_id(), turn_penalty
                                );
                            paths.push(Multi_Path_option(current_path.origin_to_next_cost, current_path.next_node, candidate_node, destinations, segment_to_use, turn_penalty) );
                        }
                    }
                }
            }
        }
        //stop the computation at 5s
        current_time = std::chrono::system_clock::now();
        elapsed_seconds = current_time - start_time;
    }
    //backtrack through the nodes
    if (found_path) {
        Graph_node* trace_back = reached_destination;
        while (!trace_back->get_is_origin()) {
            results.push_front(trace_back->get_id());
            segment_path.push_front(trace_back->get_from_segment());
            trace_back = trace_back->get_from_intersection();
        }
        results.push_front(origin_id);
    }
}

//clean up all the dynamically allocated nodes
Multi_Pathfinder::~Multi_Pathfinder() {
    for (auto i = intersections.begin(); i != intersections.end(); i++) {
        delete i->second;
    }
}

//get the specified node, creating it if necessary
Graph_node* Multi_Pathfinder::get_node(unsigned id) {
    if (intersections.count(id) != 0) {
        return intersections[id];
    }
    else {
        Graph_node* new_node = new Graph_node(id);
        intersections[id] = new_node;
        return new_node;
    }
}

double Multi_Pathfinder::get_turn_penalty() {
    return turn_penalty;
}

//return an empty list if no results
std::list<unsigned>& Multi_Pathfinder::get_results() {
    return results;
}

 //return an empty list if no results
std::list<unsigned>& Multi_Pathfinder::get_segment_results() {
    return segment_path;
}

//return the cost in seconds from source to destination
double Multi_Pathfinder::get_final_cost() {
    if (reached_destination != NULL) {
        return reached_destination->get_cost_to_node();
    }
    else {
        return 0.0;
    }
}

/****************************************************************************************
                                  Pathfinder_boolean
 ****************************************************************************************/

//intersection ids of the origin and destination intersections
Pathfinder_boolean::Pathfinder_boolean(unsigned origin_id, unsigned destination_id, double turn_penalty_) {
    turn_penalty = turn_penalty_;
    
    destination = new Graph_node(destination_id);
    destination->set_as_destination();
    
    origin = new Graph_node(origin_id);
    origin->set_as_origin();
    
    //add the start and end to the "list" of known nodes
    intersections[origin_id] = origin;
    intersections[destination_id] = destination;
    
    //set first path and enter loop
    paths.push(Path_option(0.0, NULL, origin, destination, 0, turn_penalty));
    
    while (!found_path && !paths.empty()) {
        //get and remove the top path
        Path_option current_path = paths.top();
        paths.pop();

        //if first visit to node or shorter path than previous visit
        if (current_path.next_node->visit_node(current_path.source_node, current_path.source_to_next_segment, current_path.origin_to_next_cost)) {

            //mark when found a path
            if (current_path.next_node->get_is_destination()) {
                found_path = true;
            }

            //don't expand around the destination node
            else {
                //get all connected intersections and add nodes for each, excluding node we came from
                std::vector<unsigned> adjacent_intersections_ids = find_adjacent_intersections(current_path.next_node->get_id());
                unsigned source_id = 0;
                if (current_path.source_node != NULL) {
                    source_id = current_path.source_node->get_id();
                }
                for (auto i = adjacent_intersections_ids.begin(); i != adjacent_intersections_ids.end(); i++) {
                    //remove path we came from and one-way streets in the wrong direction
                    if (current_path.source_node == NULL || (*i != source_id && are_directly_connected(current_path.next_node->get_id(), *i))) {
                        Graph_node* candidate_node = get_node(*i);
                        unsigned segment_to_use = get_any_segment(current_path.next_node->get_id(), candidate_node->get_id());
                        paths.push(Path_option(current_path.origin_to_next_cost, current_path.next_node, candidate_node, destination, segment_to_use, turn_penalty) );
                    }
                }
            }
        }
    }
}

//clean up all the dynamically allocated nodes
Pathfinder_boolean::~Pathfinder_boolean() {
    for (auto i = intersections.begin(); i != intersections.end(); i++) {
        delete i->second;
    }
}

//get the specified node from the unordered_map, creating it if necessary
Graph_node* Pathfinder_boolean::get_node(unsigned id) {
    if (intersections.count(id) != 0) {
        return intersections[id];
    }
    else {
        Graph_node* new_node = new Graph_node(id);
        intersections[id] = new_node;
        return new_node;
    }
}

double Pathfinder_boolean::get_turn_penalty() {
    return turn_penalty;
}

//return the cost in seconds from source to destination
double Pathfinder_boolean::get_final_cost() {
    return destination->get_cost_to_node();
}
