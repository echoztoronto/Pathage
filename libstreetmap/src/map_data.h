/*
 * Header file for the Map_data class that holds data loaded from map bin file
 * Property of CD-026, do not redistribute
 */

#ifndef MAP_DATA_H
#define MAP_DATA_H

#include <set>
#include <math.h>
#include <string>
#include <unordered_map>

#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include "graphics_types.h"


typedef struct coordinate {
    double x;
    double y;
} coordinate;

//cannot definitely say that two features won't have the same size, so keep id
//for sorting within sets
typedef struct feature_basic {
    unsigned id;
    double size; //area or length
    bool operator<(const feature_basic &other) const {return id < other.id;}
} feature_basic;

enum feature_colour {
    green,
    blue,
    gray,
    background
};

typedef struct feature {
    unsigned id;
    feature_colour colour;
    double size;
    int npoints;
    std::vector<t_point> points;
    bool operator<(const feature &other) const {return id < other.id;}
} feature;

typedef struct poi_basic {
    unsigned id;
    std::string name;
    bool operator<(const poi_basic &other) const {return id < other.id;}
} poi_basic;

typedef struct poi {
    unsigned id;
    std::string name;
    std::string type;
    coordinate text_position;
    coordinate icon_position;
    bool operator<(const poi &other) const {return id < other.id;}
} poi;

typedef struct street_segment_data {
    unsigned id;
    std::string street_type;
    StreetSegmentInfo info;
    std::vector<LatLon> curve_points;
    double width;
    bool draw_name;
    bool selected;
    street_segment_data() = default;
    street_segment_data(unsigned new_id, std::string new_street_type, StreetSegmentInfo new_info, std::vector<LatLon> new_curve_points, double new_width, bool new_draw_name, bool new_selected) : 
    id(new_id), street_type(new_street_type), info(new_info), curve_points(new_curve_points), width(new_width), draw_name(new_draw_name), selected(new_selected) {}
    street_segment_data(const unsigned &new_id) : id(new_id) {}
    bool operator<(const street_segment_data &other) const {return id < other.id;}
    street_segment_data operator=(const street_segment_data &other) {
        id = other.id;
        street_type = other.street_type;
        info = other.info;
        curve_points = other.curve_points;
        width = other.width;
        draw_name = other.draw_name;
        selected = other.selected;
        return *this;
    }
    bool operator==(const street_segment_data &other) const {return id == other.id;}
} street_segment_data;

typedef struct search_return_data {
    std::string object_name;
    std::string object_type;
    std::vector<unsigned> object_id;
    bool operator==(const search_return_data &other) const {return (object_name.compare(other.object_name) == 0);}
    bool operator<(const search_return_data &other) const {return (object_name.compare(other.object_name) < 0);}
    bool operator>(const search_return_data &other) const {return (object_name.compare(other.object_name) > 0);}
} search_return_data;

class Map_data {
private:
    
    std::string place_name;
    
    //street name -> street id
    std::unordered_multimap<std::string, unsigned> street_name_ID;
    
    //street id -> street segment ids
    std::unordered_map<unsigned, std::vector<unsigned>> street_ID_street_segment_IDs;
    
    //street id -> intersection ids
    std::unordered_map<unsigned, std::vector<unsigned>> street_ID_intersecton_IDs;
    
    //intersection id -> street segment ids
    std::vector<std::vector<unsigned>> intersection_ID_street_segment_IDs;
    
    //street segment id -> speed limit
    std::vector<double> street_segment_ID_travel_time;    
    
    // Positional variables
    double min_x;
    double min_y;
    double max_x;
    double max_y;
    double cos_lat; //cos(map_average_lat)
    
    int position_dimension; //size of the n by n array
    double position_jump_x; //x increment of the n by n array
    double position_jump_y; // y increment of the n by n array
    int stretch_factor; //how rectangular (non-square) the map is
    
    //these hold the maximum coordinate for each cell in the position array
    //e.g. if position_x_cutoffs[0] = 2, position[x = 0] = all points x < 2
    double* position_x_cutoffs;
    double* position_y_cutoffs;
    
    std::vector<poi_basic>** position_to_poi;
    std::vector<unsigned>** position_to_intersection;
    std::vector<feature_basic>** position_to_area_feature;
    std::vector<feature_basic>** position_to_line_feature;
    
    std::vector<street_segment_data>** position_to_highway_street_segments;
    std::vector<street_segment_data>** position_to_major_street_segments;
    std::vector<street_segment_data>** position_to_minor_street_segments;

    unsigned number_of_streets;
    unsigned number_of_street_segments;
    unsigned number_of_intersections;
    unsigned number_of_poi;
    unsigned number_of_features;
    
    std::unordered_map<OSMID, std::string> osmid_highway_type;
    std::unordered_map<std::string, std::vector<search_return_data>> soundex_library;
    
public:
    Map_data(std::string _place_name);
    ~Map_data();
    
    void initialize_poi() const;
    void initialize_intersections() const;
    void initialize_features() const;
    
    //Methods for accessing private variables
    std::string get_place_name() const;
    double get_min_x() const;
    double get_max_x() const;
    double get_min_y() const;
    double get_max_y() const;
    int get_position_dimension() const;
    std::vector<unsigned> get_street_id_from_name(std::string name) const;
    std::vector<unsigned> get_street_segment_ids_from_street_id(unsigned street_id) const;
    std::vector<unsigned> get_intersection_ids_from_street_id(unsigned street_id) const;
    std::vector<unsigned> get_street_segment_ids_from_intersection_id(unsigned intersection_id) const;
    unsigned get_closest_point_of_interest(LatLon my_position) const;
    unsigned get_closest_point_of_interest(LatLon my_position, std::string POI_name) const;
    std::vector<unsigned> get_nearby_point_of_interest(LatLon my_position, std::string POI_name) const;
    unsigned get_closest_intersection(LatLon my_position) const;
    double get_travel_time_from_street_segment_id(unsigned street_segment_id) const;
 
    unsigned get_number_of_streets() const;
    std::vector<street_segment_data> get_highway_street_segment_at_position(int x, int y) const;
    std::vector<street_segment_data> get_major_street_segment_at_position(int x, int y) const;
    std::vector<street_segment_data> get_minor_street_segment_at_position(int x, int y) const;
    std::string get_highway_type_from_osmid(OSMID osmid) const;
    std::vector<search_return_data> get_from_soundex_library(std::string soundex_code) const;
    
    std::vector<poi_basic> get_poi_at_position(int x, int y) const;
    std::vector<unsigned> get_intersection_at_position(int x, int y) const;
    std::vector<feature_basic> get_area_feature_at_position(int x, int y) const;
    std::vector<feature_basic> get_line_feature_at_position(int x, int y) const;
    
    //get the indices for the position arrays for the given position
    int get_x_index(LatLon position) const;
    int get_y_index(LatLon position) const;
    int get_x_index(double x_coordinate) const;
    int get_y_index(double y_coordinate) const;
    
    unsigned get_number_of_POIs() const;
    
    //helper functions
    double average(double num1, double num2) const;
    double find_distance_between_two_points(LatLon point1, LatLon point2) const;
    double find_distance_between_two_points(coordinate point1, coordinate point2) const;
    
    feature_colour get_feature_colour_from_type(FeatureType type) const;
    std::vector<coordinate> get_feature_points_coordinate(unsigned id) const;
    std::vector<t_point> get_feature_points_t_point(unsigned id) const;
    
    coordinate latLon_to_coordinates(LatLon point) const;
    double compute_area(std::vector<coordinate> points) const;
    double compute_length(std::vector<coordinate> points) const;
    LatLon x_and_y_to_LatLon(float x, float y) const;
    
    std::string convert_to_soundex(std::string to_be_converted) const;
    char parse_char(char c) const;
    std::vector<std::string> extract_keywords(std::string object_name) const;
    
    bool valid_street_name(std::string name);
    bool valid_intersection_name(std::string name);
    
    void add_to_street_segment_position_vector(std::vector<street_segment_data>** street_segment_position, unsigned street_segment_id, std::string &street_type);
};

#endif /* MAP_DATA_H */

