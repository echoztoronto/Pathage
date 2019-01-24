/*
 * The Delivery class describes a single pickup and drop off point, and the package's current state.
 * The Traveling_courier class implements the search for the path used to pick up and deliver the specified Deliveries.
 */

#ifndef TRAVELING_COURIER_H
#define TRAVELING_COURIER_H

#include "pathfinder.h"
#include "m4.h"
#include <unordered_map>
#include <algorithm>

class Delivery {
public:
    unsigned pickup_intersection;
    unsigned dropoff_intersection;
    bool picked_up = false;
    bool dropped_off = false;
    Delivery(unsigned pickup_intersection_, unsigned dropoff_intersection_) {
        pickup_intersection = pickup_intersection_;
        dropoff_intersection = dropoff_intersection_;
    }
};

class Traveling_courier {
public:
    bool found_path = false;
    //total, final cost (heuristically), probably in distance
    double cost = 0.0;
    
    std::unordered_multimap<unsigned, Delivery> intersection_to_deliveries;
    std::set<unsigned> possible_intersections;
    
    //intersection ids? change to something else if convenient
    //please include starting and ending depot in this!
    std::vector<unsigned> delivery_order;
    
    //starting_depot is an index for vector depots
    //feel free to assume that starting_depot is a valid index
    //greedy search + set number of permutations?
    Traveling_courier(const std::vector<DeliveryInfo>& deliveries, 
        const std::vector<unsigned>& depots, unsigned starting_depot);
    ~Traveling_courier();    
    
    void permute_results();
    
    
    bool check_path_legal();
    double cost_of_new_path();
    double cost_of_new_selection_path();
    
    std::vector<unsigned> temp_intersection_ids;
    std::vector<unsigned> new_intersection_ids;
    bool pertubate_path_2_opt(std::vector<unsigned> given_intersection_ids, double current_cost, bool check_legality = false);
    bool opt_2_permute(std::vector<unsigned> given_intersection_ids, unsigned start, unsigned end);
    
    std::unordered_multimap<unsigned, Delivery> pickups;
    std::vector<DeliveryInfo> possible_pair_swaps;
    void prep_possible_pairs();
    bool pertubate_path_pair_swap(std::vector<unsigned> given_intersection_ids, double current_cost);
    bool pair_swap_permute(std::vector<unsigned> given_intersection_ids, DeliveryInfo pair_1, DeliveryInfo pair_2);
    
    std::vector<double> node_cost;
    unsigned node_to_swap_pos = 0;
    unsigned closest_pickup_pos = 0;
    unsigned closest_dropoff_pos = 0;
    void prep_node_for_permute(std::vector<unsigned> given_intersection_ids);
    bool pertubate_path_selection_swap(std::vector<unsigned> given_intersection_ids, double current_cost);
    bool selection_swap_permute(std::vector<unsigned> given_intersection_ids, unsigned new_pos);
    
    //generate a "greedy" path (intersection ids) by giving a DeliveryInfo, several depots, a certain starting depot, and turn penalty
    std::vector<unsigned> greedy_search(const std::vector<unsigned>& depots, unsigned starting_depot_index);
};

#endif /* TRAVELING_COURIER_H */

