/*
 * Implementation file for the Map_data class that holds data loaded from map bin file
 * Property of CD-026, do not redistribute
 */

#include <set>
#include <math.h>
#include <thread>

#include "map_data.h"
#include "m1.h"
#include <boost/algorithm/string.hpp>
#include <sstream>

constexpr double METERS_TO_KM = 0.001;
constexpr double HOURS_TO_SECONDS = 3600.0;

//initialize all data structures 
Map_data::Map_data(std::string _place_name) {
    
    place_name = _place_name;
    
    number_of_streets = getNumberOfStreets();
    number_of_street_segments = getNumberOfStreetSegments();
    number_of_intersections = getNumberOfIntersections();
    number_of_poi = getNumberOfPointsOfInterest();
    number_of_features = getNumberOfFeatures();
    
    double min_lat;
    double max_lat;
    double min_lon;
    double max_lon;
    
    //check both intersections and poi for map limits
    //ignoring features and street segments
    
    //set min/max lat to first intersection
    min_lat = getIntersectionPosition(0).lat();
    max_lat = getIntersectionPosition(0).lat();
    min_lon = getIntersectionPosition(0).lon();
    max_lon = getIntersectionPosition(0).lon();

    //check all intersections to find min/max lat/lon
    for (unsigned i = 1; i < number_of_intersections; i++) {
        LatLon current_intersection = getIntersectionPosition(i);
        if (current_intersection.lat() < min_lat)
            min_lat = current_intersection.lat();
        else if (current_intersection.lat() > max_lat)
            max_lat = current_intersection.lat();
        if (current_intersection.lon() < min_lon)
            min_lon = current_intersection.lon();
        else if (current_intersection.lon() > max_lon)
            max_lon = current_intersection.lon();
    }
    
    //check all POI to find min/max lat/lon
    for (unsigned i = 1; i < number_of_poi; i++) {
        LatLon current_poi = getPointOfInterestPosition(i);
        if (current_poi.lat() < min_lat)
            min_lat = current_poi.lat();
        else if (current_poi.lat() > max_lat)
            max_lat = current_poi.lat();
        if (current_poi.lon() < min_lon)
            min_lon = current_poi.lon();
        else if (current_poi.lon() > max_lon)
            max_lon = current_poi.lon();
    }
    
    double average_lat = (min_lat + max_lat)/2.0;
    cos_lat = cos(DEG_TO_RAD*average_lat);
    
    min_x  = min_lon * cos_lat;
    max_x = max_lon * cos_lat;
    min_y = min_lat;
    max_y = max_lat;
    
    position_dimension = std::max(1, int(sqrt(number_of_poi) / 2.0));
    
    position_jump_x = (max_x - min_x)/double(position_dimension);
    position_jump_y = (max_y - min_y)/double(position_dimension);
    
    position_x_cutoffs = new double[position_dimension - 1];
    position_y_cutoffs = new double[position_dimension - 1];
    
    //find how rectangular the map is
    stretch_factor = std::max(int(std::ceil(position_jump_x/position_jump_y)), 
            int(std::ceil(position_jump_y/position_jump_x))); 
    
    //fill cutoffs
    for (int i = 0; i < position_dimension - 1; i++) {
        position_x_cutoffs[i] = min_x + (i + 1) * position_jump_x;
        position_y_cutoffs[i] = min_y + (i + 1) * position_jump_y;
    }
    
    /* Position -> Point of interest initialization */
    position_to_poi = new std::vector<poi_basic>* [position_dimension];
    for (int i = 0; i < position_dimension; i++) {
        position_to_poi[i] = new std::vector<poi_basic>[position_dimension];
    }
    
    //initialize threads to load the rest of the data
    std::thread t1(&Map_data::initialize_poi, this);    
    
    /* Position -> Intersection initialization */
    position_to_intersection = new std::vector<unsigned>* [position_dimension];
    for (int i = 0; i < position_dimension; i++) {
        position_to_intersection[i] = new std::vector<unsigned>[position_dimension];
    }
    
    std::thread t2(&Map_data::initialize_intersections, this);
    
    /* Position -> Feature initialization */
    position_to_area_feature = new std::vector<feature_basic>* [position_dimension];
    for (int i = 0; i < position_dimension; i++) {
        position_to_area_feature[i] = new std::vector<feature_basic>[position_dimension];
    }
    
    position_to_line_feature = new std::vector<feature_basic>* [position_dimension];
    for (int i = 0; i < position_dimension; i++) {
        position_to_line_feature[i] = new std::vector<feature_basic>[position_dimension];
    }
    
    std::thread t3(&Map_data::initialize_features, this);
    
    /* OSMWay database initialization */
    for(unsigned i = 0; i < getNumberOfWays(); i++) {
        const OSMWay* way = getWayByIndex(i);
        std::string key,value;
        for(unsigned j = 0; j < getTagCount(way); j++) {
            std::tie(key,value) = getTagPair(way, j);
            if (key == "highway") {
                break;
            }
        }
        osmid_highway_type.insert(std::make_pair(way->id(), value));
    }
    
    /* Position -> Street Segments initialization */
    position_to_highway_street_segments = new std::vector<street_segment_data>* [position_dimension];
    position_to_major_street_segments = new std::vector<street_segment_data>* [position_dimension];
    position_to_minor_street_segments = new std::vector<street_segment_data>* [position_dimension];
    for (int i = 0; i < position_dimension; i++) {
        position_to_highway_street_segments[i] = new std::vector<street_segment_data>[position_dimension];
        position_to_major_street_segments[i] = new std::vector<street_segment_data>[position_dimension];
        position_to_minor_street_segments[i] = new std::vector<street_segment_data>[position_dimension];
    }
    
    for (unsigned i = 0; i < number_of_street_segments; i++) {
        StreetSegmentInfo street_segment_info = getStreetSegmentInfo(i);
        std::string street_type = get_highway_type_from_osmid(street_segment_info.wayOSMID);
        if (street_type == "motorway") {
            street_type = "highway";
            add_to_street_segment_position_vector(position_to_highway_street_segments, i, street_type);
        } else if (street_type == "primary" || street_type == "secondary") {
            street_type = "major";
            add_to_street_segment_position_vector(position_to_major_street_segments, i, street_type);
        } else {
            street_type = "minor";
            add_to_street_segment_position_vector(position_to_minor_street_segments, i, street_type);
        }
    }

    /* Street name to ID unordered multimap initialization */
    for (unsigned i = 0; i < number_of_streets; i++) {
        std::string street_name = getStreetName(i);
        street_name_ID.insert(std::make_pair(street_name,i));
    }
    
    for (unsigned i = 0; i < number_of_street_segments; i++) {
        StreetSegmentInfo street_segment_info = getStreetSegmentInfo(i);
        
        /* Street ID to vector of street segment ID unordered map initialization */
        street_ID_street_segment_IDs[street_segment_info.streetID].push_back(i);
        
        /* Street segment ID to travel time vector initialization */
        //calculate length of segment in km
        double distance = METERS_TO_KM * find_street_segment_length(i);

        //time = distance / speed limit
        double time = HOURS_TO_SECONDS * distance / street_segment_info.speedLimit;
        
        //street segment id -> travel time
        street_segment_ID_travel_time.push_back(time);
    }
    
    /* Street ID to vector of intersections ID unordered map initialization*/
    for (unsigned i = 0; i < number_of_streets; i++) {
        
        //Retrieve all segments of a street
        std::vector<unsigned> temp_street_segments = street_ID_street_segment_IDs[i];
        
        //Temp set to make sure IDs are unique
        std::set<unsigned> temp_intersection_IDs;
        
        //Goes through all segments, retrieve intersection IDs and insert into set
        while (temp_street_segments.size() != 0) {
            StreetSegmentInfo temp_info = getStreetSegmentInfo(temp_street_segments.back());
            temp_street_segments.pop_back();
            temp_intersection_IDs.insert(temp_info.to);
            temp_intersection_IDs.insert(temp_info.from);
        }
        
        street_ID_intersecton_IDs[i].assign(temp_intersection_IDs.begin(), temp_intersection_IDs.end());
    }
    
    /* Intersection name to vector of street segments vector initialization*/
    for (unsigned i = 0; i < number_of_intersections; i++) {
        
        //number of street segments on the intersection
        unsigned street_segment_count = getIntersectionStreetSegmentCount(i);

        //store all street segments in the vector
        std::vector<unsigned> temp_street_segments;
        for (unsigned j = 0; j < street_segment_count; j++) {
            temp_street_segments.push_back(getIntersectionStreetSegment(i,j));
        }
        intersection_ID_street_segment_IDs.push_back(temp_street_segments);
    }
    
    /* Soundex main and keywords library initiation */
    for (unsigned i = 0; i < number_of_intersections; i++) {
        std::string intersection_name = getIntersectionName(i);
        
        if (!valid_intersection_name(intersection_name)) {
            continue;
        }
        std::vector<unsigned> ids;
        ids.push_back(i);
        search_return_data current_object = {
            intersection_name,
            "intersection",
            ids
        };
        std::vector<std::string> keywords;
        keywords = extract_keywords(intersection_name);
        while (keywords.size() != 0) {
            std::string soundex_code = convert_to_soundex(keywords.back());
            soundex_library[soundex_code].push_back(current_object);
            keywords.pop_back();
        }
    }
    std::set<std::string> all_street_names;
    for (unsigned i = 0; i < number_of_streets; i++) {
        std::string street_name = getStreetName(i);
        all_street_names.insert(street_name);
    }
    for (auto i = all_street_names.begin(); i != all_street_names.end(); i++) {
        if (!valid_street_name(*i)) {
            continue;
        }
        std::vector<unsigned> ids = get_street_id_from_name(*i);
        search_return_data current_object = {
            *i,
            "street",
            ids
        };
        std::vector<std::string> keywords;
        keywords = extract_keywords(*i);
        while (keywords.size() != 0) {
            std::string soundex_code = convert_to_soundex(keywords.back());
            soundex_library[soundex_code].push_back(current_object);
            keywords.pop_back();
        }
    }
    for (unsigned i = 0; i < number_of_poi; i++) {
        std::string poi_name = getPointOfInterestName(i);
        std::vector<unsigned> ids;
        ids.push_back(i);
        search_return_data current_object = {
            poi_name,
            "poi",
            ids
        };
        std::vector<std::string> keywords;
        keywords = extract_keywords(poi_name);
        while (keywords.size() != 0) {
            std::string soundex_code = convert_to_soundex(keywords.back());
            soundex_library[soundex_code].push_back(current_object);
            keywords.pop_back();
        }
    }
    
    t1.join();
    t2.join();
    t3.join();
}

void Map_data::initialize_poi() const {
        
    for (unsigned i = 0; i < number_of_poi; i++) {
        poi_basic current_poi;
        current_poi.id = i;
        current_poi.name = getPointOfInterestName(i);
        
        LatLon poi_position = getPointOfInterestPosition(i);
        position_to_poi[get_x_index(poi_position)][get_y_index(poi_position)].push_back(current_poi);
    }
}

void Map_data::initialize_intersections() const {
    for (unsigned i = 0; i < number_of_intersections; i++) {
        LatLon current_intersection = getIntersectionPosition(i);
        position_to_intersection[get_x_index(current_intersection)][get_y_index(current_intersection)].push_back(i);
    }
}

void Map_data::initialize_features() const {
    for (unsigned i = 0; i < number_of_features; i++) {
        feature_basic current_feature;
        current_feature.id = i;
        
        std::vector<coordinate> points = get_feature_points_coordinate(i);
        
        //find the bounding box for the feature
        //want to place the feature in the array for all locations where it might be
        //results in extra drawing for, say, a huge triangular object, but algorithm is simplified
        double feature_min_x = points[0].x;
        double feature_max_x = feature_min_x;
        double feature_min_y = points[0].y;
        double feature_max_y = feature_min_y;
        
        //add each point of the feature and check for feature boundary extremities
        for (auto j = points.begin(); j != points.end(); j++) {
            
            feature_min_x = std::min(feature_min_x, j->x);
            feature_max_x = std::max(feature_max_x, j->x);
            feature_min_y = std::min(feature_min_y, j->y);
            feature_max_y = std::max(feature_max_y, j->y);
        }
        
        //find the x and y indices of the extremities
        int min_x_index = get_x_index(feature_min_x);
        int max_x_index = get_x_index(feature_max_x);
        int min_y_index = get_y_index(feature_min_y);
        int max_y_index = get_y_index(feature_max_y);
        
        //feature with area
        if (points.front().x == points.back().x && points.front().y == points.back().y) {
            current_feature.size = compute_area(points);
            //add the feature to all areas within bounding box
            for (int x_index = min_x_index; x_index <= max_x_index; x_index++) {
                for (int y_index = min_y_index; y_index <= max_y_index; y_index++) {
                    position_to_area_feature[x_index][y_index].push_back(current_feature);
                }
            }
        }
        //line feature
        else {
            current_feature.size = compute_length(points);
            //add the feature to all areas within bounding box
            for (int x_index = min_x_index; x_index <= max_x_index; x_index++) {
                for (int y_index = min_y_index; y_index <= max_y_index; y_index++) {
                    position_to_line_feature[x_index][y_index].push_back(current_feature);
                }
            }
        }
    }
}

//clean up after yourselves!
Map_data::~Map_data() {
    
    //position array variables
    delete [] position_x_cutoffs;
    delete [] position_y_cutoffs; 
    position_x_cutoffs = NULL;
    position_y_cutoffs = NULL;
    
    for (int i = 0; i < position_dimension; i++) {
        delete [] position_to_poi[i];
    }
    delete [] position_to_poi;
    position_to_poi = NULL;
    
    for (int i = 0; i < position_dimension; i++) {
        delete [] position_to_intersection[i];
    }
    delete [] position_to_intersection;
    position_to_intersection = NULL;
    
    for (int i = 0; i < position_dimension; i++) {
        delete [] position_to_area_feature[i];
    }
    delete [] position_to_area_feature;
    position_to_area_feature = NULL;
    
    for (int i = 0; i < position_dimension; i++) {
        delete [] position_to_line_feature[i];
    }
    delete [] position_to_line_feature;
    position_to_line_feature = NULL;
    
    for (int i = 0; i < position_dimension; i++) {
        delete [] position_to_highway_street_segments[i];
    }
    delete [] position_to_highway_street_segments;
    position_to_highway_street_segments = NULL;
    
    for (int i = 0; i < position_dimension; i++) {
        delete [] position_to_major_street_segments[i];
    }
    delete [] position_to_major_street_segments;
    position_to_major_street_segments = NULL;
    
    for (int i = 0; i < position_dimension; i++) {
        delete [] position_to_minor_street_segments[i];
    }
    delete [] position_to_minor_street_segments;
    position_to_minor_street_segments = NULL;
    
    //Maps, multimaps and vectors clear
    
    street_name_ID.clear();
    
    street_ID_street_segment_IDs.clear();
    
    street_ID_intersecton_IDs.clear();
    
    intersection_ID_street_segment_IDs.clear();
    
    street_segment_ID_travel_time.clear();
    
    osmid_highway_type.clear();
    
    soundex_library.clear();
}

std::string Map_data::get_place_name() const {
    return place_name;
}

double Map_data::get_min_x() const {
    return min_x;
}

double Map_data::get_max_x() const {
    return max_x;
}

double Map_data::get_min_y() const {
    return min_y;
}

double Map_data::get_max_y() const {
    return max_y;
}

int Map_data::get_position_dimension() const {
    return position_dimension;
}

//return the id of a street from its name
std::vector<unsigned> Map_data::get_street_id_from_name(std::string name) const {
    std::vector<unsigned> street_IDs;
    
    //Retrieve range of iterators where name is found
    auto range = street_name_ID.equal_range(name);
    for (auto i = range.first; i != range.second; i++) {
        street_IDs.push_back(i->second);
    }
    return street_IDs;
}

//return all IDs of street segments of a particular street given its ID
std::vector<unsigned> Map_data::get_street_segment_ids_from_street_id(unsigned street_id) const {
    std::vector<unsigned> street_segment_IDs;
    
    //Look for iterator pointing to the corresponding street_id
    auto it = street_ID_street_segment_IDs.find(street_id);
    
    //If not found we return empty vector
    if (it == street_ID_street_segment_IDs.end()) {
        return street_segment_IDs;
    }
    else {
        return it->second;
    }
}

//return all IDs of intersections along a street given its ID
std::vector<unsigned> Map_data::get_intersection_ids_from_street_id(unsigned street_id) const {
    std::vector<unsigned> street_intersection_IDs;
    
    auto it = street_ID_intersecton_IDs.find(street_id);
    
    if (it == street_ID_intersecton_IDs.end()) {
        return street_intersection_IDs;
    }
    else {
        return it->second;
    }
}

//return all IDs of connecting street segments of an intersection given its ID
std::vector<unsigned> Map_data::get_street_segment_ids_from_intersection_id(unsigned intersection_id) const {
    
    return intersection_ID_street_segment_IDs[intersection_id];
}

unsigned Map_data::get_closest_point_of_interest(LatLon my_position) const {
    unsigned closest_poi_index = 0;
    double closest_distance;
    
    //find radius to search within
    int my_position_x_index = get_x_index(my_position);
    int my_position_y_index = get_y_index(my_position);
    
    int search_radius;
    bool found_result = false;
    //find the smallest radius that returns at least one result, then add one for safety
        //optional explanation: corner case where my_position is on an edge so 
        //closest point is outside initial radius
    for (search_radius = 0; !found_result; search_radius++) {
        int x_min = std::max(0, my_position_x_index - search_radius);
        int x_max = std::min(position_dimension - 1, my_position_x_index + search_radius);
        int y_min = std::max(0, my_position_y_index - search_radius);
        int y_max = std::min(position_dimension - 1, my_position_y_index + search_radius);
        
        //find at least one result
        for (int x = x_min; !found_result && x <= x_max; x++) {
            for (int y = y_min; !found_result && y <= y_max; y++) {
                if (get_poi_at_position(x, y).size() != 0) {
                    found_result = true;
                    //used for comparisons later
                    closest_distance = find_distance_between_two_points(my_position, getPointOfInterestPosition(get_poi_at_position(x, y)[0].id));
                    closest_poi_index = get_poi_at_position(x, y)[0].id;
                }
            }
        }
    }
    search_radius *= stretch_factor;
    
    //search within that radius for closest POI
    int x_min = std::max(0, my_position_x_index - search_radius);
    int x_max = std::min(position_dimension - 1, my_position_x_index + search_radius);
    int y_min = std::max(0, my_position_y_index - search_radius);
    int y_max = std::min(position_dimension - 1, my_position_y_index + search_radius);
    
    //search within the search radius
    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            std::vector<poi_basic> current_area_poi = get_poi_at_position(x, y);
            for (auto i = current_area_poi.begin(); i != current_area_poi.end(); i++) {
                double current_poi_distance = find_distance_between_two_points(my_position, getPointOfInterestPosition(i->id));
                if (current_poi_distance < closest_distance) {
                    closest_distance = current_poi_distance;
                    closest_poi_index = i->id;
                }
            }
        }
    }
    
    return closest_poi_index;    
}

unsigned Map_data::get_closest_point_of_interest(LatLon my_position, std::string POI_name) const {
    unsigned closest_poi_index = 0;
    double closest_distance;
    
    //find radius to search within
    int my_position_x_index = get_x_index(my_position);
    int my_position_y_index = get_y_index(my_position);
    
    int search_radius;
    bool found_result = false;
    //find the smallest radius that returns at least one result, then add one for safety
        //optional explanation: corner case where my_position is on an edge so 
        //closest point is outside initial radius
    for (search_radius = 0; !found_result; search_radius++) {
        int x_min = std::max(0, my_position_x_index - search_radius);
        int x_max = std::min(position_dimension - 1, my_position_x_index + search_radius);
        int y_min = std::max(0, my_position_y_index - search_radius);
        int y_max = std::min(position_dimension - 1, my_position_y_index + search_radius);
        
        //find at least one result
        for (int x = x_min; !found_result && x <= x_max; x++) {
            for (int y = y_min; !found_result && y <= y_max; y++) {
                std::vector<poi_basic> poi_at_position = get_poi_at_position(x, y);
                for (auto pit = poi_at_position.begin(); pit != poi_at_position.end(); pit++) {
                    if (pit->name == POI_name) {
                        found_result = true;
                        //used for comparisons later
                        closest_distance = find_distance_between_two_points(my_position, getPointOfInterestPosition(pit->id));
                        closest_poi_index = pit->id;
                    }
                }
            }
        }
    }
    search_radius *= stretch_factor;
    
    //search within that radius for closest POI
    int x_min = std::max(0, my_position_x_index - search_radius);
    int x_max = std::min(position_dimension - 1, my_position_x_index + search_radius);
    int y_min = std::max(0, my_position_y_index - search_radius);
    int y_max = std::min(position_dimension - 1, my_position_y_index + search_radius);
    
    //search within the search radius
    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            std::vector<poi_basic> current_area_poi = get_poi_at_position(x, y);
            for (auto i = current_area_poi.begin(); i != current_area_poi.end(); i++) {
                double current_poi_distance = find_distance_between_two_points(my_position, getPointOfInterestPosition(i->id));
                if (current_poi_distance < closest_distance) {
                    closest_distance = current_poi_distance;
                    closest_poi_index = i->id;
                }
            }
        }
    }
    
    return closest_poi_index;    
}

//returns candidates for closest POI by driving time
//assumes that the POI exists
//will return 0 and Anna's tears if it doesn't exist
//don't try to check for 0 to see if the POI exists - POI id 0 is probably real
//and I can't even set it to -1 because it's unsigned
//I coded myself into a corner is what I'm saying
//just don't pass in POI that don't exist orz
std::vector<unsigned> Map_data::get_nearby_point_of_interest(LatLon my_position, std::string POI_name) const {
    std::vector<unsigned> results;
    
    //find radius to search within
    int my_position_x_index = get_x_index(my_position);
    int my_position_y_index = get_y_index(my_position);
    
    int search_radius;
    bool found_result = false;
    int iterations_at_maximum = 0;
    //find the smallest radius that returns at least one result, then add one for safety
        //optional explanation: corner case where my_position is on an edge so 
        //closest point is outside initial radius
    for (search_radius = 0; !found_result; search_radius++) {
        int x_min = std::max(0, my_position_x_index - search_radius);
        int x_max = std::min(position_dimension - 1, my_position_x_index + search_radius);
        int y_min = std::max(0, my_position_y_index - search_radius);
        int y_max = std::min(position_dimension - 1, my_position_y_index + search_radius);
        
        //find at least one result
        for (int x = x_min; !found_result && x <= x_max; x++) {
            for (int y = y_min; !found_result && y <= y_max; y++) {
                std::vector<poi_basic> poi_at_position = get_poi_at_position(x, y);
                for (auto pit = poi_at_position.begin(); pit != poi_at_position.end(); pit++) {
                    if (pit->name == POI_name)
                    found_result = true;
                    //used for comparisons later
                }
            }
        }
        if (x_min == 0 && x_max == position_dimension - 1 && y_min == 0 && y_max == position_dimension - 1) {
            if (iterations_at_maximum > 0) {
                return results; //patchwork coding to prevent a crash
                //returns an empty vector to indicate lack of results
            }
            else {
                iterations_at_maximum++;
            }
        }
    }
    if (found_result) {
        //if you have to really dodge obstacles to get to this POI we found, a factor of 5 is probably enough to find another one.
        //probably
        //in theory
        search_radius *= stretch_factor * 5;

        //search within that radius for POI matching name
        int x_min = std::max(0, my_position_x_index - search_radius);
        int x_max = std::min(position_dimension - 1, my_position_x_index + search_radius);
        int y_min = std::max(0, my_position_y_index - search_radius);
        int y_max = std::min(position_dimension - 1, my_position_y_index + search_radius);

        //search within the search radius
        for (int x = x_min; x <= x_max; x++) {
            for (int y = y_min; y <= y_max; y++) {
                std::vector<poi_basic> current_area_poi = get_poi_at_position(x, y);
                for (auto i = current_area_poi.begin(); i != current_area_poi.end(); i++) {
                    if (i->name == POI_name) {
                        results.push_back(i->id);
                    }
                }
            }
        }
    }    
    return results;    
}

unsigned Map_data::get_closest_intersection(LatLon my_position) const {
    unsigned closest_intersection_index = 0;
    double closest_distance;
    
    //find radius to search within
    int my_position_x_index = get_x_index(my_position);
    int my_position_y_index = get_y_index(my_position);
    
    int search_radius;
    bool found_result = false;
    //find the smallest radius that returns at least one result, then add one for safety
        //optional explanation: corner case where my_position is on an edge so 
        //closest point is outside initial radius
    for (search_radius = 0; !found_result; search_radius++) {
        int x_min = std::max(0, my_position_x_index - search_radius);
        int x_max = std::min(position_dimension - 1, my_position_x_index + search_radius);
        int y_min = std::max(0, my_position_y_index - search_radius);
        int y_max = std::min(position_dimension - 1, my_position_y_index + search_radius);
        
        //find at least one result
        for (int x = x_min; !found_result && x <= x_max; x++) {
            for (int y = y_min; !found_result && y <= y_max; y++) {
                if (get_intersection_at_position(x, y).size() != 0) {
                    found_result = true;
                    //used for comparisons later
                    closest_distance = find_distance_between_two_points(my_position, getIntersectionPosition(get_intersection_at_position(x, y)[0]));
                    closest_intersection_index = get_intersection_at_position(x, y)[0];
                }
            }
        }
    }
    search_radius *= stretch_factor;
    
    //search within that radius for closest intersection
    int x_min = std::max(0, my_position_x_index - search_radius);
    int x_max = std::min(position_dimension - 1, my_position_x_index + search_radius);
    int y_min = std::max(0, my_position_y_index - search_radius);
    int y_max = std::min(position_dimension - 1, my_position_y_index + search_radius);
    
    //search within the search radius
    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            std::vector<unsigned> current_area_intersections = get_intersection_at_position(x, y);
            for (auto i = current_area_intersections.begin(); i != current_area_intersections.end(); i++) {
                double current_intersection_distance = find_distance_between_two_points(my_position, getIntersectionPosition(*i));
                if (current_intersection_distance < closest_distance) {
                    closest_distance = current_intersection_distance;
                    closest_intersection_index = *i;
                }
            }
        }
    }
    
    return closest_intersection_index;    
}

//returns travel time given the street segment id
double Map_data::get_travel_time_from_street_segment_id(unsigned street_segment_id) const {
    
    return street_segment_ID_travel_time[street_segment_id];
}

unsigned Map_data::get_number_of_streets() const {
    return number_of_streets;
}

std::vector<street_segment_data> Map_data::get_highway_street_segment_at_position(int x, int y) const {
    return position_to_highway_street_segments[x][y];
}

std::vector<street_segment_data> Map_data::get_major_street_segment_at_position(int x, int y) const {
    return position_to_major_street_segments[x][y];
}

std::vector<street_segment_data> Map_data::get_minor_street_segment_at_position(int x, int y) const {
    return position_to_minor_street_segments[x][y];
}

std::string Map_data::get_highway_type_from_osmid(OSMID osmid) const {
    return osmid_highway_type.find(osmid)->second;
}

//Main soundex library retrieval given a soundex code
//This returns object with the full name
std::vector<search_return_data> Map_data::get_from_soundex_library(std::string soundex_code) const {
    std::vector<search_return_data> results;
    
    auto it = soundex_library.find(soundex_code);
    if (it == soundex_library.end()) {
        return results;
    }
    else {
        return it->second;
    }
}

std::vector<poi_basic> Map_data::get_poi_at_position(int x, int y) const {
    return position_to_poi[x][y];
}

std::vector<unsigned> Map_data::get_intersection_at_position(int x, int y) const {
    return position_to_intersection[x][y];
}

std::vector<feature_basic> Map_data::get_area_feature_at_position(int x, int y) const {
    return position_to_area_feature[x][y];
}

std::vector<feature_basic> Map_data::get_line_feature_at_position(int x, int y) const {
    return position_to_line_feature[x][y];
}

//get the x index for the position->id array for the given position
int Map_data::get_x_index(LatLon position) const {
    double x_in_degrees = position.lon() * cos_lat;
    int i = 0;
    while (i < position_dimension - 2 && x_in_degrees > position_x_cutoffs[i])
        i++;
    return i;
}

//get the x index for the position->id array for the given position
int Map_data::get_x_index(double x_coordinate) const {
    int i = 0;
    while (i < position_dimension - 2 && x_coordinate > position_x_cutoffs[i])
        i++;
    return i;
}

//get the y index for the position->id array for the given position
int Map_data::get_y_index(LatLon position) const {
    double y_in_degrees = position.lat();
    int i = 0;
    while (i < position_dimension - 2 && y_in_degrees > position_y_cutoffs[i])
        i++;
    return i;
}

//get the y index for the position->id array for the given position
int Map_data::get_y_index(double y_coordinate) const {
    int i = 0;
    while (i < position_dimension - 2 && y_coordinate > position_y_cutoffs[i])
        i++;
    return i;
}

unsigned Map_data::get_number_of_POIs() const {
    return number_of_poi;
}

/* helper functions below */

double Map_data::average(double num1, double num2) const {
    return (num1 + num2)/2.0;
}

//Returns the distance between two coordinates in meters
double Map_data::find_distance_between_two_points(LatLon point1, LatLon point2) const {
    double distance;
    
    //used for x coordinate
    double cos_lat_average = cos(DEG_TO_RAD * average(point1.lat(), point2.lat()));
    
    double x1 = DEG_TO_RAD * point1.lon() * cos_lat_average;
    double x2 = DEG_TO_RAD * point2.lon() * cos_lat_average;
    double y1 = DEG_TO_RAD * point1.lat();
    double y2 = DEG_TO_RAD * point2.lat();
    
    //pythagorean theorem * radius of earth
    distance = EARTH_RADIUS_IN_METERS * sqrt( pow(y2 - y1, 2.0) + pow(x2 - x1, 2.0) );
    return distance;
}

double Map_data::find_distance_between_two_points(coordinate point1, coordinate point2) const {
    
    return sqrt( pow(point2.y - point1.y, 2.0) + pow(point2.x - point1.x, 2.0) );
}

feature_colour Map_data::get_feature_colour_from_type(FeatureType type) const {
    switch (type) {
        case Island :
            return background;
            break;
        case Park : 
        case Greenspace :
        case Golfcourse :
            return green;
            break;
        case Lake : 
        case Beach :
        case River :
        case Shoreline :
        case Stream :
            return blue;
            break;
        default: 
            return gray;
    }
}

std::vector<coordinate> Map_data::get_feature_points_coordinate(unsigned id) const {
    unsigned points_in_feature = getFeaturePointCount(id);
    std::vector<coordinate> points;
    points.resize(points_in_feature);
    
    for (unsigned point_id = 0; point_id < points_in_feature; point_id++) {
        points[point_id] = latLon_to_coordinates(getFeaturePoint(id, point_id));
    }
    return points;
}

std::vector<t_point> Map_data::get_feature_points_t_point(unsigned id) const {
    unsigned points_in_feature = getFeaturePointCount(id);
    std::vector<t_point> points;
    points.resize(points_in_feature);
    
    for (unsigned point_id = 0; point_id < points_in_feature; point_id++) {
        coordinate point = latLon_to_coordinates(getFeaturePoint(id, point_id));
        points[point_id].x = point.x;
        points[point_id].y = point.y;
    }
    return points;
}

coordinate Map_data::latLon_to_coordinates(LatLon point) const {
    coordinate coordinates;
    
    coordinates.x = point.lon() * cos_lat;
    coordinates.y = point.lat();
    
    return coordinates;
}

double Map_data::compute_area(std::vector<coordinate> points) const {
    double area = 0;
    size_t j = points.size() - 1;
    for (size_t i = 0; i < points.size(); i++) {
        area = area + (points[j].x + points[i].x) * (points[j].y - points[i].y);
        j = i;
    }
    return abs(area/2.0);
}

double Map_data::compute_length(std::vector<coordinate> points) const {
    double length = 0;
    size_t last_index = points.size() - 1;
    for (size_t i = 0; i < last_index; i++) {
        length += find_distance_between_two_points(points[i+1], points[i]);
    }
    return abs(length);
}

LatLon Map_data::x_and_y_to_LatLon(float x, float y) const {
    LatLon position(y, x/cos_lat);
    return position;
    
}

//Soundex conversion helpers
std::string Map_data::convert_to_soundex(std::string to_be_converted) const {
    std::string soundex_code;
    boost::to_upper(to_be_converted);
    for (std::string::size_type i = 0; i < to_be_converted.size(); i++) {
        if (i == 0) {
            soundex_code += to_be_converted[i];
        } else if (isalpha(to_be_converted[i])) {
            soundex_code += parse_char(to_be_converted[i]);
        }
    }
    
    for (std::string::size_type i = 0; i < to_be_converted.size(); i++) {
        if (i != 0 && to_be_converted[i] == to_be_converted[i-1]) {
            to_be_converted.erase(i, 1);
        } else if (to_be_converted[i] == '0') {
            to_be_converted.erase(i, 1);
        }
    }
    
    if (to_be_converted.length() < 4) {
        int zeros = 4 - to_be_converted.length();
        for (int i = 0; i < zeros; i++) {
            to_be_converted += '0';
        }
    } else if (to_be_converted.length() > 4) {
        to_be_converted = to_be_converted.substr(0, 4);
    }
    
    return to_be_converted;
}

char Map_data::parse_char(char c) const {
    if (c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U' || c == 'H' || c == 'W' || c == 'Y') {
        return '0';
    } else if (c == 'B' || c == 'F' || c == 'P' || c == 'V') {
        return '1';
    } else if (c == 'C' || c == 'G' || c == 'J' || c == 'K' || c == 'Q' || c == 'S' || c == 'X' || c == 'Z') {
        return '2';
    } else if (c == 'D' || c == 'T') {
        return '3';
    } else if (c == 'M' || c == 'N') {
        return '4';
    } else if (c == 'L') {
        return '5';
    } else {
        return '6';
    }
}

std::vector<std::string> Map_data::extract_keywords(std::string object_name) const {
    std::vector<std::string> keywords;
    std::stringstream input(object_name);
    while (!input.eof()) {
        input >> std::ws;
        std::string temp;
        input >> temp;
        if (temp == "Street" || temp == "street"
                || temp == "Avenue"
                || temp == "Ave"
                || temp == "St"
                || temp == "St."
                || temp == "Str"
                || temp == "and"
                || temp == "&"
                || temp == ""
                || temp == "<unknown>"
                || temp == "road"
                || temp == "Road"
                || temp == "Drive"
                || temp == "Dr"
                || temp == "dr"
                || temp == "Dr."
                || temp == "dr."
                || temp == "to"
                || temp == "To"
                ) {
            continue;
        }
        keywords.push_back(temp);
    }
    return keywords;
}

bool Map_data::valid_street_name(std::string name) {
    if (name.find("<UNKNOWN>") != std::string::npos) {
        return false;
    }
    return true;
}

bool Map_data::valid_intersection_name(std::string name) {
    boost::to_upper(name);
    if (name.find("&") == std::string::npos) {
        return false;
    }
    if (name.find("AND") == std::string::npos) {
        return false;
    }
    if (name.find("<UNKNOWN>") != std::string::npos) {
        return false;
    }
    return true;
}


//This is for m2 drawing street segments
//Given the 2D position to street segment vector, a segment, adds it to the corresponding positions
void Map_data::add_to_street_segment_position_vector(std::vector<street_segment_data>** street_segment_position, unsigned street_segment_id, std::string &street_type) {
    StreetSegmentInfo street_segment_info = getStreetSegmentInfo(street_segment_id);
    std::vector<LatLon> curve_points;

    //Finds out the max and min x,y of a street segment
    //At the same time we also stores all the curve point lat lons
    double street_segment_min_x = getIntersectionPosition(street_segment_info.from).lon()*cos_lat;
    double street_segment_max_x = street_segment_min_x;
    double street_segment_min_y = getIntersectionPosition(street_segment_info.from).lat();
    double street_segment_max_y = street_segment_min_y;

    for (unsigned j = 0; j < street_segment_info.curvePointCount; j++) {
        LatLon curve_point_latlon = getStreetSegmentCurvePoint(street_segment_id, j);
        double curve_point_x = curve_point_latlon.lon()*cos_lat;
        double curve_point_y = curve_point_latlon.lat();

        curve_points.push_back(curve_point_latlon);

        street_segment_min_x = std::min(street_segment_min_x, curve_point_x);
        street_segment_max_x = std::max(street_segment_max_x, curve_point_x);
        street_segment_min_y = std::min(street_segment_min_y, curve_point_y);
        street_segment_max_y = std::max(street_segment_max_y, curve_point_y);
    }

    //Check to and from intersection positions
    double to_intersection_x = getIntersectionPosition(street_segment_info.to).lon()*cos_lat;
    double to_intersection_y = getIntersectionPosition(street_segment_info.to).lat();

    street_segment_min_x = std::min(street_segment_min_x, to_intersection_x);
    street_segment_max_x = std::max(street_segment_max_x, to_intersection_x);
    street_segment_min_y = std::min(street_segment_min_y, to_intersection_y);
    street_segment_max_y = std::max(street_segment_max_y, to_intersection_y);

    //Convert x, y
    int min_x_index = get_x_index(street_segment_min_x);
    int max_x_index = get_x_index(street_segment_max_x);
    int min_y_index = get_y_index(street_segment_min_y);
    int max_y_index = get_y_index(street_segment_max_y);

    //For any position given the max and min x,y we keep the segment
    //This means that when that particular position is visible we can retrieve
    //this segment to draw
    for (int x_index = min_x_index; x_index <= max_x_index; x_index++) {
        for (int y_index = min_y_index; y_index <= max_y_index; y_index++) {
            street_segment_data cur_street_segment;
            if (street_type == "highway") {
                cur_street_segment = {
                    street_segment_id,
                    street_type,
                    street_segment_info,
                    curve_points,
                    5,
                    false,
                    false
                };
            } else if (street_type == "major") {
                cur_street_segment = {
                    street_segment_id,
                    street_type,
                    street_segment_info,
                    curve_points,
                    3,
                    false,
                    false
                };
            } else {
                cur_street_segment = {
                    street_segment_id,
                    street_type,
                    street_segment_info,
                    curve_points,
                    2,
                    false,
                    false
                };
            }
            street_segment_position[x_index][y_index].push_back(cur_street_segment);
        }
    }
}
