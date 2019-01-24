/*
 * All UI elements inherit from the UI_element class.
 * Add whatever you'd like at will.
 */

#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include "graphics.h"
#include "map_data.h"

class UI_element {
protected:
    //define edges of the element, in % of the screen from the top left corner
    //e.g. top = 0.0, bottom = 0.1, left = 0.0, right = 0.1 would drop a box in the top left corner
    float top = 0.0;
    float bottom = 0.0;
    float left = 0.0;
    float right = 0.0;
    
    //control whether the UI element is there
    //enables disabling of element without deleting
    volatile bool exists = true;
    
    //active may mean different things depending on the UI element
    bool active = false;
    
    //border width in pixels
    int border_width = 2;
    
    t_color active_colour = t_color(102, 102, 102, 255);
    t_color inactive_colour = t_color(191, 191, 191, 255);
    t_color border_colour = t_color(255, 255, 255, 255);
    
    void (*active_action) (void) = NULL;
    void (*inactive_action) (void) = NULL;
    
public:
    UI_element();
    UI_element(float _top, float _bottom, float _left, float _right);
    virtual ~UI_element();
    
    virtual void draw();
    
    //set coordinate system to screen and world, respectively
    void begin_drawing();
    void end_drawing();
    
    void toggle_existence();
    void enable_existence();
    void disable_existence();
    bool get_exists();
    
    virtual void toggle_activate();
    virtual void activate();
    virtual void inactivate();
    bool get_is_active();
    
    t_bound_box get_element_bounds();
    
    //ex set_edges(0.4, 0.6, 0.4, 0.6); for a box in the middle of the screen
    void set_edges(float _top, float _bottom, float _left, float _right);
    
    //ex set_border_width(3); for a border of 3 pixels
    void set_border_width(int _border_width);
    int get_border_width() const;
    
    //ex. set_active_colour(255, 0, 0); for eye-bleeding red
    void set_active_colour(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a = 255);
    void set_inactive_colour(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a = 255);
    void set_border_colour(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a = 255);
    
    //ex set_transparency(50); for half transparency
    void set_transparency(float transparency); //from 0 to 100
    
    //x and y are world coordinates
    //just pass in what act_on_mouse_button gives
    bool clicked_on(float x, float y);
    
    void set_action(void (*active_function)(void), void (*inactive_function) (void));
    
    //write your own!
    virtual void respond_to_click();

};

class Button : public UI_element {
private:
    bool has_icon = false;
    Surface icon;
    int icon_height;
    int icon_width;
    
public:
    Button();
    Button(float _top, float _bottom, float _left, float _right);
    ~Button();
    
    void set_icon(const char* _icon_location, int _icon_width, int _icon_height);
    
    virtual void draw();
    
    void respond_to_click();    
};

class SearchBox : public UI_element {
private:
    std::string current_search;
    
public:
    SearchBox();
    SearchBox(float _top, float _bottom, float _left, float _right);
    ~SearchBox();
    
    virtual void draw();
    
    void respond_to_click();   
    
    void append_to_search(const std::string &search_string_addition);
    void append_to_search(const char &search_string_addition);
    std::string get_current_search() const;
    void delete_character_current_search(const int number_to_delete);
    void set_current_search(std::string new_current_search);
    void clear_current_search();
};

class LoadingScreen : public UI_element {
private:
    
public:
    LoadingScreen();
    ~LoadingScreen();
    
    virtual void draw();
    virtual void respond_to_click();
};

class ProceedBox : public UI_element {
private:
    
public:
    ProceedBox();
    ~ProceedBox();
    
    virtual void draw();
    virtual void respond_to_click();
};

class AutocompleteBox : public UI_element {
private:
    std::vector<search_return_data> street_data;
    std::vector<search_return_data> intersection_data;
    std::vector<search_return_data> poi_data;
    
    int number_of_each_result_to_show;
    int number_being_shown;
    int currently_selected = 0;
    
    int street_results_number;
    int intersection_results_number;
    int poi_results_number;
    
    t_color street_result_box_colour = t_color(255, 255, 255, 255);
    t_color intersection_result_box_colour = t_color(255, 255, 255, 255);
    t_color poi_result_box_colour = t_color(255, 255, 255, 255);
    
public:
    AutocompleteBox();
    AutocompleteBox(float _top, float _bottom, float _left, float _right);
    ~AutocompleteBox();
    
    virtual void draw();
    void draw_box(int offset_multiplier, std::string text, std::string type, t_color colour, bool draw_border) const;
    void draw_text(double box_vertical_indent, std::string text, std::string type) const;
    
    void respond_to_click();
    
    void set_street_data(std::vector<search_return_data> new_street_data);
    void set_intersection_data(std::vector<search_return_data> new_intersection_data);
    void set_poi_data(std::vector<search_return_data> new_poi_data);
    std::vector<search_return_data> get_street_data() const;
    std::vector<search_return_data> get_intersection_data() const;
    std::vector<search_return_data> get_poi_data() const;
    void clear_street_data();
    void clear_intersection_data();
    void clear_poi_data();
    void clear_all_data();
    
    int get_number_of_each_result_to_show() const;
    void set_number_of_each_result_to_show(int const &new_number);
    
    void set_street_result_box_colour(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a = 255);
    void set_intersection_result_box_colour(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a = 255);
    void set_poi_result_box_colour(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a = 255);
    
    void down_arrow_increment_selected();
    void up_arrow_decrement_selected();
    void reset_currently_selected();
    
    void update_number_being_shown();
    
    bool get_selected_data(search_return_data &return_data) const;
};

class HelpScreen : public UI_element {
private:
    
public:
    HelpScreen();
    ~HelpScreen();
    
    virtual void draw();
    virtual void respond_to_click();
};


class PathDisplay1 : public UI_element {
private:
    std::string from;
    std::string to;
public:
    PathDisplay1();
    ~PathDisplay1();
    
    virtual void draw();
    virtual void respond_to_click();
    void set_from(std::string from_);
    void set_to(std::string to_);
    void clear_to_and_from();
};

class PathDisplay2 : public UI_element {
private:
    int number_to_display = 6;
    
    std::vector<std::string> direction_text;
    std::vector<int> direction_icon;
    std::vector<double> distance;
    std::vector<double> time;
public:
    PathDisplay2();
    ~PathDisplay2();
    
    virtual void draw();
    virtual void respond_to_click();
    void draw_direction_icon(int line_number, int type);
    void draw_line(int line_number, std::string text);
    void draw_distance_and_time(int line_number, double distance_, double time_);
    void update_directions(int direction_number, std::string text_line_1, std::string text_line_2, int icon, double distance_, double time_);
    void clear_directions();
    int get_number_to_display();
    unsigned get_number_of_directions();
};


class RightClickMenu1 : public UI_element {
private:
    
public:
    RightClickMenu1();
    ~RightClickMenu1();
    
    virtual void draw();
    virtual void respond_to_click();
};

class RightClickMenu2 : public UI_element {
private:
    
public:
    RightClickMenu2();
    ~RightClickMenu2();
    
    virtual void draw();
    virtual void respond_to_click();
};


class RightClickMenu3 : public UI_element {
private:
    
public:
    RightClickMenu3();
    ~RightClickMenu3();
    
    virtual void draw();
    virtual void respond_to_click();
};

#endif /* UI_ELEMENT_H */
