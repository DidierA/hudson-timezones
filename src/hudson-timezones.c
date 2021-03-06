/*
 * Three time zone clock;
 *
 * When it is day, the text and background are white.
 * When it is night, they are black.
 *
 * Rather than use text layers, it draws the entire frame once per minute.
 *
 * Inspired by a design on RichardG's site that I can't find again to
 * credit the designer.
 *
 * adapted to Pebble SDK 2.0 and 4 time zones by Didier Arenzana
 */

//    Copyright (C) 2013  Trammell Hudson
//    Copyright (C) 2014  Didier Arenzana
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with this program; if not, write to the Free Software Foundation, Inc.,
//    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include <pebble.h>

// this struct will be attached to each layer, and contains data to display
typedef struct {
    char *name;
    char time[9];
    int night_time;
} city_time;

typedef struct
{
    city_time  city;
    Layer *layer;
	int offset;
} timezone_t;

// Local timezone GMT offset
static const int gmt_offset = +1 * 60;

#define NUM_TIMEZONES 4
// for 3 cities
// #define CITY_FONT RESOURCE_ID_FONT_ARIAL_16
// #define CLOCK_FONT RESOURCE_ID_FONT_ARIAL_BLACK_30

//for 4 cities
#define CITY_FONT RESOURCE_ID_FONT_ARIAL_11
#define CLOCK_FONT RESOURCE_ID_FONT_ARIAL_BLACK_26

// this should be in pebble.h ...
#define layer_set_data(layer, type, data) *(type *)layer_get_data(layer)=data

static timezone_t timezones[NUM_TIMEZONES] =
{
	{ .city={.name = "New York"},   .offset = -5 * 60 },
	{ .city={.name = "London"},     .offset = +0 * 60 },
    { .city={.name = "Mumbai"},     .offset = +5 * 60 +30 },
	{ .city={.name = "Singapore"},  .offset = +8 * 60 },
};

static Window *window;
static GFont font_thin, font_thick;

static void
timezone_layer_update(
                      Layer * const me,
                      GContext * ctx
                      )
{
    city_time *city=*(city_time **)layer_get_data(me) ;
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "layer_update: Layer %p, city %p", me, city);
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "layer_update: %10s %s %s", city->name, city->time, city->night_time ? "(night)" : "(day)");

    GRect bounds=layer_get_bounds(me) ;
	const int16_t w = bounds.size.w;
	const int16_t h = bounds.size.h;
    
	// it is night there, draw in black video
	graphics_context_set_fill_color(ctx, city->night_time ? GColorBlack : GColorWhite);
	graphics_context_set_text_color(ctx,!city->night_time ? GColorBlack : GColorWhite);
	graphics_fill_rect(ctx, GRect(0, 0, w, h), 0, 0);

	graphics_draw_text(ctx,
                       city->name,
                       font_thin,
                       GRect(0, 0, w, h/3),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
                       );

	graphics_draw_text(ctx,
                       city->time,
                       font_thick,
                       GRect(0, h/3, w, 2*h/3),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
                       );
}

static void update_time(struct tm *now, int offset, city_time *city) {
    
    // convert to wanted timezone
    struct tm here=*now ;
    
	here.tm_min += (offset - gmt_offset) % 60;
	if (here.tm_min >= 60) {
		here.tm_hour++;
		here.tm_min -= 60;
	}
    if (here.tm_min < 0) {
        here.tm_hour--;
        here.tm_min += 60;
    }
    
	here.tm_hour += (offset - gmt_offset) / 60;
	if (here.tm_hour > 23)
		here.tm_hour -= 24;
	if (here.tm_hour < 0)
		here.tm_hour += 24;
    
    // format for display
    strftime(city->time, sizeof(city->time),
             clock_is_24h_style() ? "%H:%M" : "%I:%M",
             &here);
    
    // it is night between 7:00pm and 6:59am,
    // so there is no ambiguity when using 12h format
	city->night_time = (here.tm_hour > 18 || here.tm_hour < 7);

}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
	(void) units_changed;
    
	for (int i = 0 ; i < NUM_TIMEZONES ; i++) {
        update_time(tick_time, timezones[i].offset, &(timezones[i].city));
		layer_mark_dirty(timezones[i].layer);
    }
}

static void init(void) {
    
    time_t time_now = time(NULL);
    struct tm *now=localtime(&time_now);
    
    window = window_create();
    Layer *window_layer=window_get_root_layer(window) ;

    window_set_background_color(window, GColorBlack);
    window_stack_push(window, true);
    
    GRect bounds = layer_get_bounds(window_layer);
    int screen_width  = bounds.size.w;
    int screen_height = bounds.size.h;
    int layer_height  = screen_height / NUM_TIMEZONES ;
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "Screen dimensions: %dx%d", screen_width, screen_height);
    
	font_thin = fonts_load_custom_font(resource_get_handle(CITY_FONT));
	font_thick = fonts_load_custom_font(resource_get_handle(CLOCK_FONT));
    
	for (int i = 0 ; i < NUM_TIMEZONES ; i++)
	{
        Layer *layer=layer_create_with_data(
                   GRect(0, i * layer_height, screen_width, layer_height),
                   sizeof(&(timezones[i].city))
                   );

        update_time(now, timezones[i].offset, &(timezones[i].city));
        // APP_LOG(APP_LOG_LEVEL_DEBUG, "City %d - %p", i, &(timezones[i].city));
        /* APP_LOG(APP_LOG_LEVEL_DEBUG,
                "Name='%s', time='%s' (%s), offset=%d",
                timezones[i].city.name,
                timezones[i].city.time,
                timezones[i].city.night_time ? "night" : "day",
                timezones[i].offset); */
        layer_set_data(layer, city_time *, &(timezones[i].city));
        layer_set_update_proc(layer, timezone_layer_update);
		layer_add_child(window_layer, layer);
		layer_mark_dirty(layer);
        timezones[i].layer=layer;
	}

}

static void deinit(void) {
    window_destroy(window);
    for (int i=0; i<NUM_TIMEZONES; i++) {
        layer_destroy(timezones[i].layer) ;
    }
    fonts_unload_custom_font(font_thin) ;
    fonts_unload_custom_font(font_thick) ;
}

int main(void) {
    init();
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
    app_event_loop();
    deinit();
}
