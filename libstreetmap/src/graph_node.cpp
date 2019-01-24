/*
 * Implementation file for the Graph_node class
 */

#include "graph_node.h"

extern Map_data const *map_data;

/****************************************************************************************
                                  Graph_node implementation
 ****************************************************************************************/

Graph_node::Graph_node (unsigned id_){
    id = id_;
    position = getIntersectionPosition(id);
}

Graph_node::~Graph_node(){
    //nada
}

//set this node as the start of the graph
void Graph_node::set_as_origin() {
    is_origin = true;
}

//set this node as a destination
void Graph_node::set_as_destination() {
    is_destination = true;
}

//mark visited, mark source, return whether visit is least-cost
bool Graph_node::visit_node(Graph_node* source_, unsigned source_segment_, double cost_to_node_) {
    if (!visited) {
        visited = true;
        source = source_;
        source_segment = source_segment_;
        cost_to_node = cost_to_node_;
        return true;
    }
    //compare paths if already visited
    else { 
        if (cost_to_node_ < cost_to_node) {
            source = source_;
            source_segment = source_segment_;
            cost_to_node = cost_to_node_;
            return true;
        }
        else {
            return false;
        }
    }
}

bool Graph_node::get_is_origin() {
    return is_origin;
}

bool Graph_node::get_is_destination() {
    return is_destination;
}

//return whether visited or not
bool Graph_node::get_visited() {
    return visited;
}

//return source node
Graph_node* Graph_node::get_from_intersection() {
    return source;
}

unsigned Graph_node::get_from_segment() {
    return source_segment;
}

unsigned Graph_node::get_id() {
    return id;
}

LatLon Graph_node::get_position() {
    return position;
}

double Graph_node::get_cost_to_node() {
    return cost_to_node;
}

/****************************************************************************************
                                  Path_option implementation
 ****************************************************************************************/

Path_option::Path_option(double origin_to_source_cost, Graph_node* source_node_, 
        Graph_node* next_node_, Graph_node* destination_, 
        unsigned source_to_next_segment_, double turn_penalty) {
    source_node = source_node_;
    next_node = next_node_;
    destination = destination_;
    source_to_next_segment = source_to_next_segment_;
    
    origin_to_next_cost = origin_to_source_cost;    
    
    //don't add a turn penalty to the starting leg of the journey
    //two cases: initial Path_option (NULL to origin) and moving from the origin
    if (source_node != NULL && !source_node->get_is_origin()) {        
        //add the turn penalty if the streets are different
        unsigned street_id1 = getStreetSegmentInfo(source_node->get_from_segment()).streetID;
        unsigned street_id2 = getStreetSegmentInfo(source_to_next_segment).streetID;
        if (street_id1 != street_id2) {
            origin_to_next_cost += turn_penalty;
        }
    }
    
    if (source_node != NULL) {
        //add the cost to travel on the street
        origin_to_next_cost += map_data->get_travel_time_from_street_segment_id(source_to_next_segment);
    }
        
    //theoretical cost for the remainder of the journey
    predicted_next_to_destination_cost = calculate_heuristic();

    total_cost = origin_to_next_cost + predicted_next_to_destination_cost;
}

Path_option::~Path_option() {
    //sit back and relax
}

double Path_option::calculate_heuristic() {
    /*
     * 100km/h in m/s is about 27.7, so 30.0 should be faster than any real roads
     * In theory
     * Good thing we're not in Germany.     
     */
    
    //return time in seconds to cover remaining straight-line distance at max speed
    return find_distance_between_two_points(next_node->get_position(), destination->get_position()) / 30.0;
}

//I don't think I used this but I'll leave it here for now
std::vector<unsigned> Path_option::get_connected_street_segments(unsigned intersection_id) {
    std::vector<unsigned> connected_segments;
    
    unsigned num_connected_segments = getIntersectionStreetSegmentCount(intersection_id);
    
    connected_segments.resize(num_connected_segments);
    for (unsigned i = 0; i < num_connected_segments; i++) {
        connected_segments[i] = getIntersectionStreetSegment(intersection_id, i);
    }
    
    return connected_segments;
}

/****************************************************************************************
                                  Multi_Path_option implementation
 ****************************************************************************************/

Multi_Path_option::Multi_Path_option(double origin_to_source_cost, Graph_node* source_node_, 
        Graph_node* next_node_, std::vector<Graph_node*>& destinations, 
        unsigned source_to_next_segment_, double turn_penalty) {
    source_node = source_node_;
    next_node = next_node_;
    source_to_next_segment = source_to_next_segment_;
    
    origin_to_next_cost = origin_to_source_cost;    
    
    //don't add a turn penalty to the starting leg of the journey
    //two cases: initial Multi_Path_option (NULL to origin) and moving from the origin
    if (source_node != NULL && !source_node->get_is_origin()) {        
        //add the turn penalty if the streets are different
        unsigned street_id1 = getStreetSegmentInfo(source_node->get_from_segment()).streetID;
        unsigned street_id2 = getStreetSegmentInfo(source_to_next_segment).streetID;
        if (street_id1 != street_id2) {
            origin_to_next_cost += turn_penalty;
        }
    }
    
    if (source_node != NULL) {
        //add the cost to travel on the street
        origin_to_next_cost += map_data->get_travel_time_from_street_segment_id(source_to_next_segment);
    }        
    
    predicted_next_to_destination_cost = calculate_heuristic(destinations);

    total_cost = origin_to_next_cost + predicted_next_to_destination_cost;
}

Multi_Path_option::~Multi_Path_option() {
    //sit back and relax
}

double Multi_Path_option::calculate_heuristic(std::vector<Graph_node*>& destinations) {
    /*
     * 100km/h in m/s is about 27.7, so 30.0 should be faster than any real roads
     * In theory
     * Good thing we're not in Germany.     
     */
    
    double min_cost = 0.0;
    
    //compare remaining ideal cost to all destinations, and return the best one
    for (auto i = destinations.begin(); i != destinations.end(); i++) {
        double current_destination_cost = find_distance_between_two_points(next_node->get_position(), (*i)->get_position()) / 30.0;
        if (i == destinations.begin() || current_destination_cost < min_cost) {
            min_cost = current_destination_cost;
        }
    }
    
    return min_cost;
}

/****************************************************************************************
                                  Helper function
 ****************************************************************************************/

unsigned get_best_segment(bool has_previous_segment, 
        unsigned previous_segment_id, unsigned from_intersection_id, 
        unsigned to_intersection_id, double turn_penalty) {
    
    //find all segments that connect the two intersections
    std::vector<unsigned> from_segments = map_data->get_street_segment_ids_from_intersection_id(from_intersection_id);
    std::vector<unsigned> to_segments = map_data->get_street_segment_ids_from_intersection_id(to_intersection_id);
       
    std::vector<unsigned> candidate_segments;
    
    //find common segments and check for direction legality
    for (auto from = from_segments.begin(); from != from_segments.end(); from++) {
        for (auto to = to_segments.begin(); to != to_segments.end(); to++) {
            if (*from == *to) {
                StreetSegmentInfo seg_info = getStreetSegmentInfo(*from);
                bool moving_forwards = false;
                if (seg_info.from == from_intersection_id) {
                    moving_forwards = true;
                }
                if (!seg_info.oneWay || moving_forwards) {
                    candidate_segments.push_back(*from);
                }
            }
        }
    }
    
    //don't bother comparing one segment to itself
    if (candidate_segments.size() == 1) {
        return candidate_segments[0];
    }
    //compute travel times and turn penalties
    else {
        unsigned previous_street_id = 0;
        if (has_previous_segment) {
            previous_street_id = getStreetSegmentInfo(previous_segment_id).streetID;
        }

        auto segment_it = candidate_segments.begin();
        
        //set current best to first candidate
        unsigned best_segment_id = *segment_it;
        double best_time = map_data->get_travel_time_from_street_segment_id(*segment_it);
        if (has_previous_segment) {
            unsigned street_id = getStreetSegmentInfo(*segment_it).streetID;
            if (previous_street_id != street_id) {
                best_time += turn_penalty;
            }
        }
        segment_it++;
        
        //check the rest of the segments
        for ( ; segment_it != candidate_segments.end(); segment_it++) {
            double current_segment_time = map_data->get_travel_time_from_street_segment_id(*segment_it);
            if (has_previous_segment) {
                unsigned street_id = getStreetSegmentInfo(*segment_it).streetID;
                if (previous_street_id != street_id) {
                    current_segment_time += turn_penalty;
                }
            }
            if (current_segment_time < best_time) {
                best_segment_id = *segment_it;
                best_time = current_segment_time;
            }
        }

        return best_segment_id;
    }
}

unsigned get_any_segment(unsigned from_intersection_id, unsigned to_intersection_id) {
    
    //find all segments that connect the two intersections
    std::vector<unsigned> from_segments = map_data->get_street_segment_ids_from_intersection_id(from_intersection_id);
    std::vector<unsigned> to_segments = map_data->get_street_segment_ids_from_intersection_id(to_intersection_id);
           
    //find common segments and check for direction legality
    //return the first one that works
    for (auto from = from_segments.begin(); from != from_segments.end(); from++) {
        for (auto to = to_segments.begin(); to != to_segments.end(); to++) {
            if (*from == *to) {
                StreetSegmentInfo seg_info = getStreetSegmentInfo(*from);
                bool moving_forwards = false;
                if (seg_info.from == from_intersection_id) {
                    moving_forwards = true;
                }
                if (!seg_info.oneWay || moving_forwards) {
                    return(*from);
                }
            }
        }
    }
    
    return 0; //if no segment connects the two, but theoretically we should never get here
    
}