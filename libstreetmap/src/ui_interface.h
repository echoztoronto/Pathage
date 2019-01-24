/*
 * A file for holding the global variables used in the user interface.
 */

#ifndef UI_INTERFACE_H
#define UI_INTERFACE_H

#include "ui_element.h"
#include "draw_data.h"
#include "map_data.h"
#include "graphics.cpp"

extern Draw_data* draw_data;
extern std::string map_name_;
extern Map_data const *map_data;
extern volatile bool change_map_on;
extern unsigned text_offset;

void reinitialize_ui_elements();
void initialize_ui_elements();
void disable_change_map_buttons();
void enable_change_map_buttons(void (*draw_screen_map)(void));
void delete_all_ui_elements();
void disable_change_maps();
void enable_help_button(void (*draw_screen_map)(void));
void disable_right_click_menu(void (*draw_screen_map)(void));
void enable_right_click_menu(float x, float y, void (*draw_screen_map)(void));
void update_right_click_menu_position(float x, float y);

Button* beijing;
Button* cairo;
Button* cape_town;
Button* golden_horseshoe;
Button* hamilton;
Button* hong_kong;
Button* iceland;
Button* interlaken;
Button* london;
Button* moscow;
Button* new_delhi;
Button* new_york;
Button* rio_de_janeiro;
Button* saint_helena;
Button* singapore;
Button* sydney;
Button* tehran;
Button* tokyo;
Button* toronto;

Button* up_arrow;
Button* down_arrow;
Button* left_arrow;
Button* right_arrow;

Button* pathdisplay_up;
Button* pathdisplay_down;

void do_nothing();
void pd_up();
void pd_down();

void beijing_click();
void cairo_click();
void cape_town_click();
void golden_horseshoe_click();
void hamilton_click();
void hong_kong_click();
void iceland_click();
void interlaken_click();
void london_click();
void moscow_click();
void new_delhi_click();
void new_york_click();
void rio_de_janeiro_click();
void saint_helena_click();
void singapore_click();
void sydney_click();
void tehran_click();
void tokyo_click();
void toronto_click();

LoadingScreen* loading;
ProceedBox* clickproceed;
HelpScreen* help;
PathDisplay1* pd1;
PathDisplay2* pd2;

RightClickMenu1* r_click_menu1;
RightClickMenu2* r_click_menu2;
RightClickMenu3* r_click_menu3;

//used when changing maps
void reinitialize_ui_elements() {
    draw_data->add_ui_element(loading);
    loading->disable_existence();
    
    draw_data->add_ui_element(clickproceed);
    clickproceed->disable_existence();
    
    draw_data->add_ui_element(beijing);
    draw_data->add_ui_element(cairo);
    draw_data->add_ui_element(cape_town);
    draw_data->add_ui_element(golden_horseshoe);
    draw_data->add_ui_element(hamilton);
    draw_data->add_ui_element(hong_kong);
    draw_data->add_ui_element(iceland);
    draw_data->add_ui_element(interlaken);
    draw_data->add_ui_element(london);
    draw_data->add_ui_element(moscow);
    draw_data->add_ui_element(new_delhi);
    draw_data->add_ui_element(new_york);
    draw_data->add_ui_element(rio_de_janeiro);
    draw_data->add_ui_element(saint_helena);
    draw_data->add_ui_element(singapore);
    draw_data->add_ui_element(sydney);
    draw_data->add_ui_element(tehran);
    draw_data->add_ui_element(tokyo);
    draw_data->add_ui_element(toronto);
    disable_change_map_buttons();
}

void initialize_ui_elements() {
    
    loading = new LoadingScreen;   
    draw_data->add_ui_element(loading);
    loading->disable_existence();
    
    clickproceed = new ProceedBox;
    draw_data->add_ui_element(clickproceed);
    clickproceed->disable_existence();   
    
    /* initialize map change buttons */
    
    beijing = new Button;
    cairo = new Button;
    cape_town = new Button;
    golden_horseshoe = new Button;
    hamilton = new Button;
    hong_kong = new Button;
    iceland = new Button;
    interlaken = new Button;
    london = new Button;
    moscow = new Button;
    new_delhi = new Button;
    new_york = new Button;
    rio_de_janeiro = new Button;
    saint_helena = new Button;
    singapore = new Button;
    sydney = new Button;
    tehran = new Button;
    tokyo = new Button;
    toronto = new Button;
    
    up_arrow = new Button;
    down_arrow = new Button;
    left_arrow = new Button;
    right_arrow = new Button;
    
    //arrow buttons  
    //up
    up_arrow->set_edges(0, 0.05, 0.3, 0.7);
    up_arrow->set_active_colour(255, 255, 255, 255);
    up_arrow->set_inactive_colour(80, 80, 80, 80);
    up_arrow->set_transparency(30);
    up_arrow->set_icon("libstreetmap/resources/up.png", 35, 35);
    up_arrow->set_action(do_nothing, up);
    draw_data->add_ui_element(up_arrow);
    
    //down
    down_arrow->set_edges(0.95, 1, 0.3, 0.7);
    down_arrow->set_active_colour(255, 255, 255, 255);
    down_arrow->set_inactive_colour(80, 80, 80, 80);
    down_arrow->set_transparency(30);
    down_arrow->set_icon("libstreetmap/resources/down.png", 35, 35);
    down_arrow->set_action(do_nothing, down);
    draw_data->add_ui_element(down_arrow);
    
    //left
    left_arrow->set_edges(0.3, 0.7, 0, 0.03);
    left_arrow->set_active_colour(255, 255, 255, 255);
    left_arrow->set_inactive_colour(80, 80, 80, 80);
    left_arrow->set_transparency(30);
    left_arrow->set_icon("libstreetmap/resources/left.png", 35, 35);
    left_arrow->set_action(do_nothing, left);
    draw_data->add_ui_element(left_arrow);
    
    //right
    right_arrow->set_edges(0.3, 0.7, 0.97, 1);
    right_arrow->set_active_colour(255, 255, 255, 255);
    right_arrow->set_inactive_colour(80, 80, 80, 80);
    right_arrow->set_transparency(30);
    right_arrow->set_icon("libstreetmap/resources/right.png", 35, 35);
    right_arrow->set_action(do_nothing, right);
    draw_data->add_ui_element(right_arrow);   
       
    
    pd1 = new PathDisplay1;
    draw_data->add_ui_element(pd1);
    pd1->disable_existence();    
    
    pd2 = new PathDisplay2;
    draw_data->add_ui_element(pd2);
    pd2->disable_existence();   
    
    r_click_menu1 = new RightClickMenu1;
    draw_data->add_ui_element(r_click_menu1);
    r_click_menu1->disable_existence();  
    
    r_click_menu2 = new RightClickMenu2;
    draw_data->add_ui_element(r_click_menu2);
    r_click_menu2->disable_existence();  
    
    r_click_menu3 = new RightClickMenu3;
    draw_data->add_ui_element(r_click_menu3);
    r_click_menu3->disable_existence();  
    
    pathdisplay_up = new Button;
    pathdisplay_down = new Button;
    
    pathdisplay_up->set_edges(0.201, 0.241, 0.272, 0.302);
    pathdisplay_up->set_inactive_colour(80, 80, 80, 80);
    pathdisplay_up->set_transparency(0);
    pathdisplay_up->set_icon("libstreetmap/resources/pd_up.png", 30, 30);
    pathdisplay_up->set_action(do_nothing, pd_up);
    pathdisplay_up->disable_existence();
    draw_data->add_ui_element(pathdisplay_up);
    
    pathdisplay_down->set_edges(0.96, 1, 0.272, 0.302);
    pathdisplay_down->set_inactive_colour(80, 80, 80, 80);
    pathdisplay_down->set_transparency(0);
    pathdisplay_down->set_icon("libstreetmap/resources/pd_down.png", 30, 30);
    pathdisplay_down->set_action(do_nothing, pd_down);
    pathdisplay_down->disable_existence();
    draw_data->add_ui_element(pathdisplay_down);
    
    
    // all icons for maps are from https://sa3er.deviantart.com/art/World-Cities-2-0-386839729
    beijing->set_edges(0.15, 0.25, 0.14, 0.20);
    beijing->set_icon("libstreetmap/resources/beijing.png", 210, 140);
    beijing->set_border_colour(255, 255, 255, 255);
    beijing->set_active_colour(255, 255, 255, 255);
    beijing->set_inactive_colour(255, 255, 255, 255);
    beijing->set_action(beijing_click, beijing_click);
    draw_data->add_ui_element(beijing);
    
    cairo->set_edges(0.15, 0.25, 0.283, 0.383);
    cairo->set_icon("libstreetmap/resources/cairo.png", 210, 140);
    cairo->set_border_colour(255, 255, 255, 255);
    cairo->set_active_colour(255, 255, 255, 255);
    cairo->set_inactive_colour(255, 255, 255, 255);
    cairo->set_action(cairo_click, cairo_click);
    draw_data->add_ui_element(cairo);
    
    cape_town->set_edges(0.15, 0.25, 0.45, 0.55);
    cape_town->set_icon("libstreetmap/resources/capetown.png", 210, 140);
    cape_town->set_border_colour(255, 255, 255, 255);
    cape_town->set_active_colour(255, 255, 255, 255);
    cape_town->set_inactive_colour(255, 255, 255, 255);
    cape_town->set_action(cape_town_click, cape_town_click);
    draw_data->add_ui_element(cape_town);

    golden_horseshoe->set_edges(0.15, 0.25, 0.616, 0.726);
    golden_horseshoe->set_icon("libstreetmap/resources/golden.png", 210, 140);
    golden_horseshoe->set_border_colour(255, 255, 255, 255);
    golden_horseshoe->set_active_colour(255, 255, 255, 255);
    golden_horseshoe->set_inactive_colour(255, 255, 255, 255);
    golden_horseshoe->set_action(golden_horseshoe_click, golden_horseshoe_click);
    draw_data->add_ui_element(golden_horseshoe);
    
    hamilton->set_edges(0.15, 0.25, 0.783, 0.883);
    hamilton->set_icon("libstreetmap/resources/hamilton.png", 210, 140);
    hamilton->set_border_colour(255, 255, 255, 255);
    hamilton->set_active_colour(255, 255, 255, 255);
    hamilton->set_inactive_colour(255, 255, 255, 255);
    hamilton->set_action(hamilton_click, hamilton_click);
    draw_data->add_ui_element(hamilton);
    
    hong_kong->set_edges(0.35, 0.45, 0.14, 0.20);
    hong_kong->set_icon("libstreetmap/resources/hongkong.png", 210, 140);
    hong_kong->set_border_colour(255, 255, 255, 255);
    hong_kong->set_active_colour(255, 255, 255, 255);
    hong_kong->set_inactive_colour(255, 255, 255, 255);
    hong_kong->set_action(hong_kong_click, hong_kong_click);
    draw_data->add_ui_element(hong_kong);

    iceland->set_edges(0.35, 0.45, 0.283, 0.383);
    iceland->set_icon("libstreetmap/resources/iceland.png", 210, 140);
    iceland->set_border_colour(255, 255, 255, 255);
    iceland->set_active_colour(255, 255, 255, 255);
    iceland->set_inactive_colour(255, 255, 255, 255);
    iceland->set_action(iceland_click, iceland_click);
    draw_data->add_ui_element(iceland);

    interlaken->set_edges(0.35, 0.45, 0.616, 0.726);
    interlaken->set_icon("libstreetmap/resources/interlaken.png", 210, 140);
    interlaken->set_border_colour(255, 255, 255, 255);
    interlaken->set_active_colour(255, 255, 255, 255);
    interlaken->set_inactive_colour(255, 255, 255, 255);
    interlaken->set_action(interlaken_click, interlaken_click);
    draw_data->add_ui_element(interlaken);

    london->set_edges(0.35, 0.45, 0.783, 0.883);
    london->set_icon("libstreetmap/resources/london.png", 210, 140);
    london->set_border_colour(255, 255, 255, 255);
    london->set_active_colour(255, 255, 255, 255);
    london->set_inactive_colour(255, 255, 255, 255);
    london->set_action(london_click, london_click);
    draw_data->add_ui_element(london);

    moscow->set_edges(0.55, 0.65,  0.14, 0.20);
    moscow->set_icon("libstreetmap/resources/moscow.png", 210, 140);
    moscow->set_border_colour(255, 255, 255, 255);
    moscow->set_active_colour(255, 255, 255, 255);
    moscow->set_inactive_colour(255, 255, 255, 255);
    moscow->set_action(moscow_click, moscow_click);
    draw_data->add_ui_element(moscow);

    new_delhi->set_edges(0.55, 0.65, 0.283, 0.383);
    new_delhi->set_icon("libstreetmap/resources/new.png", 210, 140);
    new_delhi->set_border_colour(255, 255, 255, 255);
    new_delhi->set_active_colour(255, 255, 255, 255);
    new_delhi->set_inactive_colour(255, 255, 255, 255);
    new_delhi->set_action(new_delhi_click, new_delhi_click);
    draw_data->add_ui_element(new_delhi);

    new_york->set_edges(0.55, 0.65, 0.616, 0.726);
    new_york->set_icon("libstreetmap/resources/newyork.png", 210, 140);
    new_york->set_border_colour(255, 255, 255, 255);
    new_york->set_active_colour(255, 255, 255, 255);
    new_york->set_inactive_colour(255, 255, 255, 255);
    new_york->set_action(new_york_click, new_york_click);
    draw_data->add_ui_element(new_york);

    rio_de_janeiro->set_edges(0.55, 0.65, 0.783, 0.883);
    rio_de_janeiro->set_icon("libstreetmap/resources/rio.png", 210, 140);
    rio_de_janeiro->set_border_colour(255, 255, 255, 255);
    rio_de_janeiro->set_active_colour(255, 255, 255, 255);
    rio_de_janeiro->set_inactive_colour(255, 255, 255, 255);
    rio_de_janeiro->set_action(rio_de_janeiro_click, rio_de_janeiro_click);
    draw_data->add_ui_element(rio_de_janeiro);

    saint_helena->set_edges(0.75, 0.85, 0.14, 0.20);
    saint_helena->set_icon("libstreetmap/resources/saint.png", 210, 140);
    saint_helena->set_border_colour(255, 255, 255, 255);
    saint_helena->set_active_colour(255, 255, 255, 255);
    saint_helena->set_inactive_colour(255, 255, 255, 255);
    saint_helena->set_action(saint_helena_click, saint_helena_click);
    draw_data->add_ui_element(saint_helena);

    singapore->set_edges(0.75, 0.85, 0.283, 0.383);
    singapore->set_icon("libstreetmap/resources/singapore.png", 210, 140);
    singapore->set_border_colour(255, 255, 255, 255);
    singapore->set_active_colour(255, 255, 255, 255);
    singapore->set_inactive_colour(255, 255, 255, 255);
    singapore->set_action(singapore_click, singapore_click);
    draw_data->add_ui_element(singapore);

    sydney->set_edges(0.75, 0.85, 0.45, 0.55);
    sydney->set_icon("libstreetmap/resources/sydney.png", 210, 140);
    sydney->set_border_colour(255, 255, 255, 255);
    sydney->set_active_colour(255, 255, 255, 255);
    sydney->set_inactive_colour(255, 255, 255, 255);
    sydney->set_action(sydney_click, sydney_click);
    draw_data->add_ui_element(sydney);

    tehran->set_edges(0.75, 0.85, 0.616, 0.726);
    tehran->set_icon("libstreetmap/resources/tehran.png", 210, 140);
    tehran->set_border_colour(255, 255, 255, 255);
    tehran->set_active_colour(255, 255, 255, 255);
    tehran->set_inactive_colour(255, 255, 255, 255);
    tehran->set_action(tehran_click, tehran_click);
    draw_data->add_ui_element(tehran);

    tokyo->set_edges(0.75, 0.85, 0.783, 0.883);
    tokyo->set_icon("libstreetmap/resources/tokyo.png", 210, 140);
    tokyo->set_border_colour(255, 255, 255, 255);
    tokyo->set_active_colour(255, 255, 255, 255);
    tokyo->set_inactive_colour(255, 255, 255, 255);
    tokyo->set_action(tokyo_click, tokyo_click);
    draw_data->add_ui_element(tokyo);

    toronto->set_edges(0.45, 0.55, 0.45, 0.55);
    toronto->set_icon("libstreetmap/resources/toronto.png", 210, 140);
    toronto->set_border_colour(255, 255, 255, 255);
    toronto->set_active_colour(255, 255, 255, 255);
    toronto->set_inactive_colour(255, 255, 255, 255);
    toronto->set_action(toronto_click, toronto_click);
    draw_data->add_ui_element(toronto);
    
    help = new HelpScreen;
    draw_data->add_ui_element(help);
    help->disable_existence(); 
    
    disable_change_map_buttons(); //disable all by default  
}

void disable_change_map_buttons() {
    beijing->disable_existence();
    cairo->disable_existence();
    cape_town->disable_existence();
    golden_horseshoe->disable_existence();
    hamilton->disable_existence();
    hong_kong->disable_existence();
    iceland->disable_existence();
    interlaken->disable_existence();
    london->disable_existence();
    moscow->disable_existence();
    new_delhi->disable_existence();
    new_york->disable_existence();
    rio_de_janeiro->disable_existence();
    saint_helena->disable_existence();
    singapore->disable_existence();
    sydney->disable_existence();
    tehran->disable_existence();
    tokyo->disable_existence();
    toronto->disable_existence();
}

void enable_change_map_buttons(void (*draw_screen_map)(void)) {
    if (beijing->get_exists()) {
        //disable the buttons instead if the user clicks twice
        disable_change_map_buttons();
    }
    else {
        //hide path display stuff
        draw_data->show_found_path = false;
        draw_data->from_x = FLT_MAX;
        draw_data->from_y = FLT_MAX;
        draw_data->to_x = FLT_MAX;
        draw_data->to_y = FLT_MAX;
        draw_data->find_poi_x = FLT_MAX;
        draw_data->find_poi_y = FLT_MAX;
        draw_data->find_poi = false;

        pd1->disable_existence();
        pd2->disable_existence();
        left_arrow->enable_existence();
        pathdisplay_up->disable_existence();
        pathdisplay_down->disable_existence();
        do_not_zoom = false; 
        text_offset = 0;
        
        //turn on change map stuff
        beijing->enable_existence();
        cairo->enable_existence();
        cape_town->enable_existence();
        golden_horseshoe->enable_existence();
        hamilton->enable_existence();
        hong_kong->enable_existence();
        iceland->enable_existence();
        interlaken->enable_existence();
        london->enable_existence();
        moscow->enable_existence();
        new_delhi->enable_existence();
        new_york->enable_existence();
        rio_de_janeiro->enable_existence();
        saint_helena->enable_existence();
        singapore->enable_existence();
        sydney->enable_existence();
        tehran->enable_existence();
        tokyo->enable_existence();
        toronto->enable_existence();
    }
    draw_screen_map();
}

void do_nothing() {

}

void pd_up() {
    if (text_offset > 0) {
        text_offset--;
    }
}

void pd_down() {
    unsigned limit = std::max(0, static_cast<int>(pd2->get_number_of_directions() - pd2->get_number_to_display()));
    if (text_offset < limit) {
        text_offset ++;
    }
}

void disable_change_maps() {
    disable_change_map_buttons();
    clickproceed->disable_existence();
    change_map_on = false;
}

void beijing_click() {
    map_name_ = "beijing_china";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void cairo_click() {
    map_name_ = "cairo_egypt";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void cape_town_click() {
    map_name_ = "cape-town_south-africa";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void golden_horseshoe_click() {
    map_name_ = "golden-horseshoe_canada";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void hamilton_click() {
    map_name_ = "hamilton_canada";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void hong_kong_click() {
    map_name_ = "hong-kong_china";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void iceland_click() {
    map_name_ = "iceland";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void interlaken_click() {
    map_name_ = "interlaken_switzerland";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void london_click() {
    map_name_ = "london_england";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void moscow_click() {
    map_name_ = "moscow_russia";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void new_delhi_click() {
    map_name_ = "new-delhi_india";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void new_york_click() {
    map_name_ = "new-york_usa";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void rio_de_janeiro_click() {
    map_name_ = "rio-de-janeiro_brazil";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void saint_helena_click() {
    map_name_ = "saint-helena";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void singapore_click() {
    map_name_ = "singapore";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void sydney_click() {
    map_name_ = "sydney_australia";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void tehran_click() {
    map_name_ = "tehran_iran";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void tokyo_click() {
    map_name_ = "tokyo_japan";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void toronto_click() {
    map_name_ = "toronto_canada";   
    disable_change_map_buttons();
    clickproceed->enable_existence();
    change_map_on = true;
    change_button_text("Exit" , "YES!");
    change_button_text("Reset" , "NOOO!");
}

void enable_help_button(void (*draw_screen_map)(void)) {
    help->toggle_existence();
    if (help->get_exists()) {
        do_not_zoom = true;
    }
    else {
        do_not_zoom = false;
    }
    
    draw_screen_map();
}

void disable_right_click_menu(void (*draw_screen_map)(void)) {
    r_click_menu1->disable_existence();
    r_click_menu2->disable_existence();
    r_click_menu3->disable_existence();
    draw_screen_map();
}

void enable_right_click_menu(float x, float y, void (*draw_screen_map)(void)) {
    update_right_click_menu_position(x, y);
    r_click_menu1->enable_existence();
    r_click_menu2->enable_existence();
    r_click_menu3->enable_existence();
    draw_screen_map();
}

void update_right_click_menu_position(float x, float y) {
    //convert world x and y to screen x and y
    t_point click_xy(x, y);
    t_point click_screen = world_to_scrn(click_xy);
    
    set_coordinate_system(GL_SCREEN);
    t_bound_box screen_bounds = get_visible_screen();
    
    float menu_x = click_screen.x / screen_bounds.right();
    float menu_y = click_screen.y / screen_bounds.bottom();
    
    //x and y here are top left corner
    r_click_menu1->set_edges(menu_y , menu_y + 0.05, menu_x, menu_x + 0.1);
    r_click_menu2->set_edges(menu_y +0.045 , menu_y + 0.1, menu_x, menu_x + 0.1);
    r_click_menu3->set_edges(menu_y +0.095 , menu_y + 0.145, menu_x, menu_x + 0.1);
    
    set_coordinate_system(GL_WORLD);
}

void delete_all_ui_elements() {
    delete loading;
    delete help;
    delete pd1;
    delete pd2;
    delete pathdisplay_up;
    delete pathdisplay_down;
    delete r_click_menu1;
    delete r_click_menu2;
    delete r_click_menu3;
    delete beijing;
    delete cairo;
    delete cape_town;
    delete golden_horseshoe;
    delete hamilton;
    delete hong_kong;
    delete iceland;
    delete interlaken;
    delete london;
    delete moscow;
    delete new_delhi;
    delete new_york;
    delete rio_de_janeiro;
    delete saint_helena;
    delete singapore;
    delete sydney;
    delete tehran;
    delete tokyo;
    delete toronto;
    delete up_arrow;
    delete down_arrow;
    delete left_arrow;
    delete right_arrow;
}

#endif /* UI_INTERFACE_H */

