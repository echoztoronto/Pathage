/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "m4.h"
#include "traveling_courier.h"
#include "pathfinder.h"
#include <chrono>
#include <thread>

void multi_start(const std::vector<DeliveryInfo>& deliveries, 
        std::vector<unsigned> valid_depots, 
        unsigned depot_to_start_at, 
        Traveling_courier** results);

std::vector<unsigned> traveling_courier(const std::vector<DeliveryInfo>& deliveries, 
        const std::vector<unsigned>& depots, 
        const float turn_penalty) {
    
    //assume computing the final cost takes 40ms / path segment (intersection to intersection)
    const double search_time_limit = 28.0 - deliveries.size()*0.08;
    
    //keep track of time and stop when we've hit our limit
    std::chrono::time_point<std::chrono::system_clock> start_time, current_time;
    start_time = std::chrono::system_clock::now();
    current_time = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = current_time - start_time;
    
    //check which depots are possible to get to
    //assumes that all delivery points are connected, so using any will do
    std::vector<unsigned> valid_depots;
    
    for (auto i = depots.begin(); i != depots.end(); i++) {
        Pathfinder_boolean test_depot(*i, deliveries[0].pickUp, 0.0);
        if (test_depot.found_path) {
            valid_depots.push_back(*i);
        }
    }
    
    //use multi-start to get multiple results - start at each available depot
    unsigned next_depot_to_start_at = 0;
    
    //may end up with no results, so have something we can check against
    Traveling_courier* best_results = NULL;
    double lowest_cost = 0;
    
    //while within time limit and while still have depots to search through
    
    while (elapsed_seconds.count() < search_time_limit && next_depot_to_start_at < valid_depots.size()) {
        /* This atrocity of coding style is because threads in for loops will be scoped out and crash*/
        
        std::vector<Traveling_courier*> thread_results;
        
        //run four threads
        if (valid_depots.size() - next_depot_to_start_at > 3) {
            thread_results.resize(4);
            
            std::thread t1(multi_start, deliveries, valid_depots, next_depot_to_start_at, &(thread_results[0]));
            next_depot_to_start_at++;
            
            std::thread t2(multi_start, deliveries, valid_depots, next_depot_to_start_at, &(thread_results[1]));
            next_depot_to_start_at++;
            
            std::thread t3(multi_start, deliveries, valid_depots, next_depot_to_start_at, &(thread_results[2]));
            next_depot_to_start_at++;
            
            thread_results[3] = new Traveling_courier(deliveries, valid_depots, next_depot_to_start_at);
            next_depot_to_start_at++;
            
            t1.join();
            t2.join();
            t3.join();     
        }
        //run three threads
        else if (valid_depots.size() - next_depot_to_start_at == 3) {
            thread_results.resize(3);
            
            std::thread t1(multi_start, deliveries, valid_depots, next_depot_to_start_at, &(thread_results[0]));
            next_depot_to_start_at++;
            
            std::thread t2(multi_start, deliveries, valid_depots, next_depot_to_start_at, &(thread_results[1]));
            next_depot_to_start_at++;
            
            thread_results[2] = new Traveling_courier(deliveries, valid_depots, next_depot_to_start_at);
            next_depot_to_start_at++;
            
            t1.join();
            t2.join(); 
        }
        //run two threads
        else if (valid_depots.size() - next_depot_to_start_at == 2) {
            thread_results.resize(2);
            
            std::thread t1(multi_start, deliveries, valid_depots, next_depot_to_start_at, &(thread_results[0]));
            next_depot_to_start_at++;
            
            thread_results[1] = new Traveling_courier(deliveries, valid_depots, next_depot_to_start_at);
            next_depot_to_start_at++;
            
            t1.join();
        }
        //run one thread (the main thread)
        else {
            thread_results.resize(1);
            
            thread_results[0] = new Traveling_courier(deliveries, valid_depots, next_depot_to_start_at);
            next_depot_to_start_at++;
        }
        
        for (unsigned i = 0; i < thread_results.size(); i++) {
            if (best_results == NULL || thread_results[i]->cost < lowest_cost) {
                if (best_results != NULL) {
                    delete best_results;
                }
                best_results = thread_results[i];
                lowest_cost = best_results->cost;
            }
            else {
                delete thread_results[i];
            }
        }        
    }
    
    //this is single-threaded for now - will convert to multithreaded once sure that it works
    while (elapsed_seconds.count() < search_time_limit && next_depot_to_start_at < valid_depots.size()) {
        //check a new path and compare results to current results
        Traveling_courier* current_path = new Traveling_courier(deliveries, valid_depots, next_depot_to_start_at);
        next_depot_to_start_at++; //next depot please
        
        if (best_results == NULL || (current_path->found_path && current_path->cost < lowest_cost) ) {
            best_results = current_path;
        }
        else {
            delete current_path; //clean up dynamically allocated path
        }
        current_time = std::chrono::system_clock::now();
        elapsed_seconds = current_time - start_time;
    }
    
    //use a list for final path for ease of insertion and saving ourselves from converting each time
    std::list<unsigned> street_segments_path;
    
    //if some of the delivery points aren't connected, we'll get no path for a path segment
    bool encountered_invalid_segment = false;
    
    //will return empty vector if no results
    //otherwise, find the detailed (segment-level) path for the journey
    if (best_results != NULL) {
        for (unsigned i = 0; i < best_results->delivery_order.size() - 1; i++) {
            Pathfinder current_leg(best_results->delivery_order[i], 
                    best_results->delivery_order[i+1], turn_penalty);
            if (current_leg.get_results().empty()) {
                encountered_invalid_segment = true;
            }
            street_segments_path.insert(street_segments_path.end(), current_leg.get_segment_results().begin(), current_leg.get_segment_results().end());
            //std::cout << "Going from intersection " << best_results->delivery_order[i] << " to interesection " << best_results->delivery_order[i+1] << std::endl;
        }
    }
    
    //clean up
    delete best_results;
    
    if (encountered_invalid_segment) {
        std::vector<unsigned> no_results;
        return no_results;
    }
    
    //convert to vector
    std::vector<unsigned> segments_vector(street_segments_path.begin(), street_segments_path.end());
    return segments_vector;
}

void multi_start(const std::vector<DeliveryInfo>& deliveries, 
        std::vector<unsigned> valid_depots, 
        unsigned depot_to_start_at, 
        Traveling_courier** results) {
    //check a new path and compare results to current results
    *results = new Traveling_courier(deliveries, valid_depots, depot_to_start_at);
}