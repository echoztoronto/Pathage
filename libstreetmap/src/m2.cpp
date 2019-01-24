/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <thread>
#include <stdlib.h>
#include "m1.h"
#include "m2.h"
#include "m3.h"
#include "map_data.h"
#include "graphics.h"
#include "LatLon.h"
#include "StreetsDatabaseAPI.h"
#include "draw_data.h"
#include "graphics_types.h"
#include "ui_element.h"
#include "graphics_state.h"
#include "ui_interface.h"
#include <X11/keysym.h>
#include <regex>
#include "pathfinder.h"
#include <sstream>

extern Map_data const *map_data;
extern t_x11_state x11_state;
extern int zoom_count;
extern bool do_not_zoom;

Draw_data *draw_data;
std::string map_name_;
volatile bool change_map_on = false;
const int unused = -1;

const double PI2 = 6.28318530718;

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
unsigned text_offset = 0;
bool show_found_path = false;

std::chrono::time_point<std::chrono::system_clock> start_time, current_time;
int screen_count = 0;


/****************************************************************************************
                                  Function Declarations
 ****************************************************************************************/
void draw_screen();
    void init_search_box_related();

//Thread helper functions
void update_draw_data();

void force_redraw();

//Draw functions
void draw_features();
    bool is_bigger_than(feature first, feature second);
    int check_out_copy(feature_dynamic_data* data_set, int thread_id);
    void set_colour_for_feature(feature_colour colour);
    void draw_area_features_from_vector(std::vector<feature>& position_features);
    void draw_line_features_from_vector(std::vector<feature>& position_features);
    
void draw_poi();
    int check_out_copy(poi_dynamic_data* data_set, int thread_id);
    void draw_poi_icon_and_name(poi current_poi); 

void draw_street_segments();
    int check_out_copy(street_segment_dynamic_data* data_set, int thread_id);
    void draw_street_segments_from_set(std::set<street_segment_data>& position_segments, t_color normal_colour, t_color highlight_colour);
    void draw_line_between(LatLon point_1, LatLon point_2, t_color color, double width, bool draw_circle);
    
void draw_street_segment_names();
    void draw_street_segments_names_from_set(std::set<street_segment_data>& position_segments, t_color colour);
    void draw_segment_name(street_segment_data street_segment_info);
    double compute_rotation_angle(coordinate point_1, coordinate point_2, std::string &street_name, bool one_way);
    double distance_between_points(coordinate point_1, coordinate point_2);\

void draw_search_box_related();
    
//Button functions

void act_on_mouse_button(float x, float y, t_event_buttonPressed button_info);
    void show_poi(void (*draw_screen_ptr)(void)); 
    void toggle_poi_basic(void (*draw_screen_poi)(void));
    void poi_extra(void (*draw_screen_poi)(void));
    void show_poi_info(unsigned POI_index);
    void show_intersection_info(unsigned intersection_index);
    void reset_button(void (*draw_screen_)(void));
    void help_button(void (*draw_screen_)(void));

void act_on_key_press(char c, int keysym);
    void search_parser();
    void split_results(std::vector<search_return_data> &results, std::vector<search_return_data> &street_results, 
            std::vector<search_return_data> &intersection_results, std::vector<search_return_data> &poi_results);
    bool intersection_street_name_split(std::string input, std::string &street_name_1, std::string &street_name_2, std::string delimiter);
    
//Path finding related
typedef struct directions {
    int direction_int;
    std::string direction_string;
    double distance;
    double time;
} directions;
void draw_route(std::vector<unsigned> path);
void draw_route_segments(std::vector<unsigned> path_segments);
int direction_of_travel(unsigned from_intersection, unsigned middle_intersection,unsigned to_intersection);
std::vector<directions> format_directions(std::vector<unsigned> path);
unsigned find_segment_between_intersections(unsigned from, unsigned to);
void enable_find_path_buttons(void (*draw_screen_map)(void));
void update_found_path_intersection(std::vector<unsigned> path_intersection_id);
void format_path_data_1(unsigned from_id, unsigned to_id);
void format_path_data_2_intersection(std::vector<unsigned> path_intersection_id);
std::vector<unsigned> segments_to_intersections(std::vector<unsigned> path_segments);

/****************************************************************************************
                                  Function Implementations
 ****************************************************************************************/
/****************************************************************************************
                                 Function Implementations
 ****************************************************************************************/
void draw_map() {
    
    draw_data = new Draw_data;
    zoom_count = 0;
    
    init_graphics(map_data->get_place_name(), BLACK);
    set_visible_world(map_data->get_min_x(), map_data->get_min_y(), map_data->get_max_x(), map_data->get_max_y());
  
    //creating threads for updating map elements in the background
    std::thread t1_update_draw_data(update_draw_data);

    create_button("Zoom Out", "Change Map", enable_change_map_buttons);
    create_button("Change Map", "Find Path", enable_find_path_buttons);
    create_button("Find Path", "Show POI", show_poi);
    create_button("Exit", "Reset", reset_button);
    create_button("Reset", "Help", enable_help_button);

    initialize_ui_elements();
    init_search_box_related();
    
    start_time = std::chrono::system_clock::now();
    
    event_loop(act_on_mouse_button, nullptr, act_on_key_press, draw_screen);
    
    draw_data->disable_map();
    
    //don't quit until these threads are done
    t1_update_draw_data.join();

    delete draw_data;
    delete_all_ui_elements();
    
    close_graphics();
}

//does the actual drawing of the screen
//called in a loop by event_loop and other functions as well
void draw_screen() {
    
    current_time = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = current_time - start_time;
    
    if (elapsed_seconds.count() > 1.0) {
        std::cout << "Current frame rate: " << screen_count << " fps.\n";
        screen_count = 0;
        start_time = std::chrono::system_clock::now();
    }
    else {
        screen_count++;
    }
    
    t_bound_box current_world = get_visible_world();
    
    if (draw_data->search_box.get_is_active()) {
        set_keypress_input(true);
    } else {
        set_keypress_input(false);
    }
    
    //update screen center and bounds
    draw_data->set_current_view_center(current_world.get_xcenter(), current_world.get_ycenter());
    draw_data->set_current_bounds(get_visible_world());
    draw_data->set_current_screen_bounds(get_visible_screen());
    
    //enable buffer -> draw off screen
    set_drawing_buffer(OFF_SCREEN);
    
    clearscreen();
    
    draw_data->clear_position_to_street_name();

    draw_features();
    
    draw_street_segments();
    
    if (draw_data->show_found_path) {
        draw_route_segments(draw_data->found_path);
    } 
    
    draw_street_segment_names();
    
    if(draw_data->get_draw_poi()) {
        draw_poi();       
    }
    
    //clear message if nothing selected
    update_message("");
    
    //draw a red circle when click on an intersection
    if (draw_data->get_intersection_selected() && !r_click_menu1->get_exists()) {
        unsigned selected_intersection = draw_data->get_selected_intersection_id();
        LatLon position = getIntersectionPosition(selected_intersection);
        coordinate intersection_location = map_data->latLon_to_coordinates(position);
        
        setlinewidth(3);
        setcolor_by_name("red");
        drawarc(intersection_location.x, intersection_location.y, 0.0001, 0, 360);
        show_intersection_info(selected_intersection);
    }
    
    //draw a yellow circle when click on a poi
    if (draw_data->get_poi_selected() && !r_click_menu1->get_exists()) {
        unsigned selected_poi = draw_data->get_selected_poi_id();
        LatLon position = getPointOfInterestPosition(selected_poi);
        coordinate poi_location = map_data->latLon_to_coordinates(position);
    
        setlinewidth(3);
        setcolor_by_name("yellow");
        drawarc(poi_location.x, poi_location.y, 0.0001, 0, 360);
        show_poi_info(selected_poi);
    }
    
    //draw a black icon on the start point
    if (draw_data->from_x != FLT_MAX) {
        t_point screen = world_to_scrn(t_point(draw_data->from_x, draw_data->from_y));
        t_point actual_screen = t_point(screen.x - 20, screen.y - 42);
        t_point world_icon = scrn_to_world(actual_screen);
      
        draw_surface(load_png_from_file("libstreetmap/resources/from.png"), world_icon.x, world_icon.y);      
    }
    
    //draw a red icon on the end point
    if (draw_data->to_x != FLT_MAX) {
        t_point screen = world_to_scrn(t_point(draw_data->to_x, draw_data->to_y));
        t_point actual_screen = t_point(screen.x - 20, screen.y - 42);
        t_point world_icon = scrn_to_world(actual_screen);
      
        draw_surface(load_png_from_file("libstreetmap/resources/to.png"), world_icon.x, world_icon.y);      
    }
    
    //draw a black icon on the start point
    if (draw_data->find_poi && draw_data->find_poi_x != FLT_MAX) {
        t_point screen = world_to_scrn(t_point(draw_data->find_poi_x, draw_data->find_poi_y));
        t_point actual_screen = t_point(screen.x - 20, screen.y - 42);
        t_point world_icon = scrn_to_world(actual_screen);
      
        draw_surface(load_png_from_file("libstreetmap/resources/from.png"), world_icon.x, world_icon.y);      
    }
    
    draw_data->draw_all_ui_elements();
    
    draw_search_box_related();
    
    if (zoom_count == 0) {
        zoom_fit_();
    }
    
    //copy everything drawn to the screen
    copy_off_screen_buffer_to_screen();
}

void init_search_box_related() {
    
    //Actual search box settings
    double search_box_top = 0.0;
    double search_box_bottom = 0.05;
    double search_box_left = 0.0;
    double search_box_right = 0.275;
    double search_button_right = search_box_right + 0.025;
    
    draw_data->search_box.set_edges(search_box_top, search_box_bottom, search_box_left, search_box_right);
    draw_data->search_box.set_border_colour(255, 255, 255, 255);
    draw_data->search_box.set_active_colour(255, 255, 255, 255);
    draw_data->search_box.set_inactive_colour(255, 255, 255, 255);
    
    //The button beside the search box to clear searches
    draw_data->clear_search.set_edges(search_box_top, search_box_bottom, search_box_right, search_button_right);
    draw_data->clear_search.set_icon("libstreetmap/resources/X.png", 47, 50);
    draw_data->clear_search.set_border_colour(255, 255, 255, 255);
    draw_data->clear_search.set_active_colour(255, 255, 255, 255);
    draw_data->clear_search.set_inactive_colour(255, 255, 255, 255);
    
    //The autocomplete listing thats shows what parser processed
    draw_data->autocomplete_box.set_edges(search_box_top, search_box_bottom, search_box_left, search_box_right);
    draw_data->autocomplete_box.set_border_colour(255, 0, 0, 255);
    draw_data->autocomplete_box.set_active_colour(255, 255, 255, 255);
    draw_data->autocomplete_box.set_inactive_colour(255, 255, 255, 255);
    draw_data->autocomplete_box.set_number_of_each_result_to_show(3);
    draw_data->autocomplete_box.set_street_result_box_colour(255, 255, 200, 255);
    draw_data->autocomplete_box.set_intersection_result_box_colour(255, 200, 255, 255);
    draw_data->autocomplete_box.set_poi_result_box_colour(200, 255, 255, 255);
    
}

void update_draw_data() {
    const int thread_id = 1;
    
    feature_dynamic_data* area_feature_data = draw_data->get_feature_data(1);
    feature_dynamic_data* line_feature_data = draw_data->get_feature_data(2);
    poi_dynamic_data* poi_data = draw_data->get_poi_data();
    street_segment_dynamic_data* highway_street_segment_thread_data = draw_data->get_street_segment_data(5);
    street_segment_dynamic_data* major_street_segment_thread_data = draw_data->get_street_segment_data(6);
    street_segment_dynamic_data* minor_street_segment_thread_data = draw_data->get_street_segment_data(7);
    
    while (draw_data->get_map_active()) {
    
        /*
         * Primary copies
         */
        
        coordinate current_center = draw_data->get_current_view_center();
        t_bound_box current_bounds = draw_data->get_current_bounds();
        t_bound_box current_screen_bounds = draw_data->get_current_screen_bounds();
        
        //area features
        if (current_bounds.top() != area_feature_data->last_acknowledged_bounds.top() 
                || current_center.x != area_feature_data->last_acknowledged_center.x
                || current_center.y != area_feature_data->last_acknowledged_center.y) {
            
            //determine a minimum size required of the area to bother drawing
            double area_per_pixel = current_bounds.area() / current_screen_bounds.area();
            double cutoff = 25*area_per_pixel;
                        
            //first copy into a set to avoid duplicates
            std::set<feature_basic> new_feature_basic_set;
            
            //load new data
            for (int x = map_data->get_x_index(current_bounds.left()); x <= map_data->get_x_index(current_bounds.right()); x++) {
                for (int y = map_data->get_y_index(current_bounds.bottom()); y <= map_data->get_y_index(current_bounds.top()); y++) {
                    
                    std::vector<feature_basic> features_at_position = map_data->get_area_feature_at_position(x, y);
                    for (auto i = features_at_position.begin(); i != features_at_position.end(); i++) {
                        if (i->size > cutoff) {
                            new_feature_basic_set.insert(*i);
                        }
                    }
                }
            }
            
            //load the full data set for drawing to use
            std::set<feature> new_feature_set;
            for (auto i = new_feature_basic_set.begin(); i != new_feature_basic_set.end(); i++) {
                feature current_feature;
                current_feature.id = i->id;
                current_feature.colour = map_data->get_feature_colour_from_type(getFeatureType(i->id));
                current_feature.size = i->size;
                current_feature.npoints = getFeaturePointCount(i->id);
                current_feature.points = map_data->get_feature_points_t_point(i->id);
                new_feature_set.insert(current_feature);
            }
         
            //copy into a vector for easier reading
            //sort by size so largest features are drawn first
            std::vector<feature> new_feature_vector;
            new_feature_vector.assign(new_feature_set.begin(), new_feature_set.end());
            std::sort(new_feature_vector.begin(), new_feature_vector.end(), is_bigger_than);
            
            while (area_feature_data->primary_copy_editing_id != unused) {
                ; //wait for the copy to become available
            }
            //I'm super paranoid about data races, so declare editing, 
            //then check to make sure no one declared editing at the same time
            //and declare a second time
            area_feature_data->primary_copy_editing_id = thread_id;
            while (area_feature_data->primary_copy_editing_id != thread_id && 
                    area_feature_data->primary_copy_editing_id != unused) {
                ; //sit there and be sad that Anna's worst fears have come to life
            }
            area_feature_data->primary_copy_editing_id = thread_id;
            
            //copy into primary copy
            area_feature_data->primary_copy.swap(new_feature_vector);
            
            //once done, release lock on editing
            area_feature_data->primary_copy_editing_id = unused;
            area_feature_data->secondary_copy_up_to_date = false;
            area_feature_data->last_acknowledged_center = current_center;
            area_feature_data->last_acknowledged_bounds = current_bounds;
        }
        
        //line features
        else if (current_bounds.top() != line_feature_data->last_acknowledged_bounds.top() 
                || current_center.x != line_feature_data->last_acknowledged_center.x
                || current_center.y != line_feature_data->last_acknowledged_center.y) {
            
            //determine a minimum size required of the length to bother drawing
            double length_per_pixel = sqrt(current_bounds.area() / current_screen_bounds.area());
            double cutoff = 5*length_per_pixel;
            
            //first copy into a set to avoid duplicates
            std::set<feature_basic> new_feature_basic_set;
            
            //load new data
            for (int x = map_data->get_x_index(current_bounds.left()); x <= map_data->get_x_index(current_bounds.right()); x++) {
                for (int y = map_data->get_y_index(current_bounds.bottom()); y <= map_data->get_y_index(current_bounds.top()); y++) {
                    
                    std::vector<feature_basic> features_at_position = map_data->get_line_feature_at_position(x, y);
                    for (auto i = features_at_position.begin(); i != features_at_position.end(); i++) {
                        if (i->size > cutoff) {
                            new_feature_basic_set.insert(*i);
                        }
                    }
                }
            }
            
            //load the full data set for drawing to use
            std::set<feature> new_feature_set;
            for (auto i = new_feature_basic_set.begin(); i != new_feature_basic_set.end(); i++) {
                feature current_feature;
                current_feature.id = i->id;
                current_feature.colour = map_data->get_feature_colour_from_type(getFeatureType(i->id));
                current_feature.size = i->size;
                current_feature.npoints = getFeaturePointCount(i->id);
                current_feature.points = map_data->get_feature_points_t_point(i->id);
                new_feature_set.insert(current_feature);
            }
            
            //copy into a vector for easier reading
            //sort by size so largest features are drawn first
            std::vector<feature> new_feature_vector(new_feature_set.begin(), new_feature_set.end());
            std::sort(new_feature_vector.begin(), new_feature_vector.end(), is_bigger_than);
            
            while (line_feature_data->primary_copy_editing_id != unused) {
                ; //wait for the copy to become available
            }
            //I'm super paranoid about data races, so declare editing, 
            //then check to make sure no one declared editing at the same time
            //and declare a second time
            line_feature_data->primary_copy_editing_id = thread_id;
            while (line_feature_data->primary_copy_editing_id != thread_id && 
                    line_feature_data->primary_copy_editing_id != unused) {
                ; //sit there and be sad that Anna's worst fears have come to life
            }
            line_feature_data->primary_copy_editing_id = thread_id;
            
            //copy into primary copy
            line_feature_data->primary_copy.swap(new_feature_vector);
            
            //once done, release lock on editing
            line_feature_data->primary_copy_editing_id = unused;
            line_feature_data->secondary_copy_up_to_date = false;
            line_feature_data->last_acknowledged_center = current_center;
            line_feature_data->last_acknowledged_bounds = current_bounds;
        }
        
        //poi data
        else if (current_bounds.top() != poi_data->last_acknowledged_bounds.top() 
                ||current_center.x != poi_data->last_acknowledged_center.x
                || current_center.y != poi_data->last_acknowledged_center.y) {
            std::set<poi> new_poi_data;
            
            //load new data
            t_bound_box current_screen = get_visible_world();
            
            for (int x = map_data->get_x_index(current_screen.left()); x <= map_data->get_x_index(current_screen.right()); x++) {
                for (int y = map_data->get_y_index(current_screen.bottom()); y <= map_data->get_y_index(current_screen.top()); y++) {
                    //get the POI index from position
                    std::vector<poi_basic> POI_at_position = map_data->get_poi_at_position(x,y);
                    
                    //load each POI into the set
                    for(auto i = POI_at_position.begin(); i != POI_at_position.end(); i++) {
                        poi current_poi;
                        current_poi.id = i->id;
                        current_poi.name = i->name;
                        current_poi.type = getPointOfInterestType(i->id);
                        float xp =  map_data->latLon_to_coordinates(getPointOfInterestPosition(i->id)).x;
                        float yp =  map_data->latLon_to_coordinates(getPointOfInterestPosition(i->id)).y;

                        //adjust coordinates for icons (png size = 32x37)
                        t_point screen = world_to_scrn(t_point(xp,yp));
                        t_point actual_screen_icon = t_point(screen.x - 16, screen.y - 24);
                        t_point actual_screen_name = t_point(screen.x, screen.y + 25);
                        t_point world_icon = scrn_to_world(actual_screen_icon);
                        t_point world_name = scrn_to_world(actual_screen_name);
                        current_poi.icon_position.x = world_icon.x;
                        current_poi.icon_position.y = world_icon.y;
                        current_poi.text_position.x = world_name.x;
                        current_poi.text_position.y = world_name.y;
                        new_poi_data.insert(current_poi);
                    }
                }
            }
            
            while (poi_data->primary_copy_editing_id != unused) {
                ; //wait for the copy to become available
            }
            poi_data->primary_copy_editing_id = thread_id;
            while (poi_data->primary_copy_editing_id != thread_id && 
                    poi_data->primary_copy_editing_id != unused) {
                ; 
            }
            poi_data->primary_copy_editing_id = thread_id;
            
            //copy into primary copy
            poi_data->primary_copy = new_poi_data;
            
            //once done, release lock on editing
            poi_data->primary_copy_editing_id = unused;
            poi_data->secondary_copy_up_to_date = false;
            poi_data->last_acknowledged_center = current_center;
            poi_data->last_acknowledged_bounds = current_bounds;
        }
        
        //highways
        else if (current_bounds.top() != highway_street_segment_thread_data->last_acknowledged_bounds.top() 
                || current_center.x != highway_street_segment_thread_data->last_acknowledged_center.x
                || current_center.y != highway_street_segment_thread_data->last_acknowledged_center.y
                || draw_data->get_highway_segment_reload()) {
            draw_data->disable_highway_segment_reload();
            std::set<street_segment_data> new_highway_segment_data;
            std::unordered_map<unsigned, std::vector<unsigned>> calc_name_segments;
            
            //load new data
            t_bound_box current_screen = get_visible_world();
            for (int x = map_data->get_x_index(current_screen.left()); x <= map_data->get_x_index(current_screen.right()); x++) {
                for (int y = map_data->get_y_index(current_screen.bottom()); y <= map_data->get_y_index(current_screen.top()); y++) {
                    
                    std::vector<street_segment_data> highway_segments_at_position = map_data->get_highway_street_segment_at_position(x, y);
                    for (auto i = highway_segments_at_position.begin(); i != highway_segments_at_position.end(); i++) {
                        auto it = draw_data->selected_street_ids.find(i->info.streetID);
                        if (it != draw_data->selected_street_ids.end() && it->second) {
                            i->selected = it->second;
                        }
                        new_highway_segment_data.insert(*i);
                        calc_name_segments[i->info.streetID].push_back(i->id);
                    }
                }
            }
            
            for (auto i = calc_name_segments.begin(); i != calc_name_segments.end(); i++) {
                std::sort(i->second.begin(), i->second.end());
                int number_of_segments = i->second.size();
                if (number_of_segments == 1) {
                    continue;
                }
                
                int highway_few_segments_threshold = 2;
                int highway_many_segments_threshold = 8;
                
                int threshold;
                
                if (number_of_segments <= 6) {
                    threshold = highway_few_segments_threshold;
                } else {
                    threshold = highway_many_segments_threshold;
                }
                
                i->second.erase(i->second.begin());
                auto it = i->second.begin();
                while (i->second.size() != 0) {
                    
                    auto set_it = new_highway_segment_data.find(*it);
                    street_segment_data modified_data = *set_it;
                    modified_data.draw_name = true;
                    
                    LatLon from_pos = getIntersectionPosition(modified_data.info.from);
                    LatLon to_pos = getIntersectionPosition(modified_data.info.to);
                    
                    LatLon segment_middle_pos((from_pos.lat() + to_pos.lat()) / 2, (from_pos.lon() + to_pos.lon()) / 2);
                    
                    int x = map_data->get_x_index(segment_middle_pos.lat());
                    int y = map_data->get_y_index(segment_middle_pos.lon());
                    
                    if (!draw_data->get_position_to_street_name(x, y)) {
                        draw_data->update_position_to_street_name(x, y, 0, 0);

                        new_highway_segment_data.erase(set_it);
                        new_highway_segment_data.insert(modified_data);
                        for (int j = 0; j < threshold && it != i->second.end(); j++) {
                            i->second.erase(it);
                        }
                    } else {
                        i->second.erase(it);
                    }
                }
            }
            
            while (highway_street_segment_thread_data->primary_copy_editing_id != unused) {
                ; //wait for the copy to become available
            }
            //I'm super paranoid about data races, so declare editing, 
            //then check to make sure no one declared editing at the same time
            //and declare a second time
            highway_street_segment_thread_data->primary_copy_editing_id = thread_id;
            while (highway_street_segment_thread_data->primary_copy_editing_id != thread_id && 
                    highway_street_segment_thread_data->primary_copy_editing_id != unused) {
                ; //sit there and be sad that Anna's worst fears have come to life
            }
            highway_street_segment_thread_data->primary_copy_editing_id = thread_id;
            
            //copy into primary copy
            highway_street_segment_thread_data->primary_copy = new_highway_segment_data;
            
            //once done, release lock on editing
            highway_street_segment_thread_data->primary_copy_editing_id = unused;
            highway_street_segment_thread_data->secondary_copy_up_to_date = false;
            highway_street_segment_thread_data->last_acknowledged_center = current_center;
            highway_street_segment_thread_data->last_acknowledged_bounds = current_bounds;
        }
        
        //major streets
        else if (current_bounds.top() != major_street_segment_thread_data->last_acknowledged_bounds.top() 
                || current_center.x != major_street_segment_thread_data->last_acknowledged_center.x
                || current_center.y != major_street_segment_thread_data->last_acknowledged_center.y
                || draw_data->get_major_segment_reload()) {
            draw_data->disable_major_segment_reload();
            std::set<street_segment_data> new_major_segment_data;
            std::unordered_map<unsigned, std::vector<unsigned>> calc_name_segments;
            
            //load new data
            t_bound_box current_screen = get_visible_world();
            for (int x = map_data->get_x_index(current_screen.left()); x <= map_data->get_x_index(current_screen.right()); x++) {
                for (int y = map_data->get_y_index(current_screen.bottom()); y <= map_data->get_y_index(current_screen.top()); y++) {
                    
                    std::vector<street_segment_data> major_segments_at_position = map_data->get_major_street_segment_at_position(x, y);
                    for (auto i = major_segments_at_position.begin(); i != major_segments_at_position.end(); i++) {
                        auto it = draw_data->selected_street_ids.find(i->info.streetID);
                        if (it != draw_data->selected_street_ids.end() && it->second) {
                            i->selected = it->second;
                        }
                        new_major_segment_data.insert(*i);
                        calc_name_segments[i->info.streetID].push_back(i->id);
                    }
                }
            }
            
            for (auto i = calc_name_segments.begin(); i != calc_name_segments.end(); i++) {
                std::sort(i->second.begin(), i->second.end());
                int number_of_segments = i->second.size();
                if (number_of_segments == 1) {
                    continue;
                }
                
                int major_few_segments_threshold = 2;
                int major_many_segments_threshold = 8;
                
                int threshold;
                
                if (number_of_segments <= 6) {
                    threshold = major_few_segments_threshold;
                } else {
                    threshold = major_many_segments_threshold;
                }
                
                i->second.erase(i->second.begin());
                auto it = i->second.begin();
                while (i->second.size() != 0) {
                    auto set_it = new_major_segment_data.find(*it);
                    street_segment_data modified_data = *set_it;
                    modified_data.draw_name = true;
                    
                    new_major_segment_data.erase(set_it);
                    new_major_segment_data.insert(modified_data);
                    for (int j = 0; j < threshold && it != i->second.end(); j++) {
                        i->second.erase(it);
                    }
                }
            }
            
            while (major_street_segment_thread_data->primary_copy_editing_id != unused) {
                ; //wait for the copy to become available
            }
            //I'm super paranoid about data races, so declare editing, 
            //then check to make sure no one declared editing at the same time
            //and declare a second time
            major_street_segment_thread_data->primary_copy_editing_id = thread_id;
            while (major_street_segment_thread_data->primary_copy_editing_id != thread_id && 
                    major_street_segment_thread_data->primary_copy_editing_id != unused) {
                ; //sit there and be sad that Anna's worst fears have come to life
            }
            major_street_segment_thread_data->primary_copy_editing_id = thread_id;
            
            //copy into primary copy
            major_street_segment_thread_data->primary_copy = new_major_segment_data;
            
            //once done, release lock on editing
            major_street_segment_thread_data->primary_copy_editing_id = unused;
            major_street_segment_thread_data->secondary_copy_up_to_date = false;
            major_street_segment_thread_data->last_acknowledged_center = current_center;
            major_street_segment_thread_data->last_acknowledged_bounds = current_bounds;
        }
        
        //minor streets
        else if (current_bounds.top() != minor_street_segment_thread_data->last_acknowledged_bounds.top() 
                || current_center.x != minor_street_segment_thread_data->last_acknowledged_center.x
                || current_center.y != minor_street_segment_thread_data->last_acknowledged_center.y
                || draw_data->get_minor_segment_reload()) {
            draw_data->disable_minor_segment_reload();
            std::set<street_segment_data> new_minor_segment_data;
            std::unordered_map<unsigned, std::vector<unsigned>> calc_name_segments;

                //load new data
                t_bound_box current_screen = get_visible_world();
                for (int x = map_data->get_x_index(current_screen.left()); x <= map_data->get_x_index(current_screen.right()); x++) {
                    for (int y = map_data->get_y_index(current_screen.bottom()); y <= map_data->get_y_index(current_screen.top()); y++) {

                        std::vector<street_segment_data> minor_segments_at_position = map_data->get_minor_street_segment_at_position(x, y);
                        for (auto i = minor_segments_at_position.begin(); i != minor_segments_at_position.end(); i++) {
                            auto it = draw_data->selected_street_ids.find(i->info.streetID);
                            if (it != draw_data->selected_street_ids.end() && it->second) {
                                i->selected = it->second;
                            }
                            new_minor_segment_data.insert(*i);
                            calc_name_segments[i->info.streetID].push_back(i->id);
                        }
                    }
                }

                for (auto i = calc_name_segments.begin(); i != calc_name_segments.end(); i++) {
                    std::sort(i->second.begin(), i->second.end());
                    int number_of_segments = i->second.size();
                    if (number_of_segments == 1) {
                        continue;
                    }

                    int minor_few_segments_threshold = 2;
                    int minor_many_segments_threshold = 8;

                    int threshold;

                    if (number_of_segments <= 6) {
                        threshold = minor_few_segments_threshold;
                    } else {
                        threshold = minor_many_segments_threshold;
                    }

                    i->second.erase(i->second.begin());
                    auto it = i->second.begin();
                    while (i->second.size() != 0) {
                        auto set_it = new_minor_segment_data.find(*it);
                        street_segment_data modified_data = *set_it;
                        modified_data.draw_name = true;

                        new_minor_segment_data.erase(set_it);
                        new_minor_segment_data.insert(modified_data);
                        for (int j = 0; j < threshold && it != i->second.end(); j++) {
                            i->second.erase(it);
                        }
                    }
                }

            while (minor_street_segment_thread_data->primary_copy_editing_id != unused) {
                ; //wait for the copy to become available
            }
            //I'm super paranoid about data races, so declare editing, 
            //then check to make sure no one declared editing at the same time
            //and declare a second time
            minor_street_segment_thread_data->primary_copy_editing_id = thread_id;
            while (minor_street_segment_thread_data->primary_copy_editing_id != thread_id && 
                    minor_street_segment_thread_data->primary_copy_editing_id != unused) {
                ; //sit there and be sad that Anna's worst fears have come to life
            }
            minor_street_segment_thread_data->primary_copy_editing_id = thread_id;

            //copy into primary copy
            minor_street_segment_thread_data->primary_copy = new_minor_segment_data;

            //once done, release lock on editing
            minor_street_segment_thread_data->primary_copy_editing_id = unused;
            minor_street_segment_thread_data->secondary_copy_up_to_date = false;
            minor_street_segment_thread_data->last_acknowledged_center = current_center;
            minor_street_segment_thread_data->last_acknowledged_bounds = current_bounds;
        }

        /*
         * Secondary copies
         */
        
        //area features
        else if (!area_feature_data->secondary_copy_up_to_date) {
            while (area_feature_data->secondary_copy_editing_id != unused) {
                ; //wait for the copy to become available
            }
            //I'm super paranoid about data races, so declare editing, 
            //then check to make sure no one declared editing at the same time
            //and declare a second time
            area_feature_data->secondary_copy_editing_id = thread_id;
            while (area_feature_data->secondary_copy_editing_id != thread_id && 
                    area_feature_data->secondary_copy_editing_id != unused) {
                ; //sit there and be sad that Anna's worst fears have come to life
            }
            area_feature_data->secondary_copy_editing_id = thread_id;
            
            //copy primary copy into secondary copy
            area_feature_data->secondary_copy = area_feature_data->primary_copy;
            
            //once done, release lock on editing
            area_feature_data->secondary_copy_editing_id = unused;
            area_feature_data->secondary_copy_up_to_date = true;
        }
        
        //line features
        else if (!line_feature_data->secondary_copy_up_to_date) {
            while (line_feature_data->secondary_copy_editing_id != unused) {
                ; //wait for the copy to become available
            }
            //I'm super paranoid about data races, so declare editing, 
            //then check to make sure no one declared editing at the same time
            //and declare a second time
            line_feature_data->secondary_copy_editing_id = thread_id;
            while (line_feature_data->secondary_copy_editing_id != thread_id && 
                    line_feature_data->secondary_copy_editing_id != unused) {
                ; //sit there and be sad that Anna's worst fears have come to life
            }
            line_feature_data->secondary_copy_editing_id = thread_id;
            
            //copy primary copy into secondary copy
            line_feature_data->secondary_copy = line_feature_data->primary_copy;
            
            //once done, release lock on editing
            line_feature_data->secondary_copy_editing_id = unused;
            line_feature_data->secondary_copy_up_to_date = true;
        }
        
        //poi data
        else if (!poi_data->secondary_copy_up_to_date) {
            while (poi_data->secondary_copy_editing_id != unused) {
                ; //wait for the copy to become available
            }
            //I'm super paranoid about data races, so declare editing, 
            //then check to make sure no one declared editing at the same time
            //and declare a second time
            poi_data->secondary_copy_editing_id = thread_id;
            while (poi_data->secondary_copy_editing_id != thread_id && 
                    poi_data->secondary_copy_editing_id != unused) {
                ; //sit there and be sad that Anna's worst fears have come to life
            }
            poi_data->secondary_copy_editing_id = thread_id;
            
            //copy primary copy into secondary copy
            poi_data->secondary_copy = poi_data->primary_copy;
            
            //once done, release lock on editing
            poi_data->secondary_copy_editing_id = unused;
            poi_data->secondary_copy_up_to_date = true;
        }
        
        //highways
        else if (!highway_street_segment_thread_data->secondary_copy_up_to_date) {
            while (highway_street_segment_thread_data->secondary_copy_editing_id != unused) {
                ; //wait for the copy to become available
            }
            //I'm super paranoid about data races, so declare editing, 
            //then check to make sure no one declared editing at the same time
            //and declare a second time
            highway_street_segment_thread_data->secondary_copy_editing_id = thread_id;
            while (highway_street_segment_thread_data->secondary_copy_editing_id != thread_id && 
                    highway_street_segment_thread_data->secondary_copy_editing_id != unused) {
                ; //sit there and be sad that Anna's worst fears have come to life
            }
            highway_street_segment_thread_data->secondary_copy_editing_id = thread_id;
            
            //copy primary copy into secondary copy
            highway_street_segment_thread_data->secondary_copy = highway_street_segment_thread_data->primary_copy;
            
            //once done, release lock on editing
            highway_street_segment_thread_data->secondary_copy_editing_id = unused;
            highway_street_segment_thread_data->secondary_copy_up_to_date = true;
        }
        
        //major streets
        else if (!major_street_segment_thread_data->secondary_copy_up_to_date) {
            while (major_street_segment_thread_data->secondary_copy_editing_id != unused) {
                ; //wait for the copy to become available
            }
            //I'm super paranoid about data races, so declare editing, 
            //then check to make sure no one declared editing at the same time
            //and declare a second time
            major_street_segment_thread_data->secondary_copy_editing_id = thread_id;
            while (major_street_segment_thread_data->secondary_copy_editing_id != thread_id && 
                    major_street_segment_thread_data->secondary_copy_editing_id != unused) {
                ; //sit there and be sad that Anna's worst fears have come to life
            }
            major_street_segment_thread_data->secondary_copy_editing_id = thread_id;
            
            //copy primary copy into secondary copy
            major_street_segment_thread_data->secondary_copy = major_street_segment_thread_data->primary_copy;
            
            //once done, release lock on editing
            major_street_segment_thread_data->secondary_copy_editing_id = unused;
            major_street_segment_thread_data->secondary_copy_up_to_date = true;
        }
        
        //minor streets
        else if (!minor_street_segment_thread_data->secondary_copy_up_to_date) {
                while (minor_street_segment_thread_data->secondary_copy_editing_id != unused) {
                    ; //wait for the copy to become available
                }
                //I'm super paranoid about data races, so declare editing, 
                //then check to make sure no one declared editing at the same time
                //and declare a second time
                minor_street_segment_thread_data->secondary_copy_editing_id = thread_id;
                while (minor_street_segment_thread_data->secondary_copy_editing_id != thread_id && 
                        minor_street_segment_thread_data->secondary_copy_editing_id != unused) {
                    ; //sit there and be sad that Anna's worst fears have come to life
                }
                minor_street_segment_thread_data->secondary_copy_editing_id = thread_id;

                //copy primary copy into secondary copy
                minor_street_segment_thread_data->secondary_copy = minor_street_segment_thread_data->primary_copy;

                //once done, release lock on editing
                minor_street_segment_thread_data->secondary_copy_editing_id = unused;
                minor_street_segment_thread_data->secondary_copy_up_to_date = true;
            }
        
        /*
         * Refresh map when idle
         */
        else {
            //delay the redraw to something reasonable
            for (int count = 0; draw_data->get_map_active() && count < 100000000; count++) {
                ;//wasting electricity
            }

            //force the screen to redraw
            //note: constant redraws slows down the map
            XClearArea(x11_state.display, x11_state.toplevel, 0, 0, 1, 1, true);
            XFlush(x11_state.display);
        }        
    }
}

//higher level function for drawing all features on the map
//called by draw_screen()
void draw_features() {
        
    setlinewidth(1.0);
    const int thread_id = 0;
    
    //draw in order of descending size to avoid drawing oceans over islands, etc
    //draw area features before line features to avoid drawing parks over rivers
    
    //area features
    feature_dynamic_data* data_to_draw = draw_data->get_feature_data(1); //1 -> area features
    
    //take out a copy for drawing
    int copy_to_use = check_out_copy(data_to_draw, thread_id);
    
    if (copy_to_use == 1) {
        draw_area_features_from_vector(data_to_draw->primary_copy);
        //release lock
        data_to_draw->primary_copy_editing_id = unused;
    }
    else {
        draw_area_features_from_vector(data_to_draw->secondary_copy);
        //release lock
        data_to_draw->secondary_copy_editing_id = unused;
    }
    
    
    //line features
    data_to_draw = draw_data->get_feature_data(2); //2 -> line features
    
    //take out a copy for drawing
    copy_to_use = check_out_copy(data_to_draw, thread_id);
    
    if (copy_to_use == 1) {
        draw_line_features_from_vector(data_to_draw->primary_copy);
        //release lock
        data_to_draw->primary_copy_editing_id = unused;
    }
    else {
        draw_line_features_from_vector(data_to_draw->secondary_copy);
        //release lock
        data_to_draw->secondary_copy_editing_id = unused;
    }
}

//return whether first is bigger than second (area/length)
bool is_bigger_than(feature first, feature second) {
    return first.size > second.size;
}
    
//used to reserve a copy of drawing data
//ensures nothing else will touch the data while it's being used
//releasing the lock must be done manually afterwards
int check_out_copy(feature_dynamic_data* data_set, int thread_id) {
    int copy_to_use = 1;
    int successful_locks = 0;
    
    //check multiple times just in case something else is trying to edit at the same time
    //Anna is paranoid, sorry!
    while (successful_locks < 2) {
        if (copy_to_use == 1) {
            if (data_set->primary_copy_editing_id == unused || data_set->primary_copy_editing_id == thread_id) {
                data_set->primary_copy_editing_id = thread_id;
                successful_locks++;
            }
            else {
                copy_to_use = 2;
                successful_locks = 0;
            }
        }
        else {
            if (data_set->secondary_copy_editing_id == unused || data_set->secondary_copy_editing_id == thread_id) {
                data_set->secondary_copy_editing_id = thread_id;
                successful_locks++;
            }
            else {
                copy_to_use = 1;
                successful_locks = 0;
            }
        }
    }
    return copy_to_use;
}

void set_colour_for_feature(feature_colour colour) {
    if (colour == green) {
        t_color green_colour(51, 122, 51);
        setcolor(green_colour);
    }
    else if (colour == blue) {
        t_color blue_colour(71, 102, 128);
        setcolor(blue_colour);
    }
    else if (colour == background) {
        t_color background_colour(95, 95, 95);
        setcolor(background_colour);
    }
    else {
        t_color other_colour(51, 51, 51);
        setcolor(other_colour);
    }
}

void draw_area_features_from_vector(std::vector<feature>& position_features) {
    if (position_features.empty()) {
        return;
    }
        
    for (auto i = position_features.begin(); i != position_features.end(); i++) {
        set_colour_for_feature(i->colour);
        fillpoly(&(i->points[0]), i->npoints);    
    }
}

void draw_line_features_from_vector(std::vector<feature>& position_features) {
    if (position_features.empty()) {
        return;
    }

    for (auto i = position_features.begin(); i != position_features.end(); i++) {
        set_colour_for_feature(i->colour);
        for (int vertex = 0; vertex < i->npoints - 1; vertex++) {
            drawline(i->points[vertex].x, i->points[vertex].y, i->points[vertex + 1].x, i->points[vertex + 1].y);
        }
    }
}

void draw_poi() {
    int thread_id = 0;
    
    if (LOD_min_dim_test(0.01)) {
        poi_dynamic_data* data_to_draw = draw_data->get_poi_data();

        //take out a copy for drawing
        int copy_to_use = check_out_copy(data_to_draw, thread_id);

        if (copy_to_use == 1) {
            for (auto i = data_to_draw->primary_copy.begin(); i != data_to_draw->primary_copy.end(); i++) {
                draw_poi_icon_and_name(*i);  
            }
            //release lock
            data_to_draw->primary_copy_editing_id = unused;
        }        
        else {
            for (auto i = data_to_draw->secondary_copy.begin(); i != data_to_draw->secondary_copy.end(); i++) {
                draw_poi_icon_and_name(*i);  
            }
            //release lock
            data_to_draw->secondary_copy_editing_id = unused;
        }
    }
}
    
int check_out_copy(poi_dynamic_data* data_set, int thread_id) {
    int copy_to_use = 1;
    int successful_locks = 0;
    while (successful_locks < 2) {
        if (copy_to_use == 1) {
            if (data_set->primary_copy_editing_id == unused || data_set->primary_copy_editing_id == thread_id) {
                data_set->primary_copy_editing_id = thread_id;
                successful_locks++;
            }
            else {
                copy_to_use = 2;
                successful_locks = 0;
            }
        }
        else {
            if (data_set->secondary_copy_editing_id == unused || data_set->secondary_copy_editing_id == thread_id) {
                data_set->secondary_copy_editing_id = thread_id;
                successful_locks++;
            }
            else {
                copy_to_use = 1;
                successful_locks = 0;
            }
        }
    }
    return copy_to_use;
}

//draw icons depend on types, then if zoomed-in enough, draw names
void draw_poi_icon_and_name(poi current_poi) {
    //All icon images are from “Road Transportation map markers,” Maps Icons Collection. [Online]. 
    //Available: https://mapicons.mapsmarker.com/category/markers/transportation/road-transportation/ 
    
    if (zoom_count > 9) setfontsize(10);
    else if (zoom_count == 9) setfontsize(9);
    else if (zoom_count == 8) setfontsize(8);
    
    bool draw_text = false;
       
    if (zoom_count == 8) {
        if((current_poi.text_position.x - draw_data->get_previous_poi_x() > 0.00008) && (current_poi.text_position.y - draw_data->get_previous_poi_y() > 0.00008)) {
            draw_text = true;
        }
    }
    
    if (zoom_count == 9) {
        if((current_poi.text_position.x - draw_data->get_previous_poi_x() > 0.00005) && (current_poi.text_position.y - draw_data->get_previous_poi_y() > 0.00005)) {
            draw_text = true;
        }
    }
    
    if (zoom_count == 10) {
        if((current_poi.text_position.x - draw_data->get_previous_poi_x() > 0.00002) && (current_poi.text_position.y - draw_data->get_previous_poi_y() > 0.00003)) {
            draw_text = true;
        }
    }
    
    if (zoom_count > 10) {
        draw_text = true;
    }
    
    //restaurant
    if (draw_data->get_poi_basic() && (current_poi.type == "restaurant" || current_poi.type == "old_restaurant")) {
        draw_surface(load_png_from_file("libstreetmap/resources/restaurant.png"), current_poi.icon_position.x, current_poi.icon_position.y);
        if (draw_text) {
            setcolor(DARKGREY);
            drawtext(current_poi.text_position.x, current_poi.text_position.y, current_poi.name, 0.001, 0.001);    
        }   
    }    
    //bus
    else if (draw_data->get_poi_basic() && current_poi.type == "bus_station" ) {
        draw_surface(load_png_from_file("libstreetmap/resources/bus.png"), current_poi.icon_position.x, current_poi.icon_position.y);
        if (draw_text) {
            setcolor(DARKGREY);
            drawtext(current_poi.text_position.x, current_poi.text_position.y, current_poi.name, 0.001, 0.001);    
        }      
    }    
    //cafe
    else if (draw_data->get_poi_basic() && (current_poi.type == "cafe" || current_poi.type == "old_cafe" || current_poi.type == "internet_cafe")) {
        draw_surface(load_png_from_file("libstreetmap/resources/cafe.png"), current_poi.icon_position.x, current_poi.icon_position.y);
        if (draw_text) {
            setcolor(DARKGREY);
            drawtext(current_poi.text_position.x, current_poi.text_position.y, current_poi.name, 0.001, 0.001);    
        }    
    }
    //fast food
    else if (draw_data->get_poi_basic() && (current_poi.type == "fast_food" || current_poi.type == "old_fast_food")) {
        draw_surface(load_png_from_file("libstreetmap/resources/fastfood.png"), current_poi.icon_position.x, current_poi.icon_position.y);
        if (draw_text) {
            setcolor(DARKGREY);
            drawtext(current_poi.text_position.x, current_poi.text_position.y, current_poi.name, 0.001, 0.001);    
        }     
    }
    //bank, atm
    else if (draw_data->get_poi_basic() && (current_poi.type == "bank" || current_poi.type == "atm" || current_poi.type == "old_bank")) {
        draw_surface(load_png_from_file("libstreetmap/resources/bank.png"), current_poi.icon_position.x, current_poi.icon_position.y);
        if (draw_text) {
            setcolor(DARKGREY);
            drawtext(current_poi.text_position.x, current_poi.text_position.y, current_poi.name, 0.001, 0.001);    
        }      
    }    
    //parking
    else if (draw_data->get_poi_basic() && (current_poi.type == "bicycle parking" || current_poi.type == "parking")) {
        draw_surface(load_png_from_file("libstreetmap/resources/parking.png"), current_poi.icon_position.x, current_poi.icon_position.y);
        if (draw_text) {
            setcolor(DARKGREY);
            drawtext(current_poi.text_position.x, current_poi.text_position.y, current_poi.name, 0.001, 0.001);    
        }     
    }
    //extra 
    if (draw_data->get_poi_extra() && current_poi.type != "restaurant" && current_poi.type != "old_restaurant" && current_poi.type != "bus_station" 
                && current_poi.type != "cafe" && current_poi.type != "old_cafe" && current_poi.type != "internet_cafe" && current_poi.type != "fast_food" 
                && current_poi.type != "old_fast_food" && current_poi.type != "bank" && current_poi.type != "atm" && current_poi.type != "old_bank"
                && current_poi.type != "bicycle parking" && current_poi.type != "parking" ) {
        draw_surface(load_png_from_file("libstreetmap/resources/star.png"), current_poi.icon_position.x, current_poi.icon_position.y);
        if (draw_text) {
            setcolor(DARKGREY);
            drawtext(current_poi.text_position.x, current_poi.text_position.y, current_poi.name, 0.001, 0.001);    
        }        
    }
    draw_data->set_previous_poi_x(current_poi.text_position.x);
    draw_data->set_previous_poi_y(current_poi.text_position.y);     
}

//Draws all street segments from the primary / secondary copy of street_dynamic_data
void draw_street_segments() {
    const int thread_id = 0;
    
    //Minor -> major -> highways so highway is on top
    
    street_segment_dynamic_data* data_to_draw;
    //choose copy for drawing
    int copy_to_use;
    
    //Zoom level to show minor segments
    if (LOD_area_test(0.005)) {
        t_color minor_segments_colour(128, 128, 128);
        t_color minor_segments_highlight_colour(128, 44, 0);
        data_to_draw = draw_data->get_street_segment_data(7); //7 -> minor
        copy_to_use = check_out_copy(data_to_draw, thread_id);
        
        if (copy_to_use == 1) {
            draw_street_segments_from_set(data_to_draw->primary_copy, minor_segments_colour, minor_segments_highlight_colour);
            //release lock
            data_to_draw->primary_copy_editing_id = unused;
        }
        else {
            draw_street_segments_from_set(data_to_draw->secondary_copy, minor_segments_colour, minor_segments_highlight_colour);
            //release lock
            data_to_draw->secondary_copy_editing_id = unused;
        }
    }

    //Different levels of zoom, only draw when close enough
    if (LOD_area_test(0.5)) {
        t_color major_segments_colour(128, 128, 128);
        t_color major_segments_highlight_colour(128, 44, 0);
        data_to_draw = draw_data->get_street_segment_data(6); //6 -> major
        copy_to_use = check_out_copy(data_to_draw, thread_id);
        
        if (copy_to_use == 1) {
            draw_street_segments_from_set(data_to_draw->primary_copy, major_segments_colour, major_segments_highlight_colour);
            //release lock
            data_to_draw->primary_copy_editing_id = unused;
        }
        else {
            draw_street_segments_from_set(data_to_draw->secondary_copy, major_segments_colour, major_segments_highlight_colour);
            //release lock
            data_to_draw->secondary_copy_editing_id = unused;
        }
    }

    //load all highway street segments within selected array index
    t_color highway_segments_colour(128, 108, 102);
    t_color highway_segments_highlight_colour(128, 44, 0);
    data_to_draw = draw_data->get_street_segment_data(5); //5 -> highway
    copy_to_use = check_out_copy(data_to_draw, thread_id);

    if (copy_to_use == 1) {
        draw_street_segments_from_set(data_to_draw->primary_copy, highway_segments_colour, highway_segments_highlight_colour);
        //release lock
        data_to_draw->primary_copy_editing_id = unused;
    }
    else {
        draw_street_segments_from_set(data_to_draw->secondary_copy, highway_segments_colour, highway_segments_highlight_colour);
        //release lock
        data_to_draw->secondary_copy_editing_id = unused;
    }
}
    
int check_out_copy(street_segment_dynamic_data* data_set, int thread_id) {
    int copy_to_use = 1;
    int successful_locks = 0;
    while (successful_locks < 2) {
        if (copy_to_use == 1) {
            if (data_set->primary_copy_editing_id == unused || data_set->primary_copy_editing_id == thread_id) {
                data_set->primary_copy_editing_id = thread_id;
                successful_locks++;
            }
            else {
                copy_to_use = 2;
                successful_locks = 0;
            }
        }
        else {
            if (data_set->secondary_copy_editing_id == unused || data_set->secondary_copy_editing_id == thread_id) {
                data_set->secondary_copy_editing_id = thread_id;
                successful_locks++;
            }
            else {
                copy_to_use = 1;
                successful_locks = 0;
            }
        }
    }
    return copy_to_use;
}

void draw_street_segments_from_set(std::set<street_segment_data>& position_segments, t_color normal_colour, t_color highlight_colour) {
    if (position_segments.empty()) {
        return;
    }
    
    t_color colour;
    bool draw_circle = LOD_area_test(0.00009);
//    bool draw_circle = false;
    
    for (auto i = position_segments.begin(); i != position_segments.end(); i++) {
        
        colour = normal_colour;

        //If no curve points we just draw 
        if (i->info.curvePointCount == 0) {
            draw_line_between(getIntersectionPosition(i->info.from), getIntersectionPosition(i->info.to), colour, i->width, draw_circle);
        } else {
            //Draw from start intersection to first curve point
            draw_line_between(getIntersectionPosition(i->info.from), i->curve_points[0], colour, i->width, draw_circle);
            //Loops through all curve points, drawing from one to next
            for (unsigned j = 0; j < i->curve_points.size() - 1; j++) {
                draw_line_between(i->curve_points[j], i->curve_points[j+1], colour, i->width, draw_circle);
            }
            //Last curve point to end interesction
            draw_line_between(i->curve_points[i->curve_points.size() - 1], getIntersectionPosition(i->info.to), colour, i->width, draw_circle);
        }
    }
    
    for (auto i = position_segments.begin(); i != position_segments.end(); i++) {
        if (i->selected) { 
            
            colour = highlight_colour;

            //If no curve points we just draw 
            if (i->info.curvePointCount == 0) {
                draw_line_between(getIntersectionPosition(i->info.from), getIntersectionPosition(i->info.to), colour, i->width, draw_circle);
            } else {
                //Draw from start intersection to first curve point
                draw_line_between(getIntersectionPosition(i->info.from), i->curve_points[0], colour, i->width, draw_circle);
                //Loops through all curve points, drawing from one to next
                for (unsigned j = 0; j < i->curve_points.size() - 1; j++) {
                    draw_line_between(i->curve_points[j], i->curve_points[j+1], colour, i->width, draw_circle);
                }
                //Last curve point to end interesction
                draw_line_between(i->curve_points[i->curve_points.size() - 1], getIntersectionPosition(i->info.to), colour, i->width, draw_circle);
            }
        }
    }
}

void draw_line_between(LatLon point_1, LatLon point_2, t_color color, double width, bool draw_circle) {
    
    double zoom = (get_visible_world().get_width() > get_visible_world().get_height()) ? get_visible_world().get_width() : get_visible_world().get_height();
    double factor = 0.1/zoom;

    if (LOD_area_test(0.0075)) {
        factor = 0.05/zoom;
    }
    
    //Convert to cartesian
    coordinate point_1_coordinate = map_data->latLon_to_coordinates(point_1);    
    coordinate point_2_coordinate = map_data->latLon_to_coordinates(point_2);
    
    //Convert to t_point for drawline function
    t_point t_point_1 = t_point(point_1_coordinate.x, point_1_coordinate.y);
    t_point t_point_2 = t_point(point_2_coordinate.x, point_2_coordinate.y);
    
    setcolor(t_color(color.red,color.green,color.blue,color.alpha));
    
    if (draw_circle) {
        fillarc(t_point_1, (width*factor*zoom)/2925, 0, 360); //Width that scales with zoom
        fillarc(t_point_2, (width*factor*zoom)/2925, 0, 360);
    }
    
    setlinewidth(width*factor);
    setlinestyle(SOLID);
    
    drawline(t_point_1, t_point_2);
}

//Same deal as draw_street_segments
void draw_street_segment_names() {
    const int thread_id = 0;
    
    //Minor -> major -> highways so highway is on top
    
    if (LOD_area_test(0.05)) {
        street_segment_dynamic_data* data_to_draw;
        //choose copy for drawing
        int copy_to_use;

        if (LOD_area_test(0.0005)) {
            t_color minor_names_colour(0, 0, 0);
            data_to_draw = draw_data->get_street_segment_data(7); //7 -> minor
            copy_to_use = check_out_copy(data_to_draw, thread_id);

            if (copy_to_use == 1) {
                draw_street_segments_names_from_set(data_to_draw->primary_copy, minor_names_colour);
                //release lock
                data_to_draw->primary_copy_editing_id = unused;
            }
            else {
                draw_street_segments_names_from_set(data_to_draw->secondary_copy, minor_names_colour);
                //release lock
                data_to_draw->secondary_copy_editing_id = unused;
            }
        }

        //Different levels of zoom, only draw when close enough
        if (LOD_area_test(0.005)) {
            t_color major_names_colour(0, 0, 0);
            data_to_draw = draw_data->get_street_segment_data(6); //6 -> major
            copy_to_use = check_out_copy(data_to_draw, thread_id);

            if (copy_to_use == 1) {
                draw_street_segments_names_from_set(data_to_draw->primary_copy, major_names_colour);
                //release lock
                data_to_draw->primary_copy_editing_id = unused;
            }
            else {
                draw_street_segments_names_from_set(data_to_draw->secondary_copy, major_names_colour);
                //release lock
                data_to_draw->secondary_copy_editing_id = unused;
            }
        }

        //load all highway street segments within selected array index
        t_color highway_names_colour(0, 0, 0);
        data_to_draw = draw_data->get_street_segment_data(5); //5 -> highway
        copy_to_use = check_out_copy(data_to_draw, thread_id);

        if (copy_to_use == 1) {
            draw_street_segments_names_from_set(data_to_draw->primary_copy, highway_names_colour);
            //release lock
            data_to_draw->primary_copy_editing_id = unused;
        }
        else {
            draw_street_segments_names_from_set(data_to_draw->secondary_copy, highway_names_colour);
            //release lock
            data_to_draw->secondary_copy_editing_id = unused;
        }
    }
}

void draw_street_segments_names_from_set(std::set<street_segment_data>& position_segments, t_color colour) {
    if (position_segments.empty()) {
        return;
    }
    setcolor(colour);
    
    for (auto i = position_segments.begin(); i != position_segments.end(); i++) {
        if (!(i->draw_name)) { //if we set it to not draw from algo in processing then we just skip it
            continue;
        }
        draw_segment_name(*i);
    }
}

void draw_segment_name(street_segment_data street_segment_info) {
    
    std::string street_name = getStreetName(street_segment_info.info.streetID);
    //If street name is unknown we don't draw text
    if (street_name == "<unknown>") {
        return;
    }
    
    //Points where the text start and end
    LatLon text_start, text_end;
    
    double max_distance = 0.0;
    
    text_start = getIntersectionPosition(street_segment_info.info.from);
    text_end = getIntersectionPosition(street_segment_info.info.to);
        
    //Put at longest part of curved segment to place our text
    if (street_segment_info.info.curvePointCount != 0) {
        for (unsigned i = 0; i < street_segment_info.info.curvePointCount-1; i++) {
            coordinate point_1 = map_data->latLon_to_coordinates(street_segment_info.curve_points[i]);
            coordinate point_2 = map_data->latLon_to_coordinates(street_segment_info.curve_points[i+1]);
            double distance = distance_between_points(point_1, point_2);
            if (distance > max_distance) {
                max_distance = distance;
                text_start = street_segment_info.curve_points[i];
                text_end = street_segment_info.curve_points[i+1];
            }
        }
    }
    
    coordinate cart_text_start = map_data->latLon_to_coordinates(text_start);
    coordinate cart_text_end = map_data->latLon_to_coordinates(text_end);
    
    //Get text rotation angle
    double rotation_angle = compute_rotation_angle(cart_text_start, cart_text_end, street_name, street_segment_info.info.oneWay);

    //Sets the middle of text
    float avg_x = (cart_text_start.x + cart_text_end.x) / 2;
    float avg_y = (cart_text_start.y + cart_text_end.y) / 2;
    
    //Tolerance allows the name more room to show
    double font_size;
    double tolerance = 1.5;
    
    //Higher tolerance for major and highways
    if (street_segment_info.street_type == "highway" || street_segment_info.street_type == "major" ) {
        tolerance = 4.0;
    }
    
    //Scaling font
    double zoom = (get_visible_world().get_width() > get_visible_world().get_height()) ? get_visible_world().get_width() : get_visible_world().get_height();
    double factor = zoom/8;
    if (factor > 0.000005) font_size = 9;
    else font_size = (0.00004 / factor);
    
    if (LOD_area_test(0.000004)) {
        font_size *= 2;
    }
    if (LOD_area_test(0.000001)) {
        font_size *= 2;
    }
    
    settextattrs(font_size, rotation_angle);
    setcolor(BLACK);
    drawtext(avg_x, avg_y, street_name,  abs(cart_text_start.x - cart_text_end.x)*tolerance, abs(cart_text_start.y - cart_text_end.y)*tolerance);
    settextrotation(0);
    
}

double compute_rotation_angle(coordinate point_1, coordinate point_2, std::string &street_name, bool one_way) {
    
    std::string arrow_right = " >>> ";
    std::string arrow_left = " <<< ";
     
    double delta_x = point_2.x - point_1.x; 
    double delta_y = point_2.y - point_1.y;

    double angle = atan(delta_y/delta_x) / DEG_TO_RAD;
    if (one_way) {
        if (delta_x == 0) {
            street_name += arrow_right;
        } else if (delta_x > 0) { //First or fourth quadrant, -90 ~ 90, either way text not upside down
            street_name += arrow_right;
        } else { //Left arrow because we essentially flipped the entire text
            street_name = arrow_left + street_name; 
        }
    }
    return angle;
}

double distance_between_points(coordinate point_1, coordinate point_2) {
    return sqrt((point_1.x - point_2.x)*(point_1.x - point_2.x) + (point_1.y - point_2.y)*(point_1.y - point_2.y));
}
    
void draw_search_box_related() {    
    draw_data->search_box.draw();
    draw_data->clear_search.draw();
    draw_data->autocomplete_box.draw();
}

//deal with conditionals related to clicking on the map area
void act_on_mouse_button(float x, float y, t_event_buttonPressed button_info) {
    //on map section
    if (button_info.button == 1) {
        
        //check UI elements registered with draw_data for being clicked on
        UI_element* clicked_on_element = draw_data->find_clicked_on_element(x, y);
        
        //other UI things
        //for historic reasons, not all UI elements are registered with draw_data
        if (clicked_on_element == NULL) {
            
            disable_right_click_menu(draw_screen);
            
            if (button_info.shift_pressed) {
                draw_data->from_x = x;
                draw_data->from_y = y;
            }
            else if (button_info.ctrl_pressed) {
                draw_data->to_x = x;
                draw_data->to_y = y;
            }
            else {
        
                LatLon click_position = map_data->x_and_y_to_LatLon(x, y);

                unsigned closest_intersection_id = find_closest_intersection(click_position);
                if (find_distance_between_two_points(click_position, getIntersectionPosition(closest_intersection_id)) < 10.0) {
                    draw_data->set_selected_intersection(closest_intersection_id);
                    draw_data->unselect_poi();
                }
                else {
                    draw_data->unselect_intersection();
                }

                unsigned closest_poi_id = find_closest_point_of_interest(click_position);
                if (draw_data->get_draw_poi() && find_distance_between_two_points(click_position, getPointOfInterestPosition(closest_poi_id)) < 10.0) {
                    draw_data->set_selected_poi(closest_poi_id);
                    draw_data->unselect_intersection();
                }
                else {
                    draw_data->unselect_poi();
                }

                if (draw_data->search_box.clicked_on(x, y) && !draw_data->search_box.get_is_active()) { //Clicked on search box, activate it and autocomplete
                    draw_data->search_box.respond_to_click();
                    draw_data->autocomplete_box.respond_to_click();
                }
                if (!draw_data->search_box.clicked_on(x, y) && draw_data->search_box.get_is_active()) { //Didn't click on search box, but search box active, deactivate
                    draw_data->search_box.respond_to_click();
                    draw_data->autocomplete_box.respond_to_click();
                    draw_data->autocomplete_box.reset_currently_selected();
                }

                if (draw_data->clear_search.clicked_on(x, y)) { //Clear button, clears search and related
                    draw_data->selected_street_ids.clear();
                    draw_data->search_box.clear_current_search();
                    draw_data->autocomplete_box.clear_all_data();
                    draw_data->unselect_intersection();
                    draw_data->unselect_poi();
                    draw_data->force_reload_segments();
                    draw_data->find_poi = false;    
                }
            }
        }
        
        //UI element registered with draw_data
        else {
            clicked_on_element->respond_to_click();
        }
        
        //redraw to update any changes
        draw_screen();
    }
    else if (button_info.button == 3) {
        if (!help->get_exists()) {
            enable_right_click_menu(x, y, do_nothing);
            draw_data->right_click_x = x;
            draw_data->right_click_y = y;
        }
    }
    else if (button_info.button == 4) {
        if (do_not_zoom == true && !help->get_exists()) {
            if (text_offset > 0) {
                text_offset--;
            }
        }
    }
    else if (button_info.button == 5) {
        if (do_not_zoom == true && !help->get_exists()) {
            unsigned limit = std::max(0, static_cast<int>(pd2->get_number_of_directions() - pd2->get_number_to_display()));
            if (text_offset < limit) {
                text_offset ++;
            }
        }
    }
}
    
//button for triggering on poi display
void show_poi(void (*draw_screen_poi)(void)) {
    draw_screen_poi();
    if(draw_data->get_draw_poi()) {
        draw_data->set_draw_poi(false);
        update_message("Point of Interests display: OFF");       
        change_button_text("Hide POI", "Show POI");
        if(draw_data->get_poi_basic()) {
            destroy_button("Basics: ON");
        }
        else {
            destroy_button("Basics: OFF");
        }
        
        if(draw_data->get_poi_extra()) {
            destroy_button("Extras: ON");
        }
        else {
            destroy_button("Extras: OFF");
        }
        draw_data->set_poi_basic(false);
        draw_data->set_poi_extra(false);
        
    }
    else {
        draw_data->set_draw_poi(true);
        update_message("Point of Interests display: ON");
        change_button_text("Show POI", "Hide POI");      
        if(draw_data->get_poi_basic()) {
            if(draw_data->get_poi_extra()) {
                create_button("Hide POI", "Basics: ON", toggle_poi_basic);
                create_button("Basics: ON", "Extras: ON", poi_extra);
            }
            else {
                create_button("Hide POI", "Basics: ON", toggle_poi_basic);
                create_button("Basics: ON", "Extras: OFF", poi_extra);
            }
        }
        else {
            if(draw_data->get_poi_extra()) {
                create_button("Hide POI", "Basics: OFF", toggle_poi_basic);
                create_button("Basics: OFF", "Extras: ON", poi_extra);
            }
            else {
                create_button("Hide POI", "Basics: OFF", toggle_poi_basic);
                create_button("Basics: OFF", "Extras: OFF", poi_extra);
            }  
        }
    }
}

//sub function for Show POI button
void toggle_poi_basic(void (*draw_screen_poi)(void)) {
    draw_screen_poi();
    if(draw_data->get_poi_basic()) {
        draw_data->set_poi_basic(false);
        update_message("Basic Point of Interests display: OFF");  
        change_button_text("Basics: ON", "Basics: OFF");             
    }
    else {
        draw_data->set_poi_basic(true);
        update_message("Basic Point of Interests display: ON");
        change_button_text("Basics: OFF", "Basics: ON"); 
    }
    
}

//sub function for Show POI button
void poi_extra(void (*draw_screen_poi)(void)) {
    draw_screen_poi();
    if(draw_data->get_poi_extra()) {
        draw_data->set_poi_extra(false);
        update_message("Extra Point of Interests display: OFF"); 
        change_button_text("Extras: ON", "Extras: OFF"); 
    }
    else {
        draw_data->set_poi_extra(true);
        update_message("Extra Point of Interests display: ON");
        change_button_text("Extras: OFF", "Extras: ON"); 
    }
}

//display selected poi information 
void show_poi_info(unsigned POI_index) {
    std::string POI_type = getPointOfInterestType(POI_index);
    std::replace(POI_type.begin(), POI_type.end(), '_', ' ');
    std::string POI_name = getPointOfInterestName(POI_index);
    std::string lat =  std::to_string(getPointOfInterestPosition(POI_index).lat());
    std::string lon =  std::to_string(getPointOfInterestPosition(POI_index).lon());
    update_message("POI: " + POI_name + "  Type: " + POI_type + "   Lat: " + lat + "   Lon: " + lon); 
}
  
//display selected intersection information
void show_intersection_info(unsigned intersection_index) {
    std::string name = getIntersectionName(intersection_index);
    std::string lat =  std::to_string(getIntersectionPosition(intersection_index).lat());
    std::string lon =  std::to_string(getIntersectionPosition(intersection_index).lon());
    update_message("Intersection: " + name + "   Lat: " + lat + "   Lon: " + lon);   
}

//function for Reset button
void reset_button(void (*draw_screen_)(void)) {
    //change map button
    clickproceed->disable_existence();
    change_map_on = false;
    change_button_text("YES!" , "Exit");
    change_button_text("NOOO!", "Reset");   
    
    //selected stuff
    draw_data->unselect_intersection();
    draw_data->unselect_poi();
    
    //general interface
    update_message(" ");
    zoom_fit_();
    
    //variables
    zoom_count = 0;
    draw_data->from_x = FLT_MAX;
    draw_data->from_y = FLT_MAX;
    draw_data->to_x = FLT_MAX;
    draw_data->to_y = FLT_MAX;
    draw_data->old_from_x = FLT_MAX;
    draw_data->old_from_y = FLT_MAX;
    draw_data->old_to_x = FLT_MAX;
    draw_data->old_to_y = FLT_MAX;
    draw_data->find_poi_x = FLT_MAX;
    draw_data->find_poi_y = FLT_MAX;
    draw_data->find_poi = false;
    draw_data->show_found_path = false;
    text_offset = 0;
    do_not_zoom = false;
    draw_data->show_found_path = false;
    
    //customized UI elements
    pd1->disable_existence();
    pd2->disable_existence();
    left_arrow->enable_existence();
    pathdisplay_up->disable_existence();
    pathdisplay_down->disable_existence();
    r_click_menu1->disable_existence();
    r_click_menu2->disable_existence();
    r_click_menu3->disable_existence();
    help->disable_existence();
    loading->disable_existence();
    
    draw_screen_();
}

void act_on_key_press(char c, int keysym) {
#ifdef X11 // Extended keyboard codes only supported for X11 for now
    
    if (draw_data->search_box.get_is_active()) {
        //Back space key, we remove character from search, if nothing left we don't even run autocomplete parser
        if (keysym == XK_BackSpace) { //Backspace 65288
            draw_data->search_box.delete_character_current_search(1);
            if (draw_data->search_box.get_current_search() == "") {
                draw_data->autocomplete_box.clear_all_data();
            } else {
                search_parser();
            }
        //Enter key, triggers the search
        } else if (keysym == XK_Return) { //Enter 65293
            
            search_return_data selected_result;
            
            if (draw_data->find_poi) {
                
                if(draw_data->autocomplete_box.get_selected_data(selected_result)) {
                
                    draw_data->unselect_poi();
                    draw_data->search_box.set_current_search(selected_result.object_name);
                
                    draw_data->set_selected_poi(selected_result.object_id[0]);
                    
                    LatLon from_position = map_data->x_and_y_to_LatLon(draw_data->find_poi_x, draw_data->find_poi_y);
                    unsigned from_id = map_data->get_closest_intersection(from_position);

                    LatLon to_position = map_data->x_and_y_to_LatLon(draw_data->to_x, draw_data->to_y);
                    unsigned to_id = map_data->get_closest_intersection(to_position);
                    
                    //Path to closest POI
                    std::vector<unsigned> path_segments_vector = find_path_to_point_of_interest(from_id, selected_result.object_name, 0.0);
                    
                    unsigned poi_closest_intersection_id = getStreetSegmentInfo(path_segments_vector[path_segments_vector.size() - 1]).to;
                    coordinate end_poi_coords = map_data->latLon_to_coordinates(getIntersectionPosition(poi_closest_intersection_id));
                    
                    //Updating stuff
                    draw_data->to_x = end_poi_coords.x;
                    draw_data->to_y = end_poi_coords.y;
                    
                    
                    std::vector<unsigned> path_intersections_vector = segments_to_intersections(path_segments_vector);
                    
                    update_found_path_intersection(path_segments_vector);
                    
                    format_path_data_1(from_id, to_id);
                    
                    format_path_data_2_intersection(path_intersections_vector);
                    
                    draw_data->search_box.respond_to_click();
                    draw_data->autocomplete_box.respond_to_click();
                    draw_data->autocomplete_box.reset_currently_selected();
                    
                    draw_data->show_found_path = true;
                    
                    draw_data->old_from_x = draw_data->from_x;
                    draw_data->old_from_y = draw_data->from_y;
                    draw_data->old_to_x = draw_data->to_x;
                    draw_data->old_to_y = draw_data->to_y;
                    
                    pd1->enable_existence();
                    pd2->enable_existence();
                    left_arrow->disable_existence();
                    pathdisplay_up->enable_existence();
                    pathdisplay_down->enable_existence();
                    pathdisplay_up->draw();
                    draw_screen();
                    
                } else {
                    
                }
            }
            
            draw_data->selected_street_ids.clear();
            draw_data->unselect_intersection();
            draw_data->unselect_poi();
            
            //If user is using autocomplete listings
            if(draw_data->autocomplete_box.get_selected_data(selected_result)) {
                if (selected_result.object_type == "street") {    
                    
                    draw_data->search_box.set_current_search(selected_result.object_name);
                    
                    double total_distance = 0.0;
                    double avg_x = 0.0;
                    double avg_y = 0.0;
                    int counter = 0;
                    
                    for (unsigned i = 0; i < selected_result.object_id.size(); i++) {
                        draw_data->selected_street_ids[selected_result.object_id[i]] = true;
                        total_distance += find_street_length(selected_result.object_id[i]);
                        std::vector<unsigned> segment_ids = map_data->get_street_segment_ids_from_street_id(selected_result.object_id[i]);
                        for (unsigned j = 0; j < segment_ids.size(); j++) {
                            LatLon to_position = getIntersectionPosition(getStreetSegmentInfo(segment_ids[j]).to);
                            avg_x += to_position.lon();
                            avg_y += to_position.lat();
                            counter++;
                        }
                    }
                    
                    draw_data->force_reload_segments();
                    
                    avg_x /= counter;
                    avg_y /= counter;
                    coordinate center = map_data->latLon_to_coordinates(LatLon(avg_y, avg_x));
                    center_and_zoom(center.x, center.y, draw_screen, total_distance/2000000);                  
                                        
                } else if (selected_result.object_type == "intersection") {
                    
                    draw_data->search_box.set_current_search(selected_result.object_name);
                                        
                    draw_data->set_selected_intersection(selected_result.object_id[0]);
                    coordinate center = map_data->latLon_to_coordinates(getIntersectionPosition(selected_result.object_id[0]));
                    center_and_zoom(center.x, center.y, draw_screen);
                    
                } else {
                    
                    draw_data->search_box.set_current_search(selected_result.object_name);
                                        
                    draw_data->set_selected_poi(selected_result.object_id[0]);
                    coordinate center = map_data->latLon_to_coordinates(getPointOfInterestPosition(selected_result.object_id[0]));
                    center_and_zoom(center.x, center.y, draw_screen);
                }
            } else { //If not we just search whatever he typed assuming it's error free and case sensitive
                
                std::vector<unsigned> searched_street_ids = find_street_ids_from_name(draw_data->search_box.get_current_search());
                if (searched_street_ids.size() != 0) { //Found a street
                    draw_data->selected_street_ids.clear();
                    while (searched_street_ids.size() != 0) {
                        draw_data->selected_street_ids[searched_street_ids.back()] = true;
                        searched_street_ids.pop_back();
                    }
                    
                    draw_data->force_reload_segments();
                    
                } else { //Can't find street now find intersection, currently search only works for exact names
                    std::string street_name_1, street_name_2;
                    bool good_search = false;
                    if (intersection_street_name_split(draw_data->search_box.get_current_search(), street_name_1, street_name_2, "&")) {
                        good_search = true;
                    } else if (intersection_street_name_split(draw_data->search_box.get_current_search(), street_name_1, street_name_2, "and")) {
                        good_search = true;
                    }
                    if (good_search) {
                        std::vector<unsigned> selected_intersection_ids = find_intersection_ids_from_street_names(street_name_1, street_name_2);
                        if (selected_intersection_ids.size() != 0) {
                            for (unsigned i = 0; i < selected_intersection_ids.size(); i++) {
                                draw_data->set_selected_intersection(selected_intersection_ids[i]);
                            }
                            coordinate center = map_data->latLon_to_coordinates(getIntersectionPosition(selected_intersection_ids[0]));
                            center_and_zoom(center.x, center.y, draw_screen);
                        }
                    }
                }
            }
        //Only append characters that are proper letters to the search string
        } else if (keysym >= 0x0020 && keysym <= 0x007e) {
            draw_data->search_box.append_to_search(c);
            search_parser();
        //Arrow keys adjusts highlight in autocomplete
        } else if (keysym == XK_Down) {
            draw_data->autocomplete_box.down_arrow_increment_selected();
        } else if (keysym == XK_Up) {
            draw_data->autocomplete_box.up_arrow_decrement_selected();
        }
        
        draw_screen();
    }
#endif
}
    
void search_parser() {
    std::vector<std::string> keywords = map_data->extract_keywords(draw_data->search_box.get_current_search());
    if (keywords.size() == 0) {
        return;
    }
    //Using first keywords to find possible list of names
    //Then use subsequent keywords to filter
    //Names without subsequent keywords are removed
    std::vector<search_return_data> results = map_data->get_from_soundex_library(map_data->convert_to_soundex(keywords[0]));
    if (keywords.size() > 1) {
        for (unsigned i = 1; i < keywords.size(); i++) {
//            std::vector<std::string> keywords_from_soundex_library = map_data->get_from_soundex_keyword_library(map_data->convert_to_soundex(keywords[i]));
//            for (unsigned j = 0; j < keywords_from_soundex_library.size(); j++) {
                for (auto k = results.begin(); k != results.end();) {
//                    std::size_t it = k->object_name.find(keywords_from_soundex_library[j]);
                    std::size_t it = k->object_name.find(keywords[i]);
                    if (it == std::string::npos) {
                        results.erase(k);
                    } else {
                        k++;
                    }
                }
//            }
        }
    }
    //Split into streets, intersections and POIs
    std::vector<search_return_data> street_results;
    std::vector<search_return_data> intersection_results;
    std::vector<search_return_data> poi_results;
    
    split_results(results, street_results, intersection_results, poi_results);
    
    //Perform sorting
    //First sort is for alphabetical + length
    //Second is only length
    //Lambda expressions C++11
    //Streets
    std::sort(street_results.begin(), street_results.end(), [](const search_return_data &lhs, const search_return_data &rhs) {
        return (lhs.object_name.compare(rhs.object_name) < 0);
    });
    std::sort(street_results.begin(), street_results.end(), [](const search_return_data &lhs, const search_return_data &rhs) {
        return (lhs.object_name.length() < rhs.object_name.length());
    });
    
    //Intersections
    std::sort(intersection_results.begin(), intersection_results.end(), [](const search_return_data &lhs, const search_return_data &rhs) {
        return (lhs.object_name.compare(rhs.object_name) < 0);
    });
    std::sort(intersection_results.begin(), intersection_results.end(), [](const search_return_data &lhs, const search_return_data &rhs) {
        return (lhs.object_name.length() < rhs.object_name.length());
    });
    
    //POIs
    std::sort(poi_results.begin(), poi_results.end(), [](const search_return_data &lhs, const search_return_data &rhs) {
        return (lhs.object_name.compare(rhs.object_name) < 0);
    });
    std::sort(poi_results.begin(), poi_results.end(), [](const search_return_data &lhs, const search_return_data &rhs) {
        return (lhs.object_name.length() < rhs.object_name.length());
    });
    
    //Put into autocomplete boxes
    draw_data->autocomplete_box.clear_all_data();
    
    if (draw_data->find_poi) {
        sort(poi_results.begin(), poi_results.end());
        poi_results.erase(unique(poi_results.begin(), poi_results.end()), poi_results.end());
        draw_data->autocomplete_box.set_poi_data(poi_results);
    } else {
        draw_data->autocomplete_box.set_street_data(street_results);
        draw_data->autocomplete_box.set_intersection_data(intersection_results);
        draw_data->autocomplete_box.set_poi_data(poi_results);
    }
    
    draw_data->autocomplete_box.update_number_being_shown();
    
}

//All this does is separate results into its different types for autocomplete to show
void split_results(std::vector<search_return_data> &results, std::vector<search_return_data> &street_results, 
        std::vector<search_return_data> &intersection_results, std::vector<search_return_data> &poi_results) {
    for (unsigned i = 0; i < results.size(); i++) {
        search_return_data current_data = results[i];
        if (current_data.object_type == "street") {
            street_results.push_back(current_data);
        } else if (current_data.object_type == "intersection") {
            intersection_results.push_back(current_data);
        } else if (current_data.object_type == "poi") {
            poi_results.push_back(current_data);
        }
    }
}

bool intersection_street_name_split(std::string input, std::string &street_name_1, std::string &street_name_2, std::string delimiter) {
    auto it = input.find(delimiter);
    if (it == std::string::npos) {
        return false;
    }
    street_name_1 = input.substr(0, it);
    input.erase(0, it + delimiter.length());
    street_name_2 = input;
    
    //Remove start and trailing spaces 
    street_name_1 = std::regex_replace(street_name_1, std::regex("^ +"), "");
    street_name_1 = std::regex_replace(street_name_1, std::regex(" +$"), "");
    street_name_2 = std::regex_replace(street_name_2, std::regex("^ +"), "");
    street_name_2 = std::regex_replace(street_name_2, std::regex(" +$"), "");
    
    return true;
}

int direction_of_travel(unsigned from_intersection, unsigned middle_intersection,unsigned to_intersection) {
    
    unsigned first_segment = find_segment_between_intersections(from_intersection, middle_intersection);
    unsigned second_segment = find_segment_between_intersections(middle_intersection, to_intersection);
    StreetSegmentInfo first_segment_info = getStreetSegmentInfo(first_segment);
    StreetSegmentInfo second_segment_info = getStreetSegmentInfo(second_segment);
    
    //Same road (and therefore straight) if same name
    if (getStreetName(first_segment_info.streetID) == getStreetName(second_segment_info.streetID)) {
        return 0;
    }
    
    coordinate from_coord = map_data->latLon_to_coordinates(getIntersectionPosition(from_intersection));
    coordinate middle_coord = map_data->latLon_to_coordinates(getIntersectionPosition(middle_intersection));
    coordinate to_coord = map_data->latLon_to_coordinates(getIntersectionPosition(to_intersection));
    
    double cur_segment_angle = atan2(to_coord.y - from_coord.y, to_coord.x - from_coord.x);
    double next_segment_angle = atan2(middle_coord.y - from_coord.y, middle_coord.x - from_coord.x);
    
    if (abs(cur_segment_angle - next_segment_angle) < (PI2/36)) {
        return 0; //Straight
    }
    
    if (cur_segment_angle < 0 && next_segment_angle > 0 ) {
        cur_segment_angle += PI2;
    }
    
    if (cur_segment_angle > next_segment_angle && cur_segment_angle < (next_segment_angle + PI2 / 2)) {
        return 2; //Right
    }
    return 1; //Left
}

void draw_route(std::vector<unsigned> path) {
    
    if (path.size() <= 1) {
        return;
    }
    
    std::set<street_segment_data> route_segments;
    
    for (unsigned i = 0; i < path.size() - 1; i++) {
        std::vector<unsigned> connected_segments = find_intersection_street_segments(path[i]);
        for (unsigned j = 0; j < connected_segments.size(); j++) {
            StreetSegmentInfo segment_info = getStreetSegmentInfo(connected_segments[j]);
            if (segment_info.from == path[i+1] || segment_info.to == path[i+1]) {
                std::vector<LatLon> curve_points;
                for (unsigned k = 0; k < segment_info.curvePointCount; k++) {
                    LatLon curve_point_latlon = getStreetSegmentCurvePoint(connected_segments[j], k);
                    curve_points.push_back(curve_point_latlon);
                }
                street_segment_data cur_street_segment;
                cur_street_segment = {
                    connected_segments[j],
                    "",
                    segment_info,
                    curve_points,
                    3,
                    false,
                    true
                };
                route_segments.insert(cur_street_segment);
            }
        }
    }
    
    draw_street_segments_from_set(route_segments, t_color(255, 255, 255), t_color(255, 89, 0));
}

void draw_route_segments(std::vector<unsigned> path_segments) {
    
    if (path_segments.size() <= 0) {
        return;
    }
    
    std::set<street_segment_data> route_segments;
    
    for (unsigned i = 0; i < path_segments.size(); i++) {
        
        StreetSegmentInfo segment_info = getStreetSegmentInfo(path_segments[i]);
        std::vector<LatLon> curve_points;
        for (unsigned k = 0; k < segment_info.curvePointCount; k++) {
            LatLon curve_point_latlon = getStreetSegmentCurvePoint(path_segments[i], k);
            curve_points.push_back(curve_point_latlon);
        }
        street_segment_data cur_street_segment;
        cur_street_segment = {
            path_segments[i],
            "",
            segment_info,
            curve_points,
            3,
            false,
            true
        };
        route_segments.insert(cur_street_segment);
    }
    
    draw_street_segments_from_set(route_segments, t_color(255, 255, 255), t_color(255, 89, 0));
}

std::vector<directions> format_directions(std::vector<unsigned> path) {
    std::vector<directions> results;
    
    //If there are no nodes or only 1 we return empty since we can't even get a street segment
    if (path.size() <= 1) {
        return results;
    }
    
    //Else we initialize the first segment
    unsigned segment_inbetween = find_segment_between_intersections(path[0], path[1]);
    std::string street_name = getStreetName(getStreetSegmentInfo(segment_inbetween).streetID);
    double distance = find_street_segment_length(segment_inbetween);
    double time = find_street_segment_travel_time(segment_inbetween);
    directions first_direction{
        0,
        street_name,
        distance,
        time
    };
    results.push_back(first_direction);
        
    //Now we have to look ahead 2 more nodes
    //First we check for direction
    //If direction is the straight then we just append to the current directions struct
    //If direction isn't straight, then we make a new directions struct and append to the back of the results vector
    
    for (unsigned i = 0; i < path.size() - 2; i++) {
        int next_direction = direction_of_travel(path[i], path[i+1], path[i+2]);
        segment_inbetween = find_segment_between_intersections(path[i+1], path[i+2]);
        street_name = getStreetName(getStreetSegmentInfo(segment_inbetween).streetID);
        distance = find_street_segment_length(segment_inbetween);
        time = find_street_segment_travel_time(segment_inbetween);
        if (next_direction == 0) { //If we continue the same direction as the current street segment            
            results[results.size()-1].distance += distance;
            results[results.size()-1].time += time;
        } else {
            directions following_direction{
                next_direction,
                street_name,
                distance,
                time
            };
            results.push_back(following_direction);
        }
    }
    
    //At the end we should have a vector of directions struct
    //This vector contains all the routing information
    //where same streets are all grouped together, with their combined time and distance added up
    //Now we format the directions with "Turn right on to" etc...
    
    for (unsigned i = 0; i < results.size(); i++) {
        int direction_of_cur = results[i].direction_int;
        std::string straight = "Go straight on ";
        std::string right = "Turn right onto ";
        std::string left = "Turn left onto ";
        if (direction_of_cur == 0) {
            results[i].direction_string = straight + results[i].direction_string;
        } else if (direction_of_cur == 1) {
            results[i].direction_string = right + results[i].direction_string;
        } else {
            results[i].direction_string = left + results[i].direction_string;
        }
    }
    
    return results;
}

unsigned find_segment_between_intersections(unsigned from, unsigned to) {
    std::vector<unsigned> connected_segments = find_intersection_street_segments(from);
    for (unsigned j = 0; j < connected_segments.size(); j++) {
        StreetSegmentInfo segment_info = getStreetSegmentInfo(connected_segments[j]);
        if (segment_info.from == to || segment_info.to == to) {
            return connected_segments[j];
        }
    }
    return 0;
}

void enable_find_path_buttons(void (*draw_screen_map)(void)) {
    
    //Check if activated already or not so we know to clear or not
    //If we already have the menus open, we want to close them and clear searches
    if (draw_data->old_from_x == draw_data->from_x && draw_data->old_from_y == draw_data->from_y && draw_data->old_to_x == draw_data->to_x && draw_data->old_to_y == draw_data->to_y) {

        //Reset back to impossible values so we know route isn't ready
        draw_data->from_x = FLT_MAX;
        draw_data->from_y = FLT_MAX;
        draw_data->to_x = FLT_MAX;
        draw_data->to_y = FLT_MAX;
        
        //Clear search box related in the event it was a poi routing
        if (draw_data->find_poi) {
            draw_data->selected_street_ids.clear();
            draw_data->search_box.clear_current_search();
            draw_data->autocomplete_box.clear_all_data();
        }
        
        //Reset booleans that facilitate drawing of routing related stuff
        draw_data->show_found_path = false;
        draw_data->find_poi = false;
        
        //Removing side menu
        pd1->disable_existence();
        pd2->disable_existence();
        left_arrow->enable_existence();
        pathdisplay_up->disable_existence();
        pathdisplay_down->disable_existence();
        do_not_zoom = false;
        text_offset = 0;
        draw_screen_map();

        return;
    }   
    
    //Start and end not set
    if (draw_data->from_x == FLT_MAX && draw_data->from_y == FLT_MAX && draw_data->to_x == FLT_MAX && draw_data->to_y == FLT_MAX) {
         update_message("Start and Destination not set");
        return;
    } else if (draw_data->from_x == FLT_MAX || draw_data->from_y == FLT_MAX) {
        update_message("Start not set");
        return;
    } else if (draw_data->to_x == FLT_MAX || draw_data->to_y == FLT_MAX) {
        update_message("Destination not set");
        return;
    } 
    
    draw_data->show_found_path = true;
    
    draw_data->old_from_x = draw_data->from_x;
    draw_data->old_from_y = draw_data->from_y;
    draw_data->old_to_x = draw_data->to_x;
    draw_data->old_to_y = draw_data->to_y;
    
    //Check if both start and end are defined
    //If so we draw the route and also update the directions
    
    LatLon from_position = map_data->x_and_y_to_LatLon(draw_data->from_x, draw_data->from_y);
    unsigned from_id = map_data->get_closest_intersection(from_position);
    
    LatLon to_position = map_data->x_and_y_to_LatLon(draw_data->to_x, draw_data->to_y);
    unsigned to_id = map_data->get_closest_intersection(to_position);
    
    Pathfinder path = Pathfinder(from_id, to_id, 0.0);
    
    std::list<unsigned> result = path.get_results();
    std::vector<unsigned> path_intersection_vector;
    while (result.size() != 0) {
        path_intersection_vector.push_back(result.front());
        result.pop_front();
    }
    
    result.clear();
    result = path.get_segment_results();
    std::vector<unsigned> path_segment_vector;
    while (result.size() != 0) {
        path_segment_vector.push_back(result.front());
        result.pop_front();
    }

    update_found_path_intersection(path_segment_vector);
    
    format_path_data_1(from_id, to_id);
    
    format_path_data_2_intersection(path_intersection_vector);
    
    pd1->enable_existence();
    pd2->enable_existence();
    left_arrow->disable_existence();
    pathdisplay_up->enable_existence();
    pathdisplay_down->enable_existence();
    pathdisplay_up->draw();
    draw_screen_map();
}

void update_found_path_intersection(std::vector<unsigned> path_intersection_id){
    draw_data->found_path.clear();
    draw_data->found_path = path_intersection_id;
}

void format_path_data_1(unsigned from_id, unsigned to_id) {    
    
    std::string from_name = getIntersectionName(from_id);
    std::string to_name = getIntersectionName(to_id);
    
    pd1->set_from(from_name);
    pd1->set_to(to_name);
}

void format_path_data_2_intersection(std::vector<unsigned> path_intersection_id) {
    
    pd2->clear_directions();
    
    std::vector<directions> formatted_direction_text = format_directions(path_intersection_id);
    
    for(unsigned i = 1; i <= formatted_direction_text.size(); i++) {
        std::string text_line_1;
        std::string text_line_2;
        
        std::stringstream text_to_display(formatted_direction_text[i-1].direction_string);
        std::string temp;
        int char_count = 0;
        int char_per_line = 28;
        
        while (!text_to_display.eof()) {
            text_to_display >> temp;
            temp = temp + " ";
            char_count += temp.length();
            if (char_count > (char_per_line * 2)) {
                break;
            }
            if (char_count <= char_per_line) {
                text_line_1 = text_line_1 + temp;
            } else if (char_count > char_per_line && char_count <= (char_per_line * 2)) {
                text_line_2 = text_line_2 + temp;
            }
        }
        pd2->update_directions(i, text_line_1, text_line_2, formatted_direction_text[i-1].direction_int, formatted_direction_text[i-1].distance, formatted_direction_text[i-1].time);
    }
}

std::vector<unsigned> segments_to_intersections(std::vector<unsigned> path_segments) {
    
    std::vector<unsigned> path_intersections;
    
    if (path_segments.size() == 0) {
        return path_intersections;
    }
    
    path_intersections.push_back(getStreetSegmentInfo(path_segments[0]).from);
    
    for (unsigned i = 0; i < path_segments.size(); i++) {
        path_intersections.push_back(getStreetSegmentInfo(path_segments[i]).to);
    }
    
    return path_intersections;
}