/*
 * This class contains dynamic data used to draw the map. 
 */

#ifndef DRAW_DATA_H
#define DRAW_DATA_H

#include <set>
#include <math.h>
#include <string>
#include <unordered_map>

#include "StreetsDatabaseAPI.h"
#include "map_data.h"
#include "ui_element.h"

typedef struct feature_dynamic_data {
    //default to impossible values to force loading
    coordinate last_acknowledged_center;
    t_bound_box last_acknowledged_bounds;
    std::vector<feature> primary_copy;
    std::vector<feature> secondary_copy;
    
    /*
    * holds the id of the thread editing the set to prevent collisions
    * -1 -> unoccupied
    * 0 -> main thread
    * # -> thread number
    */
    volatile int primary_copy_editing_id = -1;
    volatile int secondary_copy_editing_id = -1;
    bool secondary_copy_up_to_date = true;
    
} feature_dynamic_data;

typedef struct poi_dynamic_data {
    //default to impossible values to force loading
    coordinate last_acknowledged_center;
    t_bound_box last_acknowledged_bounds;
    std::set<poi> primary_copy;
    std::set<poi> secondary_copy;
    
    /*
    * holds the id of the thread editing the set to prevent collisions
    * -1 -> unoccupied
    * 0 -> main thread
    * # -> thread number
    */
    volatile int primary_copy_editing_id = -1;
    volatile int secondary_copy_editing_id = -1;
    bool secondary_copy_up_to_date = true;
    
} poi_dynamic_data;

typedef struct street_segment_dynamic_data {
    //default to impossible values to force loading
    coordinate last_acknowledged_center;
    t_bound_box last_acknowledged_bounds;
    bool force_reload = false;
    std::set<street_segment_data> primary_copy;
    std::set<street_segment_data> secondary_copy;
    
    /*
    * holds the id of the thread editing the set to prevent collisions
    * -1 -> unoccupied
    * 0 -> main thread
    * # -> thread number
    */
    volatile int primary_copy_editing_id = -1;
    volatile int secondary_copy_editing_id = -1;
    bool secondary_copy_up_to_date = true;
    
} street_segment_dynamic_data;

class Draw_data {
private:
    
    bool** position_to_street_names;
    
    int high_res_position_dimension;
    
    volatile bool map_active = true;
    
    unsigned number_of_nodes;
    unsigned number_of_ways;
    unsigned number_of_relations;
    
    double previous_poi_x;
    double previous_poi_y;
    
    coordinate current_view_center;
    t_bound_box current_bounds;
    t_bound_box current_screen_bounds;
    float zoom_level;
    
    //toggling variables
    bool draw_poi = false;
    bool poi_basic = true;
    bool poi_extra = false;
    
    //on-screen (drawn) map elements    
    feature_dynamic_data area_features;
    feature_dynamic_data line_features;
    poi_dynamic_data on_screen_pois;
    street_segment_dynamic_data highway_segments;
    street_segment_dynamic_data major_segments;
    street_segment_dynamic_data minor_segments;
       
    //interactivity things
    std::vector<UI_element*> ui_elements;
    bool intersection_selected = false;
    unsigned selected_intersection_id;
    bool poi_selected = false;
    unsigned selected_poi_id;
    
public:    
    SearchBox search_box;
    Button clear_search;
    AutocompleteBox autocomplete_box;
    std::unordered_map<unsigned, bool> selected_street_ids;
    
    std::vector<unsigned> found_path;
    float right_click_x;
    float right_click_y;
    float from_x = FLT_MAX;
    float from_y = FLT_MAX;
    float to_x = FLT_MAX;
    float to_y = FLT_MAX;
    float old_from_x = FLT_MAX;
    float old_from_y = FLT_MAX;
    float old_to_x = FLT_MAX;
    float old_to_y = FLT_MAX;
    float find_poi_x = FLT_MAX;
    float find_poi_y = FLT_MAX;
    bool show_found_path = false;
    bool find_poi = false;
    
    Draw_data();
    ~Draw_data();
    
    //accessors
    bool get_map_active() const;
    float get_zoom_level() const;
    bool get_draw_poi() const;
    bool get_poi_basic() const;
    bool get_poi_extra() const;
    double get_previous_poi_x() const;
    double get_previous_poi_y() const;
    coordinate get_current_view_center() const;
    t_bound_box get_current_bounds() const;
    t_bound_box get_current_screen_bounds() const;
    
    feature_dynamic_data* get_feature_data(int id);
    poi_dynamic_data* get_poi_data();
    street_segment_dynamic_data* get_street_segment_data(int id);
    bool get_highway_segment_reload();
    bool get_major_segment_reload();
    bool get_minor_segment_reload();
    
    //interactivity functions
    UI_element* add_ui_element(UI_element* new_item);
    void draw_all_ui_elements();
    UI_element* find_clicked_on_element(float x, float y); //returns NULL if none
    bool get_intersection_selected() const;
    unsigned get_selected_intersection_id() const;
    bool get_poi_selected() const;
    unsigned get_selected_poi_id() const;
    
    //mutators
    void disable_map();
    void set_zoom_level(float new_zoom_level);
    void set_draw_poi(bool on);
    void set_poi_basic(bool on);
    void set_poi_extra(bool on);
    void set_current_view_center(float x, float y);
    void set_current_view_center(coordinate new_center);
    void set_current_bounds(t_bound_box new_bounds);
    void set_current_screen_bounds(t_bound_box new_bounds);
    void set_selected_intersection(unsigned id);
    void set_previous_poi_x(double x);
    void set_previous_poi_y(double y);
    void unselect_intersection();
    void set_selected_poi(unsigned id);
    void unselect_poi();
    void force_reload_segments();
    void disable_highway_segment_reload();
    void disable_major_segment_reload();
    void disable_minor_segment_reload();
    
    void clear_position_to_street_name() const;
    bool get_position_to_street_name(int x, int y) const;
    void update_position_to_street_name(int x, int y, int x_range, int y_range);
};

#endif /* DRAW_DATA_H */

