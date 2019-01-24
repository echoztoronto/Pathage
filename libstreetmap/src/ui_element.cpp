/*
 * 
 */

#include "ui_element.h"
#include "graphics.h"
#include <assert.h>
#include "draw_data.h"
#include <algorithm>

extern Draw_data *draw_data;
extern RightClickMenu1* r_click_menu1;
extern RightClickMenu2* r_click_menu2;
extern RightClickMenu3* r_click_menu3;
extern PathDisplay1* pd1;
extern PathDisplay2* pd2;
extern Button* pathdisplay_up;
extern Button* pathdisplay_down;
extern bool do_not_zoom;
extern unsigned text_offset;

/****************************************************************************************
                                  UI_element class
 ****************************************************************************************/

UI_element::UI_element() {
    //sit back and relax
}

UI_element::UI_element(float _top, float _bottom, float _left, float _right) {
    assert("Invalid button bounds" && _top < _bottom && _left < _right);
    top = _top;
    bottom = _bottom;
    left = _left;
    right = _right;
}

UI_element::~UI_element() {
    //sit back and relax
}

void UI_element::draw() {
    //don't draw unless exists
    if (exists) {
        //switch to UI drawing mode
        set_coordinate_system(GL_SCREEN);

        t_bound_box screen_coordinates = get_visible_screen();
        if (border_width != 0) {
            setcolor(border_colour);
            fillrect(screen_coordinates.right()*left, 
                    screen_coordinates.bottom()*top, 
                    screen_coordinates.right()*right, 
                    screen_coordinates.bottom()*bottom);
        }
        if (active) {
            setcolor(active_colour);
        }
        else {
            setcolor(inactive_colour);
        }
        fillrect(screen_coordinates.right()*left + border_width, 
                screen_coordinates.bottom()*top + border_width, 
                screen_coordinates.right()*right - border_width, 
                screen_coordinates.bottom()*bottom - border_width);

        //put back to avoid wrecking everything
        set_coordinate_system(GL_WORLD);
    }
}

void UI_element::begin_drawing() {
    set_coordinate_system(GL_SCREEN);
}

void UI_element::end_drawing() {
    set_coordinate_system(GL_WORLD);
}

void UI_element::toggle_existence() {
    exists = !exists;
}

void UI_element::enable_existence() {
    exists = true;
}

void UI_element::disable_existence() {
    exists = false;
}

bool UI_element::get_exists() {
    return exists;
}

void UI_element::toggle_activate() {
    active = !active;
}

void UI_element::activate() {
    active = true;
}

void UI_element::inactivate() {
    active = false;
}

bool UI_element::get_is_active() {
    return active;
}

t_bound_box UI_element::get_element_bounds() {
    return t_bound_box(left, bottom, right, top);
}

void UI_element::set_edges(float _top, float _bottom, float _left, float _right) {
    assert("Invalid button bounds" && _top < _bottom && _left < _right);
    top = _top;
    bottom = _bottom;
    left = _left;
    right = _right;
}

void UI_element::set_border_width(int _border_width) {
    border_width = _border_width;
}

int UI_element::get_border_width() const {
    return border_width;
}

void UI_element::set_active_colour(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a) {
    active_colour = t_color(r, g, b, a);
}
void UI_element::set_inactive_colour(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a) {
    inactive_colour = t_color(r, g, b, a);
}
void UI_element::set_border_colour(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a) {
    border_colour = t_color(r, g, b, a);
}

void UI_element::set_transparency(float transparency) {
    assert("Transparency should be between 0 and 100" && transparency >= 0 && transparency <= 100);
    
    active_colour.alpha = int(255 * transparency/100);
    inactive_colour.alpha = int(255 * transparency/100);
    border_colour.alpha = int(255 * transparency/100);
}

bool UI_element::clicked_on(float x, float y) {
    //ignore if doesn't exist
    if (!exists) {
        return false;
    }
    //convert world x and y to screen x and y
    t_point click_xy(x, y);
    t_point click_screen = world_to_scrn(click_xy);
    
    t_bound_box screen_boundaries = get_visible_screen();
    
    //compare click coordinates to button edges
    if (click_screen.y < top * screen_boundaries.bottom() ||
            click_screen.y > bottom * screen_boundaries.bottom() ||
            click_screen.x < left * screen_boundaries.right() ||
            click_screen.x > right * screen_boundaries.right() ) {
        return false;
    }
    else {
        return true;
    }
}

void UI_element::set_action(void (*active_function)(void), void (*inactive_function) (void)) {
    active_action = active_function;
    inactive_action = inactive_function;
}

void UI_element::respond_to_click() {
    if (get_is_active() && active_action != NULL) {
        active_action();
    }
    else if (inactive_action != NULL) {
        inactive_action();
    }
}

/****************************************************************************************
                                Button class
 ****************************************************************************************/

Button::Button() : UI_element() {
    //still nothing
}

Button::Button(float _top, float _bottom, float _left, float _right) 
    : UI_element(_top, _bottom, _left, _right) {
    //nothing to see here
}

Button::~Button() {
    
}

void Button::set_icon(const char* _icon_location, int _icon_width, int _icon_height) {
    has_icon = true;
    icon = load_png_from_file(_icon_location);
    icon_height = _icon_height;
    icon_width = _icon_width;
}

void Button::draw() {
    UI_element::draw();
    if (get_exists() && has_icon) {
        set_coordinate_system(GL_SCREEN);

        //get midpoint of the button
        t_bound_box button_edges = get_element_bounds();
        t_bound_box screen_edges = get_visible_screen();
        float x_midpoint = (button_edges.left()*screen_edges.right() + button_edges.right()*screen_edges.right())/2.0;
        float y_midpoint = (button_edges.top()*screen_edges.bottom() + button_edges.bottom()*screen_edges.bottom())/2.0;

        //draw the icon in the middle of the button
        draw_surface(icon, x_midpoint - icon_width/2, y_midpoint - icon_height/2);

        set_coordinate_system(GL_WORLD);
    }        
}

void Button::respond_to_click() {
    UI_element::respond_to_click();
}

/****************************************************************************************
                                SearchBox class
 ****************************************************************************************/

SearchBox::SearchBox() : UI_element() {
    //still nothing
}

SearchBox::SearchBox(float _top, float _bottom, float _left, float _right) 
    : UI_element(_top, _bottom, _left, _right) {
    //nothing to see here
}

SearchBox::~SearchBox() {
    
}

void SearchBox::draw() {
    UI_element::draw();
    
    std::string text_to_show;
    if (current_search.size() != 0) {
        text_to_show = current_search;
    } else if (!this->get_is_active()) {
        text_to_show = "Click to search";
    }
    
    set_coordinate_system(GL_SCREEN);
    
    t_bound_box screen_coordinates = get_visible_screen();
    t_bound_box own_bounds = this->get_element_bounds();
    double border = this->get_border_width();
    
    int left_shift = -10;
    int left_indent = 12;
    int right_indent = 12;
    int vertical_indent = 7;
    
    int box_vertical_indent = -2;
    
    if (this->get_is_active()) {
        setlinewidth(1);
        setcolor(0, 0, 0, 235);
        drawline(screen_coordinates.left()*own_bounds.left() + left_indent, 
                screen_coordinates.bottom()*own_bounds.bottom() - vertical_indent, 
                screen_coordinates.right()*own_bounds.right() - right_indent, 
                screen_coordinates.bottom()*own_bounds.bottom() - vertical_indent);
//        left_shift = 300;
        left_shift = 0;
//        left_shift -= current_search.length()*7; //WIP
        box_vertical_indent = 15;
    }
    
    t_bound_box search_bound_box(
            screen_coordinates.left()*own_bounds.left() + border - left_shift, 
            screen_coordinates.top()*own_bounds.top() + border + box_vertical_indent,  
            screen_coordinates.right()*own_bounds.right() - border, 
            screen_coordinates.bottom()*own_bounds.bottom() - border);
    
    setfontsize(12);
    setcolor(BLACK);
    drawtext_in(search_bound_box, text_to_show);
    
    set_coordinate_system(GL_WORLD);
}

void SearchBox::respond_to_click() {
    if (get_is_active()) {
        inactivate();
    }
    else {
        activate();
    }
}

void SearchBox::append_to_search(const std::string &search_string_addition) {
    current_search += search_string_addition;
}

void SearchBox::append_to_search(const char &search_string_addition) {
    current_search += search_string_addition;
}

std::string SearchBox::get_current_search() const {
    return current_search;
}

void SearchBox::delete_character_current_search(const int number_to_delete) {
    for (int i = 0; (i < number_to_delete) && (current_search.size() != 0); i++) {
        current_search.pop_back();
    }
}

void SearchBox::set_current_search(std::string new_current_search) {
    current_search = new_current_search;
}

void SearchBox::clear_current_search() {
    current_search.clear();
}

/****************************************************************************************
                                LoadingScreen class
 ****************************************************************************************/

LoadingScreen::LoadingScreen() {
    set_edges(0.45, 0.55, 0.4, 0.6);
    set_border_width(0);
    set_inactive_colour(80, 80, 80);
    set_transparency(50);
}

LoadingScreen::~LoadingScreen() {
    ;//the joys of being a simple loading screen
}

void LoadingScreen::draw() {
    UI_element::draw();
    if (get_exists()) {
        begin_drawing();
    
        setcolor(255, 255, 255);
        setfontsize(25);
        t_bound_box screen_bounds = get_visible_screen();
        std::string loading = "Loading..";
        drawtext(screen_bounds.right()/2, screen_bounds.bottom()/2, loading);

        end_drawing();
    }
}

void LoadingScreen::respond_to_click() {
    ;//ignore them
}

/****************************************************************************************
                                ProceedBox class
 ****************************************************************************************/

ProceedBox::ProceedBox() {
    set_edges(0.45, 0.55, 0.2, 0.8);
    set_border_width(0);
    set_inactive_colour(80, 80, 80);
    set_transparency(50);
}

ProceedBox::~ProceedBox() {
    ;//the joys of being a simple box
}

void ProceedBox::draw() {
    UI_element::draw();
    if (get_exists()) {
        begin_drawing();
    
        setcolor(255, 255, 255);
        setfontsize(25);
        t_bound_box screen_bounds = get_visible_screen();
        std::string loading = "Are You Sure You Want to Change the Map? --->";
        drawtext(screen_bounds.right()/2, screen_bounds.bottom()/2, loading);

        end_drawing();
    }
}

void ProceedBox::respond_to_click() {
    ;//ignore them
}

/****************************************************************************************
                                AutocompleteBox class
 ****************************************************************************************/

AutocompleteBox::AutocompleteBox() : UI_element() {
    //still nothing
}

AutocompleteBox::AutocompleteBox(float _top, float _bottom, float _left, float _right) 
    : UI_element(_top, _bottom, _left, _right) {
    //nothing to see here
}

AutocompleteBox::~AutocompleteBox() {
    clear_all_data();
}

void AutocompleteBox::draw() {
    if (get_is_active()) {
        
        set_coordinate_system(GL_SCREEN);
        
        int offset_multiplier = 1;
        
        if (draw_data->find_poi) {
            for (unsigned j = 0; j < poi_data.size() && j < static_cast<unsigned>(number_of_each_result_to_show); j++) {
                draw_box(offset_multiplier, poi_data[j].object_name, poi_data[j].object_type, poi_result_box_colour, false);
                offset_multiplier++;
            }
        } else {

            //First <number_of_each_result_to_show> number of terms are streets
            for (unsigned j = 0; j < street_data.size() && j < static_cast<unsigned>(number_of_each_result_to_show); j++) {
                draw_box(offset_multiplier, street_data[j].object_name, street_data[j].object_type, street_result_box_colour, false);
                offset_multiplier++;
            }

            //Next are intersections
            for (unsigned j = 0; j < intersection_data.size() && j < static_cast<unsigned>(number_of_each_result_to_show); j++) {
                draw_box(offset_multiplier, intersection_data[j].object_name, intersection_data[j].object_type, intersection_result_box_colour, false);
                offset_multiplier++;
            }

            //Next are POIs
            for (unsigned j = 0; j < poi_data.size() && j < static_cast<unsigned>(number_of_each_result_to_show); j++) {
                draw_box(offset_multiplier, poi_data[j].object_name, poi_data[j].object_type, poi_result_box_colour, false);
                offset_multiplier++;
            }
        }
        
        set_coordinate_system(GL_WORLD);
    }
}

void AutocompleteBox::draw_box(int offset_multiplier, std::string text, std::string type, t_color colour, bool draw_border) const {
    
    double delta_y = 0.055;
    double current_left = left;
    double current_top = (offset_multiplier * delta_y) - 0.0065;
    double current_right = right;
    double current_bottom = (offset_multiplier * delta_y) + 0.05;

    double box_vertical_indent = 0.02 + (offset_multiplier * delta_y);
        
    t_bound_box screen_coordinates = get_visible_screen();
    
    if (draw_border || offset_multiplier == currently_selected) {
        setcolor(border_colour);
        fillrect(screen_coordinates.right()*current_left, 
                screen_coordinates.bottom()*current_top, 
                screen_coordinates.right()*current_right, 
                screen_coordinates.bottom()*current_bottom);
    }
        
    setcolor(colour);
    fillrect(screen_coordinates.right()*current_left + border_width, 
            screen_coordinates.bottom()*current_top + border_width,
            screen_coordinates.right()*current_right - border_width, 
            screen_coordinates.bottom()*current_bottom - border_width);
    
    draw_text(box_vertical_indent, text, type);
}

void AutocompleteBox::draw_text(double box_vertical_indent, std::string text, std::string type) const {
    t_bound_box screen_coordinates = get_visible_screen();
    
    while (text.length() > 45) {
        text.pop_back();
    }
    
    //Name of the object
    setfontsize(12);
    setcolor(BLACK);
    drawtext(screen_coordinates.right()*0.1375, screen_coordinates.bottom()*box_vertical_indent, text);
    
    double left_offset = 0.0;
    
    if (type == "street") {
        left_offset = 0.26;
    } else if (type == "intersection") {
        left_offset = 0.2525;
    } else if (type == "poi") {
        left_offset = 0.265;
    }
    
    //Type of the object
    setfontsize(8);
    setcolor(0, 0, 0, 100);
    drawtext(screen_coordinates.right()*left_offset, screen_coordinates.bottom()*(box_vertical_indent + 0.018), type);
}

void AutocompleteBox::respond_to_click() {
    if (get_is_active()) {
        inactivate();
        set_border_width(0);
    }
    else {
        activate();
        set_border_width(2);
    }
}

void AutocompleteBox::set_street_data(std::vector<search_return_data> new_street_data) {
    street_data = new_street_data;
}

void AutocompleteBox::set_intersection_data(std::vector<search_return_data> new_intersection_data) {
    intersection_data = new_intersection_data;
}

void AutocompleteBox::set_poi_data(std::vector<search_return_data> new_poi_data) {
    poi_data = new_poi_data;
}

std::vector<search_return_data> AutocompleteBox::get_street_data() const {
    return street_data;
}

std::vector<search_return_data> AutocompleteBox::get_intersection_data() const {
    return intersection_data;
}

std::vector<search_return_data> AutocompleteBox::get_poi_data() const {
    return poi_data;
}

void AutocompleteBox::clear_street_data() {
    street_data.clear();
}

void AutocompleteBox::clear_intersection_data() {
    intersection_data.clear();
}

void AutocompleteBox::clear_poi_data() {
    poi_data.clear();
}

void AutocompleteBox::clear_all_data() {
    clear_street_data();
    clear_intersection_data();
    clear_poi_data();
    reset_currently_selected();
}

int AutocompleteBox::get_number_of_each_result_to_show() const {
    return number_of_each_result_to_show;
}

void AutocompleteBox::set_number_of_each_result_to_show(int const &new_number) {
    number_of_each_result_to_show = new_number;
}

void AutocompleteBox::set_street_result_box_colour(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a) {
    street_result_box_colour = t_color(r, g, b, a);
}

void AutocompleteBox::set_intersection_result_box_colour(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a) {
    intersection_result_box_colour = t_color(r, g, b, a);
}
void AutocompleteBox::set_poi_result_box_colour(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a) {
    poi_result_box_colour = t_color(r, g, b, a);
}

void AutocompleteBox::down_arrow_increment_selected() {
    currently_selected = (currently_selected >= number_being_shown) ? 1 : currently_selected + 1;
}

void AutocompleteBox::up_arrow_decrement_selected() {
    currently_selected = (currently_selected <= 1) ? number_being_shown : currently_selected - 1;
}

void AutocompleteBox::reset_currently_selected() {
    currently_selected = 0;
}

void AutocompleteBox::update_number_being_shown() {
    street_results_number = std::min(number_of_each_result_to_show, static_cast<int>(street_data.size()));
    intersection_results_number = std::min(number_of_each_result_to_show, static_cast<int>(intersection_data.size()));
    poi_results_number = std::min(number_of_each_result_to_show, static_cast<int>(poi_data.size()));
    number_being_shown = street_results_number + intersection_results_number + poi_results_number;
}

bool AutocompleteBox::get_selected_data(search_return_data &return_data) const {
    if (currently_selected < 0 || currently_selected > number_being_shown) {
        return false;
    }
    if (currently_selected >= 1 && currently_selected <= street_results_number) {
        return_data = street_data[currently_selected-1];
        return true;
    } else if (currently_selected > street_results_number && currently_selected <= (street_results_number + intersection_results_number)) {
        return_data = intersection_data[currently_selected-street_results_number-1];
        return true;
    } else if (currently_selected > (street_results_number + intersection_results_number) && currently_selected <= (street_results_number + intersection_results_number + poi_results_number)) {
        return_data = poi_data[currently_selected-street_results_number-intersection_results_number-1];
        return true;
    }
    return false;
}


/****************************************************************************************
                                HelpScreen class
 ****************************************************************************************/

HelpScreen::HelpScreen() {
    set_edges(0, 1, 0, 1);
    set_border_width(0);
    set_inactive_colour(80, 80, 80);
    set_transparency(50);
}

HelpScreen::~HelpScreen() {
    ;
}

void HelpScreen::draw() {
    UI_element::draw();
    if (get_exists()) {
        begin_drawing();
    
        setcolor(255, 255, 255);
  
        t_bound_box screen_bounds = get_visible_screen();

        setfontsize(25);
        drawtext(screen_bounds.right()/2, screen_bounds.bottom()/10,   "ECE297  Mapper Application");
        setfontsize(20);
        drawtext(screen_bounds.right()/2, screen_bounds.bottom()/5,    "Contributed by cd-026: Anna Shi, Zak Zhao, Echo Zheng");
        setfontsize(25);
        drawtext(screen_bounds.right()/2, screen_bounds.bottom()*3/10, "Find Path Between Intersections:                                                                                         ");
        setfontsize(20);
        drawtext(screen_bounds.right()/2, screen_bounds.bottom()*0.35,  "1. Set Start (right click OR shift + left click) and Destination (right click OR ctrl + left click) first        ");
        setfontsize(20);
        drawtext(screen_bounds.right()/2, screen_bounds.bottom()*0.4,   "2. Click Find Path (and click it every time when Start or Destination Point changes)                        ");
        setfontsize(20);                                                                   
        drawtext(screen_bounds.right()/2, screen_bounds.bottom()*0.45,  "3. Click on display interface, use mouse wheel or arrow buttons to show more path information    ");
        setfontsize(25);
        drawtext(screen_bounds.right()/2, screen_bounds.bottom()*0.55, "Find Path to Closest Point of Interest:                                                                                 ");
        setfontsize(20);
        drawtext(screen_bounds.right()/2, screen_bounds.bottom()*0.6,  "1. Set current intersection (right click and click Find closest POI)                                                       ");
        setfontsize(20);
        drawtext(screen_bounds.right()/2, screen_bounds.bottom()*0.65, "2. Type in POI's name and use arrows keys and enter to select from the results showing up          ");
        setfontsize(25);
        drawtext(screen_bounds.right()/2, screen_bounds.bottom()*0.75,  "Enjoy!");
        end_drawing();
    }
}

void HelpScreen::respond_to_click() {
    HelpScreen::exists = false;
    do_not_zoom = false;
}



/****************************************************************************************
                                PathDisplay1 class
 ****************************************************************************************/

PathDisplay1::PathDisplay1() {
    set_edges(0.05, 0.2, 0, 0.3);
    set_border_colour(0, 0, 0);
    set_border_width(2);
    set_inactive_colour(0, 0, 0);
    set_transparency(80);
}

PathDisplay1::~PathDisplay1() {
    ;
}

void PathDisplay1::draw() {
    UI_element::draw();
    if (get_exists()) {
        begin_drawing();
               
        setcolor(255, 255, 255);
        setfontsize(12);
        t_bound_box screen_bounds = get_visible_screen();
        drawtext(screen_bounds.right()*0.15, screen_bounds.bottom()*0.08, from);
        setfontsize(15);
        drawtext(screen_bounds.right()*0.15, screen_bounds.bottom()*0.13, "TO");
        setfontsize(12);
        drawtext(screen_bounds.right()*0.15, screen_bounds.bottom()*0.18, to);
        
        end_drawing();
    }
}

void PathDisplay1::respond_to_click() {
    ;
}

void PathDisplay1::set_from(std::string from_) {
    from = from_;
}
void PathDisplay1::set_to(std::string to_) {
    to = to_;
}

void PathDisplay1::clear_to_and_from() {
    from = "";
    to = "";
}

/****************************************************************************************
                                PathDisplay2 class
 ****************************************************************************************/

PathDisplay2::PathDisplay2() {
    set_edges(0.2, 1, 0, 0.3);
    set_border_colour(0, 0, 0);
    set_inactive_colour(255, 255, 255);
    set_active_colour(255, 255, 255);
}

PathDisplay2::~PathDisplay2() {
    clear_directions();
}

void PathDisplay2::draw() {
     if (exists) {
        //switch to UI drawing mode
        set_coordinate_system(GL_SCREEN);

        t_bound_box screen_coordinates = get_visible_screen();
        if (border_width != 0) {
            setcolor(border_colour);
            fillrect(screen_coordinates.right()*left, 
                    screen_coordinates.bottom()*top, 
                    screen_coordinates.right()*right, 
                    screen_coordinates.bottom()*bottom);
        }
        if (active) {
            setcolor(active_colour);
            set_transparency(100);
            border_width = 3;
        }
        else {
            setcolor(inactive_colour);
            set_transparency(85);
            border_width = 1;
        }
        fillrect(screen_coordinates.right()*left + border_width, 
                screen_coordinates.bottom()*top + border_width, 
                screen_coordinates.right()*right - border_width, 
                screen_coordinates.bottom()*bottom - border_width);

        //put back to avoid wrecking everything
        set_coordinate_system(GL_WORLD);
    }
       
    if (get_exists()) {
        begin_drawing();
    
        setcolor(0, 0, 0);
        
        for (unsigned i = 1 + text_offset; (i <= (number_to_display + text_offset)) && (i <= direction_icon.size()); i++) {
            int line_number = i - text_offset;
            draw_direction_icon(line_number, direction_icon[i-1]);
            draw_line(line_number*2-1, direction_text[(i*2)-2]);
            draw_line(line_number*2, direction_text[(i*2)-1]);
            draw_distance_and_time(line_number, distance[i-1], time[i-1]);
        }
        
        end_drawing();
    }
}

void PathDisplay2::respond_to_click() {
    active = !active;
    if (active) {
        do_not_zoom = true;
    }
    else {
        do_not_zoom = false;
    }
}

void PathDisplay2::draw_direction_icon(int line_number, int type) {
    
    float x = 0.01;
    float y = line_number*0.128 + 0.10;
       
    std::string png_path;
    if (type == 0) {
        png_path = "libstreetmap/resources/no_turn.png";
    }
    else if (type == 2) {
        png_path = "libstreetmap/resources/turn_left.png";
    }
    else if (type == 1) {
        png_path = "libstreetmap/resources/turn_right.png";
    }
    
    const char *path = png_path.c_str();
    
    set_coordinate_system(GL_SCREEN);
    t_bound_box screen_bounds = get_visible_screen();
    
    //fix the icon offset (icon size = 50x50)
    t_point screen = t_point(screen_bounds.right()*x, screen_bounds.bottom()*y);
    t_point world = t_point(scrn_to_world(screen).x - 25, scrn_to_world(screen).y - 25);
    screen = world_to_scrn(world);
        
    draw_surface(load_png_from_file(path),screen_bounds.right()*x, screen_bounds.bottom()*y);        

    set_coordinate_system(GL_WORLD);    
}

void PathDisplay2::draw_line(int line_number, std::string text) {
    set_coordinate_system(GL_SCREEN);
    t_bound_box screen_bounds = get_visible_screen();
    int even_count = (line_number-1)/2;

    setfontsize(15);
    
    drawtext(screen_bounds.right()*0.17, screen_bounds.bottom()*(0.03*line_number + 0.21 + 0.068*(even_count)), text); 

    set_coordinate_system(GL_WORLD);
}

void PathDisplay2::draw_distance_and_time(int line_number, double distance_, double time_) {
    set_coordinate_system(GL_SCREEN);
    
    t_bound_box screen_bounds = get_visible_screen();
    
    std::string distance_units;
    std::string time_units;
    
    if (distance_ >= 1000) {
        distance_ /= 1000;
        distance_units = "km ";
    } else {
        distance_units = "m ";
    }
    
    if (time_ >= 60) {
        time_ /= 60;
        time_units = "min ";
    } else if (time_ >= 3600) {
        time_ /= 3600;
        time_units = "hr";
    } else {
        time_units = "s ";
    }
    
    std::string distance_text = std::to_string(distance_);
    std::string time_text = std::to_string(time_);
    
    for (int i = 0 ; i < 4; i++) {
        distance_text.pop_back();
        time_text.pop_back();
    }
    
    setfontsize(12);
    
    drawtext(screen_bounds.right()*0.06, screen_bounds.bottom()*(0.044*line_number + 0.18 + 0.08375*(line_number)), "Distance: " + distance_text + distance_units); 
    drawtext(screen_bounds.right()*0.25, screen_bounds.bottom()*(0.044*line_number + 0.18 + 0.08375*(line_number)), "Time: " + time_text + time_units); 
    
    set_coordinate_system(GL_WORLD);
}

void PathDisplay2::update_directions(int direction_number, std::string text_line_1, std::string text_line_2, int icon, double distance_, double time_) {
    direction_icon.push_back(-1);
    direction_text.push_back("");
    direction_text.push_back("");
    distance.push_back(0.0);
    time.push_back(0.0);
    
    direction_icon[direction_number-1] = icon;
    direction_text[(direction_number*2)-2] = text_line_1;
    direction_text[(direction_number*2)-1] = text_line_2;
    distance[direction_number-1] = distance_;
    time[direction_number-1] = time_;
}

void PathDisplay2::clear_directions() {
    direction_icon.clear();
    direction_text.clear();
    distance.clear();
    time.clear();
}

int PathDisplay2::get_number_to_display() {
    return number_to_display;
}

unsigned PathDisplay2::get_number_of_directions() {
    return direction_icon.size();
}

/****************************************************************************************
                                RightClickMenu1 class
 ****************************************************************************************/

RightClickMenu1::RightClickMenu1() {
    set_border_colour(0, 0, 0);
    set_border_width(2);
    set_inactive_colour(255, 255, 255);
}

RightClickMenu1::~RightClickMenu1() {
}

void RightClickMenu1::draw() {
    UI_element::draw();
    if (exists) {
        set_coordinate_system(GL_SCREEN);
        t_bound_box screen_bounds = get_visible_screen();
        setcolor(0, 0, 0);
        setfontsize(11);
        drawtext(screen_bounds.right()*(left + right)/2, screen_bounds.bottom()*(top + bottom)/2, "Set start");
        set_coordinate_system(GL_WORLD);
    }
}

void RightClickMenu1::respond_to_click() {
    exists = false;  
    r_click_menu2->disable_existence();
    r_click_menu3->disable_existence();
    draw_data->found_path.clear();
    draw_data->from_x = draw_data->right_click_x;
    draw_data->from_y = draw_data->right_click_y;
    draw_data->find_poi_x = FLT_MAX;
    draw_data->find_poi_y = FLT_MAX;
    
    if (draw_data->find_poi) {
        draw_data->to_x = FLT_MAX;
        draw_data->to_y = FLT_MAX;
    }
    
    draw_data->find_poi = false;
}


/****************************************************************************************
                                RightClickMenu2 class
 ****************************************************************************************/

RightClickMenu2::RightClickMenu2() {
    set_border_colour(0, 0, 0);
    set_border_width(2);
    set_inactive_colour(255, 255, 255);
}

RightClickMenu2::~RightClickMenu2() {
}

void RightClickMenu2::draw() {
    UI_element::draw();
    if (exists) {
        set_coordinate_system(GL_SCREEN);
        t_bound_box screen_bounds = get_visible_screen();
        setcolor(0, 0, 0);
        setfontsize(11);
        drawtext(screen_bounds.right()*(left + right)/2, screen_bounds.bottom()*(top + bottom)/2, "Set destination");
        set_coordinate_system(GL_WORLD);
    }
}

void RightClickMenu2::respond_to_click() {
    exists = false;  
    r_click_menu1->disable_existence();
    r_click_menu3->disable_existence();
    draw_data->found_path.clear();
    draw_data->to_x = draw_data->right_click_x;
    draw_data->to_y = draw_data->right_click_y;
    draw_data->find_poi_x = FLT_MAX;
    draw_data->find_poi_y = FLT_MAX;
    
    if (draw_data->find_poi) {
        draw_data->from_x = FLT_MAX;
        draw_data->from_y = FLT_MAX;
    }
    
    draw_data->find_poi = false;
}


/****************************************************************************************
                                RightClickMenu3 class
 ****************************************************************************************/

RightClickMenu3::RightClickMenu3() {
    set_border_colour(0, 0, 0);
    set_border_width(2);
    set_inactive_colour(255, 255, 255);
}

RightClickMenu3::~RightClickMenu3() {
}

void RightClickMenu3::draw() {
    UI_element::draw();
    if (exists) {
        set_coordinate_system(GL_SCREEN);
        t_bound_box screen_bounds = get_visible_screen();
        setcolor(0, 0, 0);
        setfontsize(11);
        drawtext(screen_bounds.right()*(left + right)/2, screen_bounds.bottom()*(top + bottom)/2, "Find closest POI");
        set_coordinate_system(GL_WORLD);
    }
}

void RightClickMenu3::respond_to_click() {
    exists = false;  
    r_click_menu1->disable_existence();
    r_click_menu2->disable_existence();
    draw_data->found_path.clear();
    draw_data->selected_street_ids.clear();
    draw_data->search_box.clear_current_search();
    draw_data->autocomplete_box.clear_all_data();
    draw_data->find_poi_x = draw_data->right_click_x;
    draw_data->find_poi_y = draw_data->right_click_y;
    draw_data->find_poi = true;
    draw_data->from_x = FLT_MAX;
    draw_data->from_y = FLT_MAX;
    draw_data->to_x = FLT_MAX;
    draw_data->to_y = FLT_MAX;
    
    pd1->disable_existence();
    pd2->disable_existence();
    pathdisplay_up->disable_existence();
    pathdisplay_down->disable_existence();
    do_not_zoom = false;
    text_offset = 0;
    
    draw_data->search_box.respond_to_click();
    draw_data->autocomplete_box.respond_to_click();
}