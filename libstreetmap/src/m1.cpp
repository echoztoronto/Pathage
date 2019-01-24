/* 
 * Copyright 2018 University of Toronto
 *
 * Permission is hereby granted, to use this software and associated 
 * documentation files (the "Software") in course work at the University 
 * of Toronto, or for personal use. Other uses are prohibited, in 
 * particular the distribution of the Software either publicly or to third 
 * parties.
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <set>
#include <math.h>
#include <algorithm>

#include "m1.h"
#include "map_data.h"
#include "StreetsDatabaseAPI.h"

Map_data const* map_data;

bool load_map_osm(std::string map_osm_name);
double average(double num1, double num2);

bool load_map_osm(std::string map_osm_name) {
    return loadOSMDatabaseBIN(map_osm_name); //Indicates whether the map has loaded successfully
}

//Loads a map streets.bin file. Returns true if successful, false if some error
//occurs and the map can’t be loaded.
bool load_map(std::string map_name) {
    
    //get the path of the OSM database through string manipulation    
    unsigned first = map_name.find("/maps/"); //length 6
    unsigned last = map_name.find(".streets.bin"); //length 
    std::string location_name = map_name.substr(first + 6, last - first - 6);
    
    std::string map_osm_path = "/cad2/ece297s/public/maps/" + location_name + ".osm.bin";  
    
    bool load_successful = load_map_osm(map_osm_path);
    
    if (!load_successful) {
        std::cerr << "Failed to load osm database '" << map_osm_path << "'\n";
        return false;
    }
    
    load_successful = loadStreetsDatabaseBIN(map_name); //Indicates whether the map has loaded successfully
    
    if(load_successful) {
        
        //replace characters to make the name look presentable
        std::replace(location_name.begin(), location_name.end(), '-', ' ');
        std::replace(location_name.begin(), location_name.end(), '_', ',');
        
        //add a space after any comma
        std::string::size_type comma_location = location_name.find(",");
        if (comma_location != std::string::npos) {
            location_name.insert(comma_location + 1, " ");
        }
        
        //capitaize first letter of each word
        //from thispointer.com/convert-first-letter-of-each-word-of-a-string-to-upper-case-in-c/
        char previous = ' ';
        for (auto c = location_name.begin(); c != location_name.end(); c++) {
            if (previous == ' ' && *c != ' ' && std::isalpha(*c)) {
                *c = std::toupper(*c);
            }
            previous = *c;
        }
        
        //load data here
        map_data = new Map_data(location_name);
    }

    return load_successful;
}

//Close the map (if loaded)
void close_map() {
    closeStreetDatabase();
    closeOSMDatabase();
    //Clean-up your map related data structures here
    delete map_data;
}

//Returns street id(s) for the given street name
//If no street with this name exists, returns a 0-length vector.
std::vector<unsigned> find_street_ids_from_name(std::string street_name) {
   
    return map_data->get_street_id_from_name(street_name);
}

//Returns the street segments for the given intersection 
std::vector<unsigned> find_intersection_street_segments(unsigned intersection_id) {
    
    return map_data->get_street_segment_ids_from_intersection_id(intersection_id);
}

//Returns the street names at the given intersection (includes duplicate street 
//names in returned vector)
std::vector<std::string> find_intersection_street_names(unsigned intersection_id) {
    std::vector<std::string> street_names;

    //all street segments on the given intersection.
    std::vector<unsigned> street_segments = map_data->get_street_segment_ids_from_intersection_id(intersection_id);
  
    //find street ID within each street segment Info; then find street name for each street ID.
    for (unsigned i = 0; i < getIntersectionStreetSegmentCount(intersection_id); i++) {
        street_names.push_back(getStreetName(getStreetSegmentInfo(street_segments[i]).streetID));
    }

    return street_names;
}

//Returns true if you can get from intersection1 to intersection2 using a single 
//street segment (hint: check for 1-way streets too)
//corner case: an intersection is considered to be connected to itself
bool are_directly_connected(unsigned intersection_id1, unsigned intersection_id2) {
    //Corner case as mentioned above
    if (intersection_id1 == intersection_id2)
        return true;    
    
    //Get number of segments for one intersections
    unsigned id1_street_segment_count = getIntersectionStreetSegmentCount(intersection_id1);
    
    //Retrieve each segment's info and check if one way
    //If one way we check if it goes to the other
    //Else as long as one of them is the other intersection , the two are connected
    for (unsigned i = 0; i < id1_street_segment_count; i++) {
        unsigned street_segment_ID = getIntersectionStreetSegment(intersection_id1, i);
        StreetSegmentInfo street_segment_info = getStreetSegmentInfo(street_segment_ID);
        if (street_segment_info.oneWay) {
            if (street_segment_info.to == intersection_id2)
                return true;
        }
        else {
            if (street_segment_info.to == intersection_id2 || street_segment_info.from == intersection_id2)
                return true;
        }
    }
    return false;
}

//Returns all intersections reachable by traveling down one street segment 
//from given intersection (hint: you can't travel the wrong way on a 1-way street)
//the returned vector should NOT contain duplicate intersections
std::vector<unsigned> find_adjacent_intersections(unsigned intersection_id) {
    
    //Get total number of street segments of the given intersection
    unsigned street_segment_count = getIntersectionStreetSegmentCount(intersection_id);
    
    std::set<unsigned> adjacent_intersection_temp;
    
    for (unsigned i = 0; i < street_segment_count; i++) {
        
        //Retrieve the street segments IDs
        unsigned street_segment_ID = getIntersectionStreetSegment(intersection_id, i);
        
        //Retrieve info for each segment
        StreetSegmentInfo street_segment_info = getStreetSegmentInfo(street_segment_ID);
        
        //If it is one way, we make sure it goes from the given intersection to the other
        if (street_segment_info.oneWay) {
            if (street_segment_info.from == intersection_id)
                adjacent_intersection_temp.insert(street_segment_info.to);
        } else {
            //Makes sure not to add the given intersection ID, but instead add the adjacent one
            if (street_segment_info.from == intersection_id)
                adjacent_intersection_temp.insert(street_segment_info.to);
            else
                adjacent_intersection_temp.insert(street_segment_info.from);
        }
    }
    
    //Vector to hold all the IDs of adjacent intersections
    std::vector<unsigned> adjacent_intersections(adjacent_intersection_temp.begin(), adjacent_intersection_temp.end());
    
    return adjacent_intersections;
}

//Returns all street segments for the given street
std::vector<unsigned> find_street_street_segments(unsigned street_id) {
    
    return map_data->get_street_segment_ids_from_street_id(street_id);
}

//Returns all intersections along the a given street
std::vector<unsigned> find_all_street_intersections(unsigned street_id) {
    
    return map_data->get_intersection_ids_from_street_id(street_id);
}

//Return all intersection ids for two intersecting streets
//This function will typically return one intersection id.
//However street names are not guarenteed to be unique, so more than 1 intersection id
//may exist
std::vector<unsigned> find_intersection_ids_from_street_names(std::string street_name1, std::string street_name2) {
    
    //common intersections of the given streets
    std::vector<unsigned>common_intersections;
    
    //get street IDs from street names
    std::vector<unsigned>street_IDs = map_data->get_street_id_from_name(street_name1);
    std::vector<unsigned>temp = map_data->get_street_id_from_name(street_name2);
    street_IDs.insert(street_IDs.end(), temp.begin(), temp.end());
    
    //stores all intersection IDs
    std::vector<unsigned>intersection_IDs;
    for(unsigned i = 0; i < street_IDs.size(); i++){
        std::vector<unsigned>temp2 = map_data->get_intersection_ids_from_street_id(street_IDs[i]);
        intersection_IDs.insert(intersection_IDs.end(),temp2.begin(), temp2.end());
    }
    
    if (intersection_IDs.size() == 0) {
        return common_intersections;
    }
    
    //sort all intersection IDs in order
    std::sort(intersection_IDs.begin(),intersection_IDs.end());
    
    //store common intersections 
    for(unsigned i = 0; i < intersection_IDs.size()-1; i++) {
        if(intersection_IDs[i]==intersection_IDs[i+1]) {
            common_intersections.push_back(intersection_IDs[i]);
        }
    }
    
    return common_intersections;
}

//Returns the distance between two coordinates in meters
double find_distance_between_two_points(LatLon point1, LatLon point2) {
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

//Returns the length of the given street segment in meters
double find_street_segment_length(unsigned street_segment_id) {
    //fetch info about the segment
    StreetSegmentInfo segment_info = getStreetSegmentInfo(street_segment_id);
    
    //make a vector to hold all corners of the street
    std::vector<LatLon> vertices;
    
    //street start
    vertices.push_back(getIntersectionPosition(segment_info.from));
    //fill in any interior corners 
    for (unsigned i = 0; i < segment_info.curvePointCount; i++) {
        vertices.push_back(getStreetSegmentCurvePoint(street_segment_id, i));
    }
    //street end
    vertices.push_back(getIntersectionPosition(segment_info.to));
    
    //accumulate distance for each straight part
    double distance = 0.0;
    
    for (std::vector<LatLon>::iterator i = vertices.begin(); i != vertices.end() - 1; i++) {
        distance += find_distance_between_two_points(*i, *(i+1));
    }
    
    return distance;
}

//Returns the length of the specified street in meters
double find_street_length(unsigned street_id) {
    double street_length = 0;
    
    //get all street segments part of the street
    std::vector<unsigned> street_segments = find_street_street_segments(street_id);
    
    //accumulate the distance of each segment
    for (std::vector<unsigned>::size_type i = 0; i < street_segments.size(); i++) {
        street_length += find_street_segment_length(street_segments[i]);
    }
    
    return street_length;
}

//Returns the travel time to drive a street segment in seconds 
//(time = distance/speed_limit)
double find_street_segment_travel_time(unsigned street_segment_id) {
    
    return map_data->get_travel_time_from_street_segment_id(street_segment_id);
}

//Returns the nearest point of interest to the given position
unsigned find_closest_point_of_interest(LatLon my_position) {
    
    return map_data->get_closest_point_of_interest(my_position); 
}

//Returns the the nearest intersection to the given position
unsigned find_closest_intersection(LatLon my_position) {
    
    return map_data->get_closest_intersection(my_position);
}

/* helper functions below */

double average(double num1, double num2) {
    
    return (num1 + num2)/2.0;
}
