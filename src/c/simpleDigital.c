#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_minutes_layer;
static TextLayer *s_day_layer;
static TextLayer *s_year_layer;
static TextLayer *s_middle_layer;

static Layer *s_battery_layer;
static Layer *s_lower_battery_layer;

static int s_offset = 20;
static int s_battery_level;

int s_time_offset;
int s_day_offset;
int s_year_offset;


static void bluetooth_callback(bool connected) {
    if (!connected) {
        vibes_double_pulse();
    } 
    else {
        vibes_short_pulse();
    }
}

static void battery_callback(BatteryChargeState state) {
    s_battery_level = state.charge_percent;
    layer_mark_dirty(s_battery_layer);
    layer_mark_dirty(s_lower_battery_layer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    int battery_width = (bounds.size.w * s_battery_level) / 100;

    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(0, 0,battery_width, bounds.size.h), 0, GCornerNone);
}

static void lower_battery_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    int battery_width = (bounds.size.w * s_battery_level) / 100;

    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(bounds.size.w - battery_width, 0, bounds.size.w, bounds.size.h), 0, GCornerNone);
}

static void update_time() {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    static char s_time_buffer[3];
    strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H" : "%I", tick_time);
    text_layer_set_text(s_time_layer, s_time_buffer);

    static char s_minutes_buffer[3];
    strftime(s_minutes_buffer, sizeof(s_minutes_buffer), "%M", tick_time);
    text_layer_set_text(s_minutes_layer, s_minutes_buffer);

    static char s_day_buffer[6];
    strftime(s_day_buffer, sizeof(s_day_buffer), "%d.%m", tick_time);
    text_layer_set_text(s_day_layer, s_day_buffer);

    static char s_year_buffer[5];
    strftime(s_year_buffer, sizeof(s_year_buffer), "%Y", tick_time);
    text_layer_set_text(s_year_layer, s_year_buffer);
}

static void time_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
}

static void setup_text_layer(TextLayer *layer, GFont font) {
    text_layer_set_background_color(layer, GColorClear);
    text_layer_set_text_color(layer, GColorWhite);
    text_layer_set_font(layer, font);
    text_layer_set_text_alignment(layer, GTextAlignmentCenter);
}

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_time_offset = bounds.size.h / 2 - 28;
    s_day_offset = -7 + s_offset;
    s_year_offset = bounds.size.h - 30 - s_offset;

    s_day_layer = text_layer_create(GRect(0, s_day_offset, bounds.size.w, 30));
    s_time_layer = text_layer_create(GRect(0, s_time_offset, bounds.size.w / 2, 42));
    s_minutes_layer = text_layer_create(GRect(bounds.size.w / 2, s_time_offset, bounds.size.w / 2, 42));
    s_year_layer = text_layer_create(GRect(0, s_year_offset, bounds.size.w, 30));
    s_middle_layer = text_layer_create(GRect(0, s_time_offset, bounds.size.w, 42));

    s_battery_layer = layer_create(GRect(0, s_time_offset, bounds.size.w, 2));
    layer_set_update_proc(s_battery_layer, battery_update_proc);

    s_lower_battery_layer = layer_create(GRect(0, bounds.size.h - s_time_offset, bounds.size.w, 2));
    layer_set_update_proc(s_lower_battery_layer, lower_battery_update_proc);

    setup_text_layer(s_day_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
    setup_text_layer(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD)); 
    setup_text_layer(s_minutes_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD)); 
    setup_text_layer(s_year_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
    setup_text_layer(s_middle_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));

    text_layer_set_text(s_middle_layer, ":");
    text_layer_set_text_color(s_minutes_layer, PBL_IF_BW_ELSE(GColorWhite ,GColorRed));

    layer_add_child(window_layer, text_layer_get_layer(s_day_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_year_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_minutes_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_middle_layer));
    layer_add_child(window_layer, s_battery_layer);
    layer_add_child(window_layer, s_lower_battery_layer);
}

static void main_window_unload(Window *window) {
    text_layer_destroy(s_day_layer);
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_minutes_layer);
    text_layer_destroy(s_year_layer);
    layer_destroy(s_battery_layer);
    layer_destroy(s_lower_battery_layer);
}

static void init() {
    s_main_window = window_create();

    window_set_window_handlers(s_main_window, (WindowHandlers){
        .load = main_window_load,
        .unload = main_window_unload
    });

    connection_service_subscribe((ConnectionHandlers) {
        .pebble_app_connection_handler = bluetooth_callback
    });

    window_set_background_color(s_main_window, GColorBlack);

    tick_timer_service_subscribe(MINUTE_UNIT, time_handler);
    battery_state_service_subscribe(battery_callback);

    window_stack_push(s_main_window, true);

    battery_callback(battery_state_service_peek());

    update_time();
}

static void deinit() {
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
    return 0;
}