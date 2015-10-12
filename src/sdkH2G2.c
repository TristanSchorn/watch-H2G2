#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

static Window *s_main_window;

static GBitmap *s_bitmap;
static BitmapLayer *s_bitmap_layer;
static GBitmapSequence *s_sequence = NULL;

static void load_sequence();

static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_weekday_layer;
static TextLayer *s_date_layer;

static void update_time(){
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  static char buffer[] = "00:00";  
  if(clock_is_24h_style() == true){
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else{
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }
  text_layer_set_text(s_time_layer, buffer);

  static char dayBuffer[] = "dayLength";
  strftime(dayBuffer, sizeof(dayBuffer), "%A", tick_time);  
  text_layer_set_text(s_weekday_layer, dayBuffer);

  static char dateBuffer[10];
  strftime(dateBuffer, sizeof(dayBuffer), "%b %e", tick_time);
  text_layer_set_text(s_date_layer, dateBuffer);

}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  update_time();

  // Get weather update every 15 minutes
  if(tick_time->tm_min % 15 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}



static void timer_handler(void *context){
  uint32_t next_delay;
  
  if(gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_bitmap, &next_delay)){
    bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));
    
    app_timer_register(next_delay, timer_handler, NULL);//app_timer_register(next_delay, timer_handler, NULL);
  } 
}

static void load_sequence(){

  if(s_sequence){
    gbitmap_sequence_destroy(s_sequence);
    s_sequence = NULL;
  }
  if(s_bitmap){
    gbitmap_destroy(s_bitmap);
    s_bitmap = NULL;
  }

  s_sequence = gbitmap_sequence_create_with_resource(RESOURCE_ID_output);
  s_bitmap = gbitmap_create_blank(gbitmap_sequence_get_bitmap_size(s_sequence), GBitmapFormat8Bit);
  app_timer_register(1, timer_handler, NULL);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  
  //s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_mainImage);
  
  

  s_bitmap_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));
  load_sequence();

  //Time Layer
  s_time_layer = text_layer_create(GRect(0, 19, 100, 100));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorYellow);  
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

  //Weather Layer
  s_weather_layer = text_layer_create(GRect(0, 39, 100, 100));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorYellow);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentRight);
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(s_weather_layer, "DONT PANIC");
      
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));

  //Weekday Layer

  s_weekday_layer = text_layer_create(GRect(0, 59, 100, 100));
  text_layer_set_background_color(s_weekday_layer, GColorClear);
  text_layer_set_text_color(s_weekday_layer, GColorYellow);
  //text_layer_set_text(s_weekday_layer, "Saturday");
  text_layer_set_font(s_weekday_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_weekday_layer, GTextAlignmentRight);
  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weekday_layer));

  //Date Layer
  s_date_layer = text_layer_create(GRect(0, 79, 100, 100));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorYellow);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
      
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));

//old font was ROBOTO_CONDENSED_21
}

static void main_window_unload(Window *window) {
  bitmap_layer_destroy(s_bitmap_layer);
  
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_weekday_layer);
  text_layer_destroy(s_date_layer);
  
  gbitmap_sequence_destroy(s_sequence);
  gbitmap_destroy(s_bitmap);
}

//Weather Callbacks
static void inbox_received_callback(DictionaryIterator *iterator, void *context){
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];

    // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_TEMPERATURE:
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°", (int)t->value->int32);
      break;
    case KEY_CONDITIONS:
      snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }
    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);

    // Look for next item
    t = dict_read_next(iterator);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context){
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context){
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context){
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  s_main_window = window_create();
  //window_set_fullscreen(s_main_window, true);
  window_set_background_color(s_main_window, GColorBlue);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  update_time();

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
