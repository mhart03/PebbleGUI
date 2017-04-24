/*  
 *  Pebble SmartLock
 *  CET 492 - Senior Project
 *  Dr. Chen
 *  Date: 4-28-17
 *  Authors: Matthew Hart, Ryan Meyer, Josh Lanxner, Aaron Morgan
 *  Description: This code is the interface for the Pebble SmartLock.  From here the user can choose to send different commands.
 *  The commands are sent over to an Arduino teensy over I2C and then sent out over RF from there.  This GUI features a status menu item as 
 *  well that signifies whether a smartstrap is connected or not.
 */

#include "pebble.h"

#define NUM_MENU_SECTIONS 2
#define NUM_FIRST_MENU_ITEMS 2
#define NUM_SECOND_MENU_ITEMS 1

static SmartstrapAttribute *attribute;
static bool s_service_available;
//static bool lastServiceAvailable = 0;    //bool to remember what the last state of the connection status was so we aren;t updating the screen for no reason
// Pointer to the attribute buffer
size_t buff_size;
uint8_t *buffer;
static Window *s_main_window;
static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuItem s_first_menu_items[NUM_FIRST_MENU_ITEMS];
static SimpleMenuItem s_second_menu_items[NUM_SECOND_MENU_ITEMS];
static GBitmap *s_menu_icon_image;
static GBitmap *lock;
static GBitmap *unlock;
static GBitmap *connected;
static GBitmap *disconnected;
static GBitmap *testing;
AppTimer *timer;          // timer to reset the command sent message
const int delta = 2000;  //how long we will deisplay the command sent message

//this happens after a set delta to reset the command sent back to it's origonal state
void timer_callback(void *data) {
  s_first_menu_items[0].subtitle = "Press to send";
  s_first_menu_items[1].subtitle = "Press to send";
  // Redraw this as soon as possible
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}


//LOCK SELECTION CALLBACK
//   Handler for when the "Lock Door" or "Unlock Door" options
//   are selected.
static void lock_selection_callback(int index, void *ctx) {

  SimpleMenuItem *menu_item = &s_first_menu_items[index];

  //If "Lock Door" is selected
  if (index == 0) 
  {
    menu_item->subtitle = "Command sent!";
    timer = app_timer_register(delta, (AppTimerCallback) timer_callback, NULL);
    // Begin the write request, getting the buffer and its length
    smartstrap_attribute_begin_write(attribute, &buffer, &buff_size);
    // Store the data to be written to this attribute
    snprintf((char*)buffer, buff_size, "Lock Door");
    // End the write request, and send the data, not expecting a response
    smartstrap_attribute_end_write(attribute, buff_size, false);
  } 
  //If "Unlock Door" is selected
  else 
  {
    menu_item->subtitle = "Command sent!";
    timer = app_timer_register(delta, (AppTimerCallback) timer_callback, NULL);
    // Begin the write request, getting the buffer and its length
    smartstrap_attribute_begin_write(attribute, &buffer, &buff_size);
    // Store the data to be written to this attribute
    snprintf((char*)buffer, buff_size, "Unlock Door");
    // End the write request, and send the data, not expecting a response
    smartstrap_attribute_end_write(attribute, buff_size, false);
  }
  
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}

static void main_window_load(Window *window) {
  //Create bitmaps of images for application
  lock = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOCK);
  unlock = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UNLOCK);
  connected = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONNECTED);
  disconnected = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECTED);
  testing = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TESTING);

  // Although we already defined NUM_FIRST_MENU_ITEMS, you can define
  // an int as such to easily change the order of menu items later
  int num_a_items = 0;

  s_menu_sections[0] = (SimpleMenuSection) {
    .title = PBL_IF_RECT_ELSE("Pebble SmartLock™", NULL),
    .num_items = NUM_FIRST_MENU_ITEMS,
    .items = s_first_menu_items,
  };
    
  s_first_menu_items[0] = (SimpleMenuItem) {
    .title = "Lock Door",
    .icon = lock,
    .subtitle = "Press to send",
    .callback = lock_selection_callback,
  };
  
  s_first_menu_items[1] = (SimpleMenuItem) {
    .title = "Unlock Door",
    .icon = unlock,
    .subtitle = "Press to send",
    .callback = lock_selection_callback,
  };

  s_menu_sections[1] = (SimpleMenuSection) {
    .title = PBL_IF_RECT_ELSE("Connectivity", NULL),
    .num_items = NUM_SECOND_MENU_ITEMS,
    .items = s_second_menu_items,
  };

  s_second_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Status",
    .subtitle = "Checking...",
    .icon = testing,
  };

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_simple_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, NUM_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
}

void main_window_unload(Window *window) {
  simple_menu_layer_destroy(s_simple_menu_layer);
  gbitmap_destroy(s_menu_icon_image);
}

static void strap_availability_handler(SmartstrapServiceId service_id, bool is_available) {
  // A service's availability has changed
  APP_LOG(APP_LOG_LEVEL_INFO, "Service %d is %s available", (int)service_id, is_available ? "now" : "NOT");

  // Remember if the raw service is available
  s_service_available = (is_available && service_id == SMARTSTRAP_RAW_DATA_SERVICE_ID);
  
  if (is_available) 
  {
    s_second_menu_items[0].subtitle = "Connected!";
    s_second_menu_items[0].icon = connected;
  }
  else 
  {
    s_second_menu_items[0].subtitle = "Disconnected!";
    s_second_menu_items[0].icon = disconnected;
  }
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));

  
}

static void strap_notify_handler(SmartstrapAttribute *attribute) {
  // this will occur if the smartstrap notifies pebble
  //probably wont be used in our case
  vibes_short_pulse();
}

static void strap_write_handler(SmartstrapAttribute *attribute,
                                SmartstrapResult result) {
  // A write operation has been attempted
  if(result != SmartstrapResultOk) {
    // Handle the failure
    //APP_LOG(APP_LOG_LEVEL_ERROR, "Smartstrap error occured: %s", smartstrap_result_to_string(result));
  }
}

static void strap_init() {
  attribute = smartstrap_attribute_create(SMARTSTRAP_RAW_DATA_SERVICE_ID, SMARTSTRAP_RAW_DATA_ATTRIBUTE_ID, 64);

  // Subscribe to smartstrap events
  smartstrap_subscribe((SmartstrapHandlers) {
    .availability_did_change = strap_availability_handler,
    .notified = strap_notify_handler,
    .did_write = strap_write_handler,  
  }); 
  
  attribute = smartstrap_attribute_create(0x1001, 0x1001, 20);
}

static void init() {
  strap_init();
  
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  
}

static void deinit() {
  window_destroy(s_main_window);
  //Cancel timer
  app_timer_cancel(timer);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
