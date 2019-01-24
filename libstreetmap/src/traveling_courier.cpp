/*
 * Implementation file for the Traveling_courier class.
 */

#include "traveling_courier.h"

Traveling_courier::Traveling_courier(const std::vector<DeliveryInfo>& deliveries, 
        const std::vector<unsigned>& depots, unsigned starting_depot) {
    
    //initialize multimap of intersection IDs to things at the intersection
    for (auto i = deliveries.begin(); i != deliveries.end(); i++) {
        Delivery current_delivery(i->pickUp, i->dropOff); 
        
        pickups.insert({i->pickUp, current_delivery});
        
        intersection_to_deliveries.insert({i->pickUp, current_delivery});
        intersection_to_deliveries.insert({i->dropOff, current_delivery});
        
        possible_intersections.insert(i->pickUp);
    }
    
    node_cost.clear();
    delivery_order = greedy_search(depots, starting_depot);

    //Pertubation of path for better results
    bool better_path_available = false;
    
//    better_path_available = pertubate_path_2_opt(greedy_result, cost);
    
//    prep_possible_pairs();
//    //If no possible/ or only 1 pairs we dont bother
//    if (possible_pair_swaps.size() >= 2) {
//        better_path_available = pertubate_path_pair_swap(greedy_result, cost);
//    }
    
//    for (int i = 0; i < 10; i++) {
//        better_path_available = false;
        prep_node_for_permute(delivery_order);
        //If didnt change from initial values we dont bother
        if (!(node_to_swap_pos == 0 && closest_pickup_pos == 0 && closest_dropoff_pos == 0) && 
                !((closest_dropoff_pos - closest_pickup_pos) <= 2)) {
            better_path_available = pertubate_path_selection_swap(delivery_order, cost);
        }
        if (better_path_available) {
            delivery_order = new_intersection_ids;
        } 
//        else {
//            break;
//        }
//    }
    
}

Traveling_courier::~Traveling_courier() {
    ;
}


bool Traveling_courier::pertubate_path_2_opt(std::vector<unsigned> given_intersection_ids, double current_cost, bool check_legality) {
    
    if (given_intersection_ids.size() <= 2) {
        return false;
    }
    
    bool path_improved = false;
    bool done = false;
    unsigned start = 0;
    unsigned end = 1;
    int loop_count = 0;
    int max_count = 100000;
    
    while(!done) {
        
        //Permute path
        bool compute = opt_2_permute(given_intersection_ids, start, end); //Continue only if permuted
        
        if (compute && check_legality) { 
            //Check legality
            compute = check_path_legal();
        }
        
        if (compute) {
            //Reset cost
            double new_cost = 0.0;

            //Compute time
            new_cost = cost_of_new_path();

            //Compare if better
            if (new_cost < current_cost) {
                current_cost = new_cost;
                path_improved = true;
                new_intersection_ids = temp_intersection_ids;
            }
        }

        //Update start and end int
        if (end >= given_intersection_ids.size() - 1) {
            end = start + 2;
            start += 1;
        } else {
            end++;
        }
        
        loop_count++;

        //Update done boolean
        if (start >= given_intersection_ids.size() - 1 || loop_count >= max_count) {
            done = true;
        }
    }
    
    //If a better solution is found we make known
    cost = current_cost;
    return path_improved;
}

//Returns false if nothing permuted
bool Traveling_courier::opt_2_permute(std::vector<unsigned> given_intersection_ids, unsigned start, unsigned end) {
    
    if (end <= start || end > given_intersection_ids.size()) {
        return false;
    }
    
    temp_intersection_ids = given_intersection_ids;
    
    for (unsigned i = 0; i < given_intersection_ids.size(); i++) {
        if (i >= start && i <= end) {
            temp_intersection_ids[i] = given_intersection_ids[end - (i - start)];
        }
    }
    
    return true;
}
        
bool Traveling_courier::check_path_legal() {
    return true;
}

double Traveling_courier::cost_of_new_path() {
    
    double total_cost = 0.0;
    
    for (unsigned i = 0; i < temp_intersection_ids.size() - 1; i++) {
        total_cost += find_distance_between_two_points(getIntersectionPosition(temp_intersection_ids[i]), getIntersectionPosition(temp_intersection_ids[i+1])); 
    }
    return total_cost;
}
        
std::vector<unsigned> Traveling_courier::greedy_search(const std::vector<unsigned>& depots, unsigned starting_depot_index) {
    
    //vectors for remaining stuff
    std::vector<unsigned> remaining_pickup;
    std::vector<unsigned> remaining_dropoff;
    //a map to look up dropoff id when pickup is given
    std::unordered_map<unsigned, unsigned> pickup_to_dropoff;
    //intersection ids for closest positions
    unsigned closest_pickup;
    unsigned closest_ending_depot;
    //smallest cost
    double smallest_pickup_distance = 0.0;
    double smallest_ending_depot_distance = 0.0;
    double current_path_cost = 0.0;
    //current position's intersection id
    unsigned current_intersection;
    //the result vector we want: contains starting depot, pickups, dropoffs and ending depot (intersection ids)
    std::vector<unsigned> result;
    
    //A. start from the given depot 
    current_intersection = depots[starting_depot_index];
    result.push_back(current_intersection);
    node_cost.push_back(0.0); //Dummy value for the cost to reach the depot, needed to align this vector to the path
    
    //B. visit pickups and dropoffs
    //loop while there are remaining pickups & dropoffs
    //3 steps 
    while (!possible_intersections.empty()) {
        smallest_pickup_distance = 0;
        
        //Loops through all the possible intersection in the set and find the shortest distance
        for (auto it = possible_intersections.begin();  it != possible_intersections.end(); ++it) {
            double temp_distance = find_distance_between_two_points(getIntersectionPosition(current_intersection), getIntersectionPosition(*it));
            if (smallest_pickup_distance == 0 || temp_distance < smallest_pickup_distance) {
                smallest_pickup_distance = temp_distance;
                closest_pickup = *it;
            }     
        }
        
        //Remove visited intersection from set
        possible_intersections.erase(closest_pickup);
        
        //set this pickup to the current position
        current_intersection = closest_pickup;
        //Add to results
        result.push_back(closest_pickup);
        current_path_cost += smallest_pickup_distance;
        node_cost.push_back(smallest_pickup_distance);
                        
        //Update the multimap of the intersection we visited
        auto to_be_updated = intersection_to_deliveries.equal_range(closest_pickup);
        for (auto it = to_be_updated.first; it != to_be_updated.second; it++) {
            //if pickup, toggle picked_up to true, add drop off intersection to the set
            if (it->second.pickup_intersection == closest_pickup) {
                it->second.picked_up = true;
                possible_intersections.insert(it->second.dropoff_intersection);
            } 
            if (it->second.dropoff_intersection == closest_pickup) {
            //if drop off dropped off to true
                if (it->second.picked_up) {
                    it->second.dropped_off = true;
                }            
            }
        }
    }
    
    //C. choose a depot to end
    //loop through all depot to find the closest one
    for (unsigned i = 0; i < depots.size(); i++) {
        double temp_distance = find_distance_between_two_points(getIntersectionPosition(current_intersection), getIntersectionPosition(depots[i]));
        
        if (smallest_ending_depot_distance == 0 ||temp_distance < smallest_ending_depot_distance) {
            smallest_ending_depot_distance = temp_distance;
            closest_ending_depot = depots[i];
        }
    }
    current_path_cost += smallest_ending_depot_distance;
//    node_cost.push_back(smallest_ending_depot_distance);
    cost = current_path_cost;
    //add closest depot intersection id into the result vector
    result.push_back(closest_ending_depot);
    
    // a vector contains starting depot, pickups, dropoffs and ending depot (intersection ids)
    return result;
}

bool Traveling_courier::pertubate_path_pair_swap(std::vector<unsigned> given_intersection_ids, double current_cost) {
    
    if (given_intersection_ids.size() <= 2) {
        return false;
    }
    
    bool path_improved = false;
    bool done = false;
    int loop_count = 0;
    int max_count = 100000;
    unsigned first_pair = 0;
    unsigned second_pair = 1;
    
    //In while loop
        //Choose 2 DeliveryInfo from deliveries
        //Find their positions in given_intersection_ids, swap with 
    
                // assuming your vector is called v
                //iter_swap(v.begin() + position, v.begin() + nextPosition);
                // position, nextPosition are the indices of the elements you want to swap
                
        //Compute cost, etc etc
    
    while(!done) {
        //Permute path
        bool compute = pair_swap_permute(given_intersection_ids, possible_pair_swaps[first_pair], possible_pair_swaps[second_pair]); //Continue only if permuted
        
        if (compute) {
            //Reset cost
            double new_cost = 0.0;

            //Compute time
            new_cost = cost_of_new_path();

            //Compare if better
            if (new_cost < current_cost) {
                current_cost = new_cost;
                path_improved = true;
                new_intersection_ids = temp_intersection_ids;
            }
        }

        //Update start and end int
        if (second_pair >= possible_pair_swaps.size() - 1) {
            first_pair += 1;
            second_pair = first_pair + 1;
        } else {
            second_pair++;
        }
        
        loop_count++;

        //Update done boolean
        if (first_pair >= possible_pair_swaps.size() - 1 || loop_count >= max_count) {
            done = true;
        }
    }
    
    //If a better solution is found we make known
    cost = current_cost;
    return path_improved;
}

bool Traveling_courier::pair_swap_permute(std::vector<unsigned> given_intersection_ids, DeliveryInfo pair_1, DeliveryInfo pair_2) {
    
    temp_intersection_ids = given_intersection_ids;
    
    auto pair_1_pickup = std::find(temp_intersection_ids.begin(), temp_intersection_ids.end(), pair_1.pickUp);
    auto pair_1_dropoff = std::find(temp_intersection_ids.begin(), temp_intersection_ids.end(), pair_1.dropOff);
    auto pair_2_pickup = std::find(temp_intersection_ids.begin(), temp_intersection_ids.end(), pair_2.pickUp);
    auto pair_2_dropoff = std::find(temp_intersection_ids.begin(), temp_intersection_ids.end(), pair_2.dropOff);
    
    if (pair_1_pickup == pair_2_pickup && pair_1_dropoff == pair_2_dropoff) {
        return false;
    }
    
    std::iter_swap(pair_1_pickup, pair_2_pickup);
    std::iter_swap(pair_1_dropoff, pair_2_dropoff);
    
    return true;
}

void Traveling_courier::prep_possible_pairs() {
    //Look through the multimap of only pickups
    for (auto it = pickups.begin(); it != pickups.end();) {
        //We count how many times the intersection id of current pick up appears
        //We only swap one-to-one pairs so if theres more than one entry its not 1-1
        int count = pickups.count(it->first);
        if (count == 1) {
            //If pick up intersection only appears once, means we have a singular pickup at that intersection
            //Now we make sure the drop off point only drops off this one package
            if (it->first == it->second.pickup_intersection) {
                count = intersection_to_deliveries.count(it->second.dropoff_intersection);
            } else {
                count = intersection_to_deliveries.count(it->second.pickup_intersection);                
            }
            //Drop off intersection only appears once, means only this one particular drop off and no other pickups either
            //Then it's available for swapping
            if (count == 1) {                
                DeliveryInfo pair(it->second.pickup_intersection, it->second.dropoff_intersection);
                possible_pair_swaps.push_back(pair);
            }
            it++;
        } else {
            std::advance(it, count);
        }
    }
}

void Traveling_courier::prep_node_for_permute(std::vector<unsigned> given_intersection_ids) {
    
    if (given_intersection_ids.size() <= 1) {
        return;
    }
    
    //Find worst node
    double worst_cost = 0.0;
    for (unsigned i = 0; i < node_cost.size(); i++) {
        if (node_cost[i] > worst_cost) {
            worst_cost = node_cost[i];
            node_to_swap_pos = i;
        }
    }
    
    //Find the range we can swap it in
    unsigned worst_intersection_id = given_intersection_ids[node_to_swap_pos];
    auto all_deliveries_at_intersection = intersection_to_deliveries.equal_range(worst_intersection_id);
    //Looks through all the deliveries and find the closest pickup and dropoff, which are the limits of the swap
    for (auto it = all_deliveries_at_intersection.first; it != all_deliveries_at_intersection.second; it++) {
        
        unsigned other_end_point;
        if (it->second.pickup_intersection == it->first) {
            other_end_point = it->second.dropoff_intersection;
        } else {
            other_end_point = it->second.pickup_intersection;
        }
        
        //Assuming other point always can be found
        auto other_pos_it = std::find(given_intersection_ids.begin(), given_intersection_ids.end(), other_end_point);
        unsigned other_pos = other_pos_it - given_intersection_ids.begin();
        //If it came before the node to swap, it is a pickup and drops off at the node we are moving
        if (other_pos < node_to_swap_pos) {
            if (other_pos > closest_pickup_pos) {
                closest_pickup_pos = other_pos; //Our lower bound
            }
        } else if (other_pos > node_to_swap_pos) { //If it came after, it is a drop off for the pickup at the node we are moving
            if (other_pos < closest_dropoff_pos || closest_dropoff_pos == 0) {
                closest_dropoff_pos = other_pos; //Our upper bound
            }
        }
    }
    
    //If node is only a drop off, then we need to set the upper bound manually to node_cost.size()
    //This corresponds to the position of the ending depot in the greedy path
    //Upper bound is not included in the available swap range
    if (closest_dropoff_pos == 0) {
        closest_dropoff_pos = node_cost.size();
    }
    
    //Technically the same should be done for pick up, if our worst node is only drop off the lower bound should be
    //0, which corresponds to the starting depot
    //We don't have to do that since we initialized the value to 0
    //Lower bound is not included in the available swap range
}

bool Traveling_courier::pertubate_path_selection_swap(std::vector<unsigned> given_intersection_ids, double current_cost) {
    
    if (given_intersection_ids.size() <= 2) {
        return false;
    }
    
    bool path_improved = false;
    bool done = false;
    int loop_count = 0;
    int max_count = 100000;
    unsigned new_pos = closest_pickup_pos + 1;
    
    while(!done) {
        //Permute path
        bool compute = selection_swap_permute(given_intersection_ids, new_pos); //Continue only if permuted
        
        if (compute) {
            //Reset cost
            double new_cost = 0.0;

            //Compute time
            new_cost = cost_of_new_selection_path();

            //Compare if better
            if (new_cost < current_cost) {
                current_cost = new_cost;
                path_improved = true;
                new_intersection_ids = temp_intersection_ids;
            }
        }

        //Update new_pos
        new_pos++;
        loop_count++;

        //Update done boolean
        if (new_pos >= closest_dropoff_pos - 1 || loop_count >= max_count) {
            done = true;
        }
    }
    
    //If a better solution is found we make known
    cost = current_cost;
    return path_improved;
    
}

bool Traveling_courier::selection_swap_permute(std::vector<unsigned> given_intersection_ids, unsigned new_pos) {
    
    if (new_pos == node_to_swap_pos) {
        return false;
    }
    
    temp_intersection_ids = given_intersection_ids;
    
    //Hold on to this
    unsigned moving_node = temp_intersection_ids[node_to_swap_pos];
    double moving_node_cost = node_cost[node_to_swap_pos];
    
    //Depending on whether if we are shifting forwards or backwards the for loops are different
    if (new_pos < node_to_swap_pos) { //Shifting to earlier in the path
        for (unsigned i = node_to_swap_pos; i > new_pos; i--) { //Going backwards and bringing each element one spot forwards
            temp_intersection_ids[i] = temp_intersection_ids[i-1];
            node_cost[i] = node_cost[i-1];
        }
    } else { //Shifting to later in the path
        for (unsigned i = node_to_swap_pos; i < new_pos; i++) { //Going forwards and bringing each element one spot backwards
            temp_intersection_ids[i] = temp_intersection_ids[i+1];
            node_cost[i] = node_cost[i+1];
        }
    }
    
    temp_intersection_ids[new_pos] = moving_node;
    node_cost[new_pos] = moving_node_cost;
    
    return true;
}

double Traveling_courier::cost_of_new_selection_path() {
    
    double total_cost = 0.0;
    
    for (unsigned i = 0; i < temp_intersection_ids.size() - 1; i++) {
        double cur_node_cost = find_distance_between_two_points(getIntersectionPosition(temp_intersection_ids[i]), getIntersectionPosition(temp_intersection_ids[i+1])); 
        total_cost += cur_node_cost;
        if (i >= closest_pickup_pos && i <= closest_dropoff_pos) {
            node_cost[i] = cur_node_cost;
        }
    }
    return total_cost;
}