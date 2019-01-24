/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <assert.h>

#include "draw_data.h"

#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"

extern Map_data const *map_data;

Draw_data::Draw_data() {
    
    zoom_level = 1.0;
        
    number_of_nodes = getNumberOfNodes();
    number_of_ways = getNumberOfWays();
    number_of_relations = getNumberOfRelations();
    
    t_bound_box impossible_boundary(0, 0, 0, 0); 
    
    current_bounds = impossible_boundary;
    current_view_center.x = 0.0;
    current_view_center.y = 0.0;

    previous_poi_x = 0;
    previous_poi_y = 0;
    
    //force update
    coordinate impossible_coordinate;
    impossible_coordinate.x = 1000.0;
    impossible_coordinate.y = 1000.0;
    
    area_features.last_acknowledged_center = impossible_coordinate;
    line_features.last_acknowledged_center = impossible_coordinate;
    on_screen_pois.last_acknowledged_center = impossible_coordinate;
    highway_segments.last_acknowledged_center = impossible_coordinate;
    major_segments.last_acknowledged_center = impossible_coordinate;
    minor_segments.last_acknowledged_center = impossible_coordinate;
    
    area_features.last_acknowledged_bounds = impossible_boundary;
    line_features.last_acknowledged_bounds = impossible_boundary;
    on_screen_pois.last_acknowledged_bounds = impossible_boundary;
    highway_segments.last_acknowledged_bounds = impossible_boundary;
    major_segments.last_acknowledged_bounds = impossible_boundary;
    minor_segments.last_acknowledged_bounds = impossible_boundary;
    
    high_res_position_dimension = map_data->get_position_dimension() * 1;
    
    position_to_street_names = new bool* [high_res_position_dimension];
    for (int i = 0; i < high_res_position_dimension; i++) {
        position_to_street_names[i] = new bool[high_res_position_dimension];
    }
    clear_position_to_street_name();
}

Draw_data::~Draw_data() {
    
    selected_street_ids.clear();
    found_path.clear();
    from_x = FLT_MAX;
    from_y = FLT_MAX;
    to_x = FLT_MAX;
    to_y = FLT_MAX;
    old_from_x = FLT_MAX;
    old_from_y = FLT_MAX;
    old_to_x = FLT_MAX;
    old_to_y = FLT_MAX;
    show_found_path = false;
    
    for (int i = 0; i < high_res_position_dimension; i++) {
        delete [] position_to_street_names[i];
    }
    delete [] position_to_street_names;
    position_to_street_names = NULL;
    
}

/****************************************************************************************
                                  Accessors
 ****************************************************************************************/

bool Draw_data::get_map_active() const {
    return map_active;
}

float Draw_data::get_zoom_level() const {
    return zoom_level;
}

bool Draw_data::get_draw_poi() const {
    return draw_poi;
}

bool Draw_data::get_poi_basic() const {
    return poi_basic;
}
 
bool Draw_data::get_poi_extra() const {
    return poi_extra;
}

double Draw_data::get_previous_poi_x() const {
    return previous_poi_x;
}

double Draw_data::get_previous_poi_y() const {
    return previous_poi_y;
}

coordinate Draw_data::get_current_view_center() const {
    return current_view_center;
}

t_bound_box Draw_data::get_current_bounds() const {
    return current_bounds;
}

t_bound_box Draw_data::get_current_screen_bounds() const {
    return current_screen_bounds;
}

feature_dynamic_data* Draw_data::get_feature_data(int id) {
    assert (id > 0 && id < 3); //number of feature variables
    switch (id) {
        case 1 : return &(area_features);
        break;
        default : return &(line_features);
        break;
    }
}

poi_dynamic_data* Draw_data::get_poi_data() {
    return &(on_screen_pois);
}

street_segment_dynamic_data* Draw_data::get_street_segment_data(int id) {
    assert (id > 4 && id < 8); //number of feature variables
    switch (id) {
        case 5 : return &(highway_segments);
        break;
        case 6 : return &(major_segments);
        break;
        default : return &(minor_segments);
        break;
    }
}

bool Draw_data::get_highway_segment_reload() {
    return highway_segments.force_reload;
}

bool Draw_data::get_major_segment_reload() {
    return major_segments.force_reload;
}

bool Draw_data::get_minor_segment_reload() {
    return minor_segments.force_reload;
}

/****************************************************************************************
                                  Interactivity Functions
 ****************************************************************************************/

UI_element* Draw_data::add_ui_element(UI_element* new_item) {
    ui_elements.push_back(new_item);
    return ui_elements.back();
}

void Draw_data::draw_all_ui_elements() {
    for (auto i = ui_elements.begin(); i != ui_elements.end(); i++) {
        (*i)->draw();
    }
}

//returns NULL if none
UI_element* Draw_data::find_clicked_on_element(float x, float y) {
    bool found_clicked_on = false;
    
    //iterate over the vector backwards -> priority given to last-added items
    unsigned i = ui_elements.size() - 1;
    
    while (!found_clicked_on && i != 0) {
        if ((ui_elements[i])->clicked_on(x, y)) {
            found_clicked_on = true;
            return ui_elements[i];
        }
        else {
            i--;
        }
    }
    
    //check the last item as well
    if (ui_elements.size() != 0) {
        if ((ui_elements[0])->clicked_on(x, y)) {
            found_clicked_on = true;
            return ui_elements[0];
        }
    }

    return NULL;

}

bool Draw_data::get_intersection_selected() const {
    return intersection_selected;
}

unsigned Draw_data::get_selected_intersection_id() const {
    assert (intersection_selected);
    return selected_intersection_id;
}

bool Draw_data::get_poi_selected() const {
    return poi_selected;
}

unsigned Draw_data::get_selected_poi_id() const {
    assert (poi_selected);
    return selected_poi_id;
}

/****************************************************************************************
                                  Mutators
 ****************************************************************************************/

void Draw_data::disable_map() {
    map_active = false;
}

void Draw_data::set_zoom_level(float new_zoom_level) {
    zoom_level = new_zoom_level;
}

void Draw_data::set_draw_poi(bool on) {
    draw_poi = on;
}

void Draw_data::set_poi_basic(bool on) {
    poi_basic = on;
}

void Draw_data::set_poi_extra(bool on) {
    poi_extra = on;
}

void Draw_data::set_current_view_center(float x, float y) {
    current_view_center.x = x;
    current_view_center.y = y;
}

void Draw_data::set_current_view_center(coordinate new_center) {
    current_view_center = new_center;
}

void Draw_data::set_current_bounds(t_bound_box new_bounds) {
    current_bounds = new_bounds;
}

void Draw_data::set_current_screen_bounds(t_bound_box new_bounds) {
    current_screen_bounds = new_bounds;
}

void Draw_data::set_selected_intersection(unsigned id) {
    intersection_selected = true;
    selected_intersection_id = id;
}

void Draw_data::set_previous_poi_x(double x)  {
    previous_poi_x = x;
}

void Draw_data::set_previous_poi_y(double y)  {
    previous_poi_y = y;
}

void Draw_data::unselect_intersection() {
    intersection_selected = false;
}

void Draw_data::set_selected_poi(unsigned id) {
    poi_selected = true;
    selected_poi_id = id;
}

void Draw_data::unselect_poi() {
    poi_selected = false;
}

void Draw_data::force_reload_segments() {
    highway_segments.force_reload = true;
    major_segments.force_reload = true;
    minor_segments.force_reload = true;
}

void Draw_data::disable_highway_segment_reload() {
    highway_segments.force_reload = false;
}

void Draw_data::disable_major_segment_reload() {
    major_segments.force_reload = false;
}

void Draw_data::disable_minor_segment_reload() {
    minor_segments.force_reload = false;
}

void Draw_data::clear_position_to_street_name() const {
    for (int i = 0; i < high_res_position_dimension; i++) {
        for (int j = 0; j < high_res_position_dimension; j++) {
            position_to_street_names[i][j] = false;
        }
    }
}

bool Draw_data::get_position_to_street_name(int x, int y) const {
    return position_to_street_names[x][y];
}

void Draw_data::update_position_to_street_name(int x, int y, int x_range, int y_range) {
    
    int x_lower_bound = std::max(0, x - x_range);
    int x_upper_bound = std::min(high_res_position_dimension, x + x_range);
    int y_lower_bound = std::max(0, y - y_range);
    int y_upper_bound = std::min(high_res_position_dimension, y + y_range);
    
    for (int i = x_lower_bound; i < x_upper_bound; i++) {
        for (int j = y_lower_bound; j < y_upper_bound; j++) {
            position_to_street_names[i][j] = true;
        }
    }
}