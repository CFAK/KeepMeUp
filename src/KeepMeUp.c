#include <pebble.h>

#define DISPLAY_WIDTH  144
#define DISPLAY_HEIGHT 168
#define WORKER_TICKS 0

#define REPEAT_INTERVAL_MS 50
#define NUM_MINUTES_PKEY 1

#define NUM_MENU_SECTIONS 1
#define NUM_FIRST_MENU_ITEMS 2
#define NUM_SECOND_MENU_ITEMS 3
#define NUM_THIRD_MENU_ITEMS 3
#define NUM_FOURTH_MENU_ITEMS 2
#define NUM_MINUTES_DEFAULT 1

static Window *s_main_window;
static TextLayer *s_output_layer, *s_ticks_layer;

static int s_minutes = NUM_MINUTES_DEFAULT;
static int vibration_type = 1;
static bool flash_on = true;
static bool alarm_on_master = true;
static Window *s_main_window;
static Window *KMU_window;
static Window *settings_menu;
static Window *counter_menu;
static Window *vibration_menu;
static Window *flash_menu;
static ActionBarLayer *s_action_bar;
static TextLayer *s_header_layer, *s_body_layer, *s_label_layer;
static GBitmap *s_icon_plus, *s_icon_minus;
TextLayer *top_text_layer;
TextLayer *bottom_text_layer;
TextLayer *settings_text_layer;
static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuLayer *s_settings_menu_layer;
static SimpleMenuLayer *s_vibration_menu_layer;
static SimpleMenuLayer *s_flash_menu_layer;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuSection s_menu_sections2[NUM_MENU_SECTIONS];
static SimpleMenuSection s_menu_sections3[NUM_MENU_SECTIONS];
static SimpleMenuSection s_menu_sections4[NUM_MENU_SECTIONS];
static SimpleMenuItem s_first_menu_items[NUM_FIRST_MENU_ITEMS];
static SimpleMenuItem s_second_menu_items[NUM_SECOND_MENU_ITEMS];
static SimpleMenuItem s_third_menu_items[NUM_THIRD_MENU_ITEMS];
static SimpleMenuItem s_fourth_menu_items[NUM_FOURTH_MENU_ITEMS];


// static bool s_special_flag = false;
// static int s_hit_count = 0;

static void flash(){
  if(flash_on){
    light_enable(false);
    psleep(500);
    light_enable(true);
    psleep(500);
    light_enable(false);
    psleep(500);
    light_enable(true);
    psleep(500);
    light_enable(false);
  }
}

static void vibeHandler(int type){
    if (type == 1){
        vibes_long_pulse();
    } else if (type == 2){
      vibes_short_pulse();
    } else if (type == 3){
      vibes_double_pulse();
    }
    flash();
}


static void update_text() {
  static char s_body_text[18];
  snprintf(s_body_text, sizeof(s_body_text), "%u Minutes", s_minutes);
  text_layer_set_text(s_body_layer, s_body_text);
}

static void worker_message_handler(uint16_t type, AppWorkerMessage *data) {
  if(type == WORKER_TICKS) {
    // Read ticks from worker's packet
    int ticks = data->data0;
    if(ticks > 2 && ticks % (s_minutes*60) == 0){
      for(int i=0;i<3;i++){
        vibeHandler(vibration_type);
      }
    }
    // Show to user in TextLayer
    static char s_buffer[32];
    snprintf(s_buffer, sizeof(s_buffer), "%d background ticks", ticks);
    text_layer_set_text(s_ticks_layer, s_buffer);
  }
}

static void interrupt_click_handler(ClickRecognizerRef recognizer, void *context) {
  bool alarm_on_master = false;
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Check to see if the worker is currently active
  bool running = app_worker_is_running();
  // Toggle running state
  AppWorkerResult result;
  if(running) {
    result = app_worker_kill();

    if(result == APP_WORKER_RESULT_SUCCESS) {
      text_layer_set_text(s_ticks_layer, "Worker stopped!");
    } else {
      text_layer_set_text(s_ticks_layer, "Error killing worker!");
    }
  } else {
    result = app_worker_launch();

    if(result == APP_WORKER_RESULT_SUCCESS) {
      text_layer_set_text(s_ticks_layer, "Worker launched!");

    } else {
      text_layer_set_text(s_ticks_layer, "Error launching worker!");
    }
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "Result: %d", result);
}



static void increment_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_minutes >= 60) {
    // Max at 60 mins
    return;
  }
  s_minutes++;
  update_text();
}

static void decrement_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_minutes <= 0) {
    // Keep the counter at zero
    return;
  }

  s_minutes--;
  update_text();
}


static void click_config_provider(void *context) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, REPEAT_INTERVAL_MS, increment_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, REPEAT_INTERVAL_MS, decrement_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void click_interrurpt_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, interrupt_click_handler);
}

static void menu_select_callback(int index, void *ctx) {
  if(index == 0){
    window_stack_push(KMU_window, true);
  } else if (index == 1){
    window_stack_push(settings_menu, true);
  }
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}

static void settings_select_callback(int index, void *ctx) {
  if(index == 0){
    window_stack_push(counter_menu, true);
  }
  else if (index == 1){
    window_stack_push(vibration_menu, true);
  }
  else if( index == 2){
    window_stack_push(flash_menu,true);
  }

  layer_mark_dirty(simple_menu_layer_get_layer(s_settings_menu_layer));
}

static void vibration_select_callback(int index, void *ctx) {
  vibration_type = index+1;
}

static void flash_select_callback(int index, void *ctx) {
  if(index == 0){
    flash_on = true;
  } else {
    flash_on = false;
  }
}

static void main_window_load(Window *window) {

  // Although we already defined NUM_FIRST_MENU_ITEMS, you can define
  // an int as such to easily change the order of menu items later
  int num_a_items = 0;

  s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Start",
    .subtitle = "Wake Me Up!",
    .callback = menu_select_callback,
  };
  s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Settings",
    .callback = menu_select_callback,
  };

  s_menu_sections[0] = (SimpleMenuSection) {
    .num_items = NUM_FIRST_MENU_ITEMS,
    .items = s_first_menu_items,
  };

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_simple_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, NUM_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
}

static void KMU_window_load(Window *window){

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  const int inset = 8;


  // Create UI
  s_output_layer = text_layer_create(bounds);
  text_layer_set_text(s_output_layer, "Use SELECT to start/stop the background worker.");
  text_layer_set_text_alignment(s_output_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer));
#ifdef PBL_ROUND
  text_layer_enable_screen_text_flow_and_paging(s_output_layer, inset);
#endif

  s_ticks_layer = text_layer_create(GRect(PBL_IF_RECT_ELSE(5, 0), 135, bounds.size.w, 30));
  text_layer_set_text(s_ticks_layer, "No data yet.");
  text_layer_set_text_alignment(s_ticks_layer, PBL_IF_RECT_ELSE(GTextAlignmentLeft,
                                                                GTextAlignmentCenter));
  layer_add_child(window_layer, text_layer_get_layer(s_ticks_layer));
#ifdef PBL_ROUND
  text_layer_enable_screen_text_flow_and_paging(s_ticks_layer, inset);
#endif

}

static void settings_menu_load(Window *window){

  int num_b_items = 0;

  s_second_menu_items[num_b_items++] = (SimpleMenuItem) {
    .title = "Time Interval",
    .subtitle = "Change timer!",
    .callback = settings_select_callback,
  };
  s_second_menu_items[num_b_items++] = (SimpleMenuItem) {
    .title = "Vibration Pattern",
    .subtitle = "Alter vibrations",
    .callback = settings_select_callback,
  };
  s_second_menu_items[num_b_items++] = (SimpleMenuItem) {
    .title = "Screen Flash",
    .subtitle = "Flash Backlight",
    .callback = settings_select_callback,
  };

  s_menu_sections2[0] = (SimpleMenuSection) {
    .num_items = NUM_SECOND_MENU_ITEMS,
    .items = s_second_menu_items,
  };

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_settings_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections2, NUM_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(s_settings_menu_layer));
}

static void flash_menu_load(Window *window){

  int num_d_items = 0;

  s_fourth_menu_items[num_d_items++] = (SimpleMenuItem) {
    .title = "Screen Flash On",
    .callback = flash_select_callback,
  };
  s_fourth_menu_items[num_d_items++] = (SimpleMenuItem) {
    .title = "Screen Flash Off",
    .callback = flash_select_callback,
  };

  s_menu_sections4[0] = (SimpleMenuSection) {
    .num_items = NUM_FOURTH_MENU_ITEMS,
    .items = s_fourth_menu_items,
  };

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_settings_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections4, NUM_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(s_settings_menu_layer));
}

static void counter_menu_load(Window *window) {

  Layer *window_layer = window_get_root_layer(window);

  s_action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(s_action_bar, window);
  action_bar_layer_set_click_config_provider(s_action_bar, click_config_provider);

  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_plus);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_minus);

  int width = layer_get_frame(window_layer).size.w - ACTION_BAR_WIDTH - 3;

  s_header_layer = text_layer_create(GRect(4, PBL_IF_RECT_ELSE(0, 30), width, 60));
  text_layer_set_font(s_header_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_background_color(s_header_layer, GColorClear);
  text_layer_set_text(s_header_layer, "How many Minutes?");
  text_layer_set_text_alignment(s_header_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft));
  layer_add_child(window_layer, text_layer_get_layer(s_header_layer));

  s_body_layer = text_layer_create(GRect(4, PBL_IF_RECT_ELSE(44, 60), width, 60));
  text_layer_set_font(s_body_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(s_body_layer, GColorClear);
  text_layer_set_text_alignment(s_body_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft));
  layer_add_child(window_layer, text_layer_get_layer(s_body_layer));

  s_label_layer = text_layer_create(GRect(4, PBL_IF_RECT_ELSE(44, 60) + 28, width, 60));
  text_layer_set_font(s_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(s_label_layer, GColorClear);
  text_layer_set_text(s_label_layer, "In between alerts");
  text_layer_set_text_alignment(s_label_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft));
  layer_add_child(window_layer, text_layer_get_layer(s_label_layer));

  update_text();
}

static void vibration_menu_load(Window *window){

  int num_c_items = 0;

  s_third_menu_items[num_c_items++] = (SimpleMenuItem) {
    .title = "Long Vibrate",
    .subtitle = "1 long vibration.",
    .callback = settings_select_callback,
  };
  s_third_menu_items[num_c_items++] = (SimpleMenuItem) {
    .title = "Short Vibrate",
    .subtitle = "1 short vibration.",
    .callback = settings_select_callback,
  };
  s_third_menu_items[num_c_items++] = (SimpleMenuItem) {
    .title = "Burst Vibration",
    .subtitle = "2 brief vibrations.",
    .callback = settings_select_callback,
  };

  s_menu_sections3[0] = (SimpleMenuSection) {
    .num_items = NUM_THIRD_MENU_ITEMS,
    .items = s_third_menu_items,
  };

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_settings_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections3, NUM_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(s_settings_menu_layer));
}

void KMU_window_unload(Window *window) {
  text_layer_destroy(s_output_layer);
  text_layer_destroy(s_ticks_layer);
}

void settings_menu_unload(Window *window) {
  simple_menu_layer_destroy(s_settings_menu_layer);
}

void flash_menu_unload(Window *window) {
  simple_menu_layer_destroy(s_flash_menu_layer);
}

void vibration_menu_unload(Window *window){
  simple_menu_layer_destroy(s_vibration_menu_layer);
}

void counter_menu_unload(Window *window) {
  text_layer_destroy(s_header_layer);
  text_layer_destroy(s_body_layer);
  text_layer_destroy(s_label_layer);
  action_bar_layer_destroy(s_action_bar);
}

void main_window_unload(Window *window){
  simple_menu_layer_destroy(s_simple_menu_layer);
}

static void init(){

  s_icon_plus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_PLUS);
  s_icon_minus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_MINUS);

  s_minutes = persist_exists(NUM_MINUTES_PKEY) ? persist_read_int(NUM_MINUTES_PKEY) : NUM_MINUTES_DEFAULT;

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });

  //Keep me up Window
  KMU_window =  window_create();
  window_set_click_config_provider(KMU_window, click_config_provider);
  window_set_window_handlers(KMU_window,(WindowHandlers){
    .load = KMU_window_load,
    .unload = KMU_window_unload,
  });

  settings_menu = window_create();
  window_set_window_handlers(settings_menu,(WindowHandlers){
    .load = settings_menu_load,
    .unload = settings_menu_unload,
  });

  counter_menu = window_create();
  window_set_click_config_provider(counter_menu, click_config_provider);
  window_set_window_handlers(counter_menu,(WindowHandlers){
    .load = counter_menu_load,
    .unload = counter_menu_unload,
  });

  vibration_menu = window_create();
  window_set_click_config_provider(vibration_menu, click_config_provider);
  window_set_window_handlers(vibration_menu,(WindowHandlers){
    .load = vibration_menu_load,
    .unload = vibration_menu_unload,
  });

  flash_menu = window_create();
  window_set_click_config_provider(flash_menu, click_config_provider);
  window_set_window_handlers(flash_menu,(WindowHandlers){
    .load = flash_menu_load,
    .unload = flash_menu_unload,
  });


  window_stack_push(s_main_window, true);
  app_worker_message_subscribe(worker_message_handler);
}

static void deinit() {

  persist_write_int(NUM_MINUTES_PKEY, s_minutes);

  window_destroy(s_main_window);
  window_destroy(KMU_window);
  window_destroy(settings_menu);
  window_destroy(counter_menu);
  window_destroy(vibration_menu);
  window_destroy(flash_menu);

  app_worker_message_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
