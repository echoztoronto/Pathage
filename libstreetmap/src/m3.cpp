/*
 * Implementation file for the milestone 3 functions
 */

#include "m3.h"
#include "pathfinder.h"
#include "map_data.h"

extern Map_data const *map_data;

bool path_is_legal(const unsigned start_intersection, const unsigned end_intersection, const std::vector<unsigned>& path);
bool path_is_legal(const unsigned start_intersection, const std::vector<unsigned>& end_intersections, const std::vector<unsigned>& path);

// Returns the time required to travel along the path specified, in seconds. 
// The path is given as a vector of street segment ids, and this function 
// can assume the vector either forms a legal path or has size == 0.
// The travel time is the sum of the length/speed-limit of each street 
// segment, plus the given turn_penalty (in seconds) per turn implied by the path. 
// A turn occurs when two consecutive street segments have different street IDs.
double compute_path_travel_time(const std::vector<unsigned>& path, const double turn_penalty) {
    if (path.empty()) {
        return 0.0;
    }
    
    double total_cost = 0.0;
    
    unsigned previous_street_id;
    unsigned current_street_id;
    
    //treat the first segment differently since it has no segment before it
    auto segment = path.begin();
    
    StreetSegmentInfo first_segment = getStreetSegmentInfo(*segment);
    previous_street_id = first_segment.streetID;
    
    total_cost += map_data->get_travel_time_from_street_segment_id(*segment);
    
    segment++;
    
    for ( ; segment != path.end(); segment++) {
        StreetSegmentInfo current_segment_info = getStreetSegmentInfo(*segment);
        current_street_id = current_segment_info.streetID;
        
        //add the turn penalty if the streets are different
        if (previous_street_id != current_street_id) {
            total_cost += turn_penalty;
        }   

        //add the cost to travel on the upcoming segment
        total_cost += map_data->get_travel_time_from_street_segment_id(*segment);
        previous_street_id = current_street_id;
    }
       
    return total_cost;
}

// Returns a path (route) between the start intersection and the end 
// intersection, if one exists. This routine should return the shortest path
// between the given intersections when the time penalty to turn (change
// street IDs) is given by turn_penalty (in seconds).
// If no path exists, this routine returns an empty (size == 0) vector. 
// If more than one path exists, the path with the shortest travel time is 
// returned. The path is returned as a vector of street segment ids; traversing 
// these street segments, in the returned order, would take one from the start 
// to the end intersection.
std::vector<unsigned> find_path_between_intersections(const unsigned intersect_id_start, 
        const unsigned intersect_id_end, const double turn_penalty) {
    
    Pathfinder route(intersect_id_start, intersect_id_end, turn_penalty);
    std::list<unsigned> segment_ids_list = route.get_segment_results();
    
    std::vector<unsigned> segment_ids(segment_ids_list.begin(), segment_ids_list.end());
        
    return segment_ids;
}

// Returns the shortest travel time path (vector of street segments) from 
// the start intersection to a point of interest with the specified name.
// The path will begin at the specified intersection, and end on the 
// intersection that is closest (in Euclidean distance) to the point of 
// interest.
// If no such path exists, returns an empty (size == 0) vector.
std::vector<unsigned> find_path_to_point_of_interest(const unsigned intersect_id_start, 
        const std::string point_of_interest_name, const double turn_penalty) {
    
    //find sufficiently close POI
    LatLon position = getIntersectionPosition(intersect_id_start);
    std::vector<unsigned> candidate_pois = map_data->get_nearby_point_of_interest(position, point_of_interest_name);
    
    //no such path exists case
    if (candidate_pois.size() == 0) {
        std::vector<unsigned> path;
        return path;
    }
    
    //get the closest intersection to each candidate POI, using a set to remove duplicates
    std::set<unsigned> destination_intersections_set;
    for (auto pit = candidate_pois.begin(); pit != candidate_pois.end(); pit++) {
        LatLon current_poi_location = getPointOfInterestPosition(*pit);
        unsigned current_poi_intersection = map_data->get_closest_intersection(current_poi_location);
        destination_intersections_set.insert(current_poi_intersection);
    }
    std::vector<unsigned> destination_intersections(destination_intersections_set.begin(), destination_intersections_set.end());
    
    Multi_Pathfinder path_to_poi(intersect_id_start, destination_intersections, turn_penalty);
    
    std::list<unsigned> list_path = path_to_poi.get_segment_results();
    
    std::vector<unsigned> path(list_path.begin(), list_path.end());
    return path;    
}