/*
Copyright (C) 2014 Mark Reed

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "pebble.h"

static Window *window;
static Layer *window_layer;

GColor background_color = GColorBlack;

static AppSync sync;
static uint8_t sync_buffer[64];

static bool appStarted = false;

static int invert;
static int hidesec;
static int bluetoothvibe;
static int hourlyvibe;

enum WeatherKey {
  INVERT_COLOR_KEY = 0x0,
  HIDE_SEC_KEY = 0x1,
  HOURLYVIBE_KEY = 0x2,
  BLUETOOTHVIBE_KEY = 0x3
};

//static GBitmap *time_format_image;
//static BitmapLayer *time_format_layer;

//static GBitmap *battery_image;
//static BitmapLayer *battery_image_layer;
//static BitmapLayer *battery_layer;

static GBitmap *separator_image;
static BitmapLayer *separator_layer;

static GBitmap *day_name_image;
static BitmapLayer *day_name_layer;

const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DAY_NAME_SUN,
  RESOURCE_ID_IMAGE_DAY_NAME_MON,
  RESOURCE_ID_IMAGE_DAY_NAME_TUE,
  RESOURCE_ID_IMAGE_DAY_NAME_WED,
  RESOURCE_ID_IMAGE_DAY_NAME_THU,
  RESOURCE_ID_IMAGE_DAY_NAME_FRI,
  RESOURCE_ID_IMAGE_DAY_NAME_SAT
};

#define TOTAL_MAIN_DIGITS 4
static GBitmap *time_digits_images[TOTAL_MAIN_DIGITS];
static BitmapLayer *time_digits_layers[TOTAL_MAIN_DIGITS];

const int MAIN_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_0,
  RESOURCE_ID_IMAGE_NUM_1,
  RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3,
  RESOURCE_ID_IMAGE_NUM_4,
  RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6,
  RESOURCE_ID_IMAGE_NUM_7,
  RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};

#define TOTAL_SECONDS_DIGITS 2
static GBitmap *seconds_digits_images[TOTAL_SECONDS_DIGITS];
static BitmapLayer *seconds_digits_layers[TOTAL_SECONDS_DIGITS];

#define TOTAL_BATTERY_PERCENT_DIGITS 3
static GBitmap *battery_percent_images[TOTAL_BATTERY_PERCENT_DIGITS];
static BitmapLayer *battery_percent_layers[TOTAL_BATTERY_PERCENT_DIGITS];

const int BATT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_BATTNUM_0,
  RESOURCE_ID_IMAGE_BATTNUM_1,
  RESOURCE_ID_IMAGE_BATTNUM_2,
  RESOURCE_ID_IMAGE_BATTNUM_3,
  RESOURCE_ID_IMAGE_BATTNUM_4,
  RESOURCE_ID_IMAGE_BATTNUM_5,
  RESOURCE_ID_IMAGE_BATTNUM_6,
  RESOURCE_ID_IMAGE_BATTNUM_7,
  RESOURCE_ID_IMAGE_BATTNUM_8,
  RESOURCE_ID_IMAGE_BATTNUM_9,
  RESOURCE_ID_IMAGE_BATTNUM_10
};

#define TOTAL_DATE_DIGITS 2	
static GBitmap *date_digits_images[TOTAL_DATE_DIGITS];
static BitmapLayer *date_digits_layers[TOTAL_DATE_DIGITS];

const int DATE_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_DATE0,
  RESOURCE_ID_IMAGE_NUM_DATE1,
  RESOURCE_ID_IMAGE_NUM_DATE2,
  RESOURCE_ID_IMAGE_NUM_DATE3,
  RESOURCE_ID_IMAGE_NUM_DATE4,
  RESOURCE_ID_IMAGE_NUM_DATE5,
  RESOURCE_ID_IMAGE_NUM_DATE6,
  RESOURCE_ID_IMAGE_NUM_DATE7,
  RESOURCE_ID_IMAGE_NUM_DATE8,
  RESOURCE_ID_IMAGE_NUM_DATE9
};

BitmapLayer *layer_batt_img;
GBitmap *digds_batt_100;
GBitmap *digds_batt_90;
GBitmap *digds_batt_80;
GBitmap *digds_batt_70;
GBitmap *digds_batt_60;
GBitmap *digds_batt_50;
GBitmap *digds_batt_40;
GBitmap *digds_batt_30;
GBitmap *digds_batt_20;
GBitmap *digds_batt_10;
int charge_percent = 0;
InverterLayer *inverter_layer = NULL;


void set_invert_color(bool invert) {
  if (invert && inverter_layer == NULL) {
    // Add inverter layer
    inverter_layer = inverter_layer_create(GRect(0, 0, 144, 168));
    layer_add_child(window_layer, inverter_layer_get_layer(inverter_layer));
  } else if (!invert && inverter_layer != NULL) {
    // Remove Inverter layer
    layer_remove_from_parent(inverter_layer_get_layer(inverter_layer));
    inverter_layer_destroy(inverter_layer);
    inverter_layer = NULL;
  }
  // No action required
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed);

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
   
	switch (key) {

	case INVERT_COLOR_KEY:
      invert = new_tuple->value->uint8 != 0;
	  persist_write_bool(INVERT_COLOR_KEY, invert);
      set_invert_color(invert);
		
		
      break;
		
	case HIDE_SEC_KEY:
      hidesec = new_tuple->value->uint8 != 0;
	  persist_write_bool(HIDE_SEC_KEY, hidesec);	  
   if(hidesec) {
	for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
    layer_set_hidden(bitmap_layer_get_layer(seconds_digits_layers[i]), true);
			}
    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
	   
      }  else {
	for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
    layer_set_hidden(bitmap_layer_get_layer(seconds_digits_layers[i]), false);
			}
    tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
      }
		break; 

    case BLUETOOTHVIBE_KEY:
      bluetoothvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(BLUETOOTHVIBE_KEY, bluetoothvibe);
      break; 
		
    case HOURLYVIBE_KEY:
      hourlyvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(HOURLYVIBE_KEY, hourlyvibe);	  
      break;
  }
}

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
  GBitmap *old_image = *bmp_image;
  *bmp_image = gbitmap_create_with_resource(resource_id);
  GRect frame = (GRect) {
    .origin = origin,
    .size = (*bmp_image)->bounds.size
  };
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
  layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);
  if (old_image != NULL) {
	gbitmap_destroy(old_image);
	old_image = NULL;
  }
}

void update_battery(BatteryChargeState charge_state) {

        if (charge_state.charge_percent <= 10) {
			bitmap_layer_set_bitmap(layer_batt_img, digds_batt_10);
        } else if (charge_state.charge_percent <= 20) {
			bitmap_layer_set_bitmap(layer_batt_img, digds_batt_20);
        } else if (charge_state.charge_percent <= 30) {
            bitmap_layer_set_bitmap(layer_batt_img, digds_batt_30);
		} else if (charge_state.charge_percent <= 40) {
           bitmap_layer_set_bitmap(layer_batt_img, digds_batt_40);
		} else if (charge_state.charge_percent <= 50) {
           bitmap_layer_set_bitmap(layer_batt_img, digds_batt_50);
    	} else if (charge_state.charge_percent <= 60) {
           bitmap_layer_set_bitmap(layer_batt_img, digds_batt_60);	
        } else if (charge_state.charge_percent <= 70) {
           bitmap_layer_set_bitmap(layer_batt_img, digds_batt_70);
		} else if (charge_state.charge_percent <= 80) {
           bitmap_layer_set_bitmap(layer_batt_img, digds_batt_80);
		} else if (charge_state.charge_percent <= 90) {
           bitmap_layer_set_bitmap(layer_batt_img, digds_batt_90);
		} else if (charge_state.charge_percent <= 100) {
           bitmap_layer_set_bitmap(layer_batt_img, digds_batt_100);					
    }
    charge_percent = charge_state.charge_percent;

  set_container_image(&battery_percent_images[0], battery_percent_layers[0], BATT_IMAGE_RESOURCE_IDS[charge_state.charge_percent/10], GPoint( 8, 27));
  set_container_image(&battery_percent_images[1], battery_percent_layers[1], BATT_IMAGE_RESOURCE_IDS[charge_state.charge_percent%10], GPoint( 18, 27));
  set_container_image(&battery_percent_images[2], battery_percent_layers[2], BATT_IMAGE_RESOURCE_IDS[10], GPoint( 28, 27));

}

static void toggle_bluetooth_icon() {
  if (appStarted && bluetoothvibe ) {
        vibes_double_pulse();
	}
}

void bluetooth_connection_callback(bool connected) {
  toggle_bluetooth_icon();
}

void force_update(void) {
    update_battery(battery_state_service_peek());
    bluetooth_connection_service_peek();
}

unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }
  unsigned short display_hour = hour % 12;
  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}

static void update_days(struct tm *tick_time) {
  set_container_image(&day_name_image, day_name_layer, DAY_NAME_IMAGE_RESOURCE_IDS[tick_time->tm_wday], GPoint(8, 122));
  set_container_image(&date_digits_images[0], date_digits_layers[0], DATE_IMAGE_RESOURCE_IDS[tick_time->tm_mday/10], GPoint(29, 122));
  set_container_image(&date_digits_images[1], date_digits_layers[1], DATE_IMAGE_RESOURCE_IDS[tick_time->tm_mday%10], GPoint(39, 122));
}

static void update_hours(struct tm *tick_time) {

  if(appStarted && hourlyvibe) {
    //vibe!
    vibes_short_pulse();
  }
  
  unsigned short display_hour = get_display_hour(tick_time->tm_hour);

  set_container_image(&time_digits_images[0], time_digits_layers[0], MAIN_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(6, 40));
  set_container_image(&time_digits_images[1], time_digits_layers[1], MAIN_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(36, 40));
/*
 if (!clock_is_24h_style()) {
    if (tick_time->tm_hour >= 12) {
      set_container_image(&time_format_image, time_format_layer, RESOURCE_ID_IMAGE_MODE_PM, GPoint(8, 27));
      layer_set_hidden(bitmap_layer_get_layer(time_format_layer), false);
    } 
    else {
       set_container_image(&time_format_image, time_format_layer, RESOURCE_ID_IMAGE_MODE_AM, GPoint(8, 27));
    }
 }*/
}   

static void update_minutes(struct tm *tick_time) {
  set_container_image(&time_digits_images[2], time_digits_layers[2], MAIN_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(78, 40));
  set_container_image(&time_digits_images[3], time_digits_layers[3], MAIN_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(108, 40));	
}

static void update_seconds(struct tm *tick_time) {
  set_container_image(&seconds_digits_images[0], seconds_digits_layers[0], DATE_IMAGE_RESOURCE_IDS[tick_time->tm_sec/10], GPoint(118, 27));
  set_container_image(&seconds_digits_images[1], seconds_digits_layers[1], DATE_IMAGE_RESOURCE_IDS[tick_time->tm_sec%10], GPoint(128, 27));
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
	
  if (units_changed & DAY_UNIT) {
    update_days(tick_time);
  }
  if (units_changed & HOUR_UNIT) {
    update_hours(tick_time);
  }
  if (units_changed & MINUTE_UNIT) {
   update_minutes(tick_time);
  }	
  if (units_changed & SECOND_UNIT) {
    update_seconds(tick_time);
  }		
}

static void init(void) {
  memset(&time_digits_layers, 0, sizeof(time_digits_layers));
  memset(&time_digits_images, 0, sizeof(time_digits_images));

  memset(&date_digits_layers, 0, sizeof(date_digits_layers));
  memset(&date_digits_images, 0, sizeof(date_digits_images));
	
  memset(&battery_percent_layers, 0, sizeof(battery_percent_layers));
  memset(&battery_percent_images, 0, sizeof(battery_percent_images));

  const int inbound_size = 64;
  const int outbound_size = 64;
  app_message_open(inbound_size, outbound_size);  

  window = window_create();
  if (window == NULL) {
      return;
  }
  window_stack_push(window, true /* Animated */);
  window_layer = window_get_root_layer(window);
  
  background_color  = GColorBlack;
  window_set_background_color(window, background_color);
	
 // resources
	
    digds_batt_100   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_090_100);
    digds_batt_90   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_080_090);
    digds_batt_80   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_070_080);
    digds_batt_70   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_060_070);
    digds_batt_60   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_050_060);
    digds_batt_50   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_040_050);
    digds_batt_40   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_030_040);
    digds_batt_30    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_020_030);
    digds_batt_20    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_010_020);
	digds_batt_10    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_000_010);

  layer_batt_img  = bitmap_layer_create(GRect(6, 40, 132, 79));
  bitmap_layer_set_bitmap(layer_batt_img, digds_batt_100);
  layer_add_child(window_layer, bitmap_layer_get_layer(layer_batt_img));		

	
  separator_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SEPARATOR);
  GRect frame_sep = (GRect) {
    .origin = { .x = 65, .y = 40 },
    .size = separator_image->bounds.size
  };
  separator_layer = bitmap_layer_create(frame_sep);
  bitmap_layer_set_bitmap(separator_layer, separator_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(separator_layer)); 

 /*    time_format_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MODE_24H);
 GRect frame_24 = (GRect) {
    .origin = { .x = 8, .y = 27 },
   .size = time_format_image->bounds.size
  };
  time_format_layer = bitmap_layer_create(frame_24);
  if (clock_is_24h_style()) {
    time_format_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MODE_24H);
    bitmap_layer_set_bitmap(time_format_layer, time_format_image); 
  }
	layer_add_child(window_layer, bitmap_layer_get_layer(time_format_layer));	

	
 battery_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
  GRect frame4 = (GRect) {
    .origin = { .x = 118, .y = 122 },
    .size = battery_image->bounds.size
  };
  battery_layer = bitmap_layer_create(frame4);
  battery_image_layer = bitmap_layer_create(frame4);
  bitmap_layer_set_bitmap(battery_image_layer, battery_image);
  
  layer_add_child(window_layer, bitmap_layer_get_layer(battery_image_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(battery_layer));
	*/
	
  // Create time and date layers
  GRect dummy_frame = { {0, 0}, {0, 0} };
	
  day_name_layer = bitmap_layer_create(dummy_frame);
  layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer));

	
  for (int i = 0; i < TOTAL_MAIN_DIGITS; ++i) {
    time_digits_layers[i] = bitmap_layer_create(dummy_frame);
	GCompOp compositing_mode = GCompOpAnd;
    bitmap_layer_set_compositing_mode(time_digits_layers[i], compositing_mode);
    layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers[i]));
  }
	
  for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
    seconds_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(seconds_digits_layers[i]));
  }
	
  for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
    date_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layers[i]));
  }
	
  for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
    battery_percent_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(battery_percent_layers[i]));
  }		
	
  Tuplet initial_values[] = {
    TupletInteger(INVERT_COLOR_KEY, persist_read_bool(INVERT_COLOR_KEY)),
    TupletInteger(HIDE_SEC_KEY, persist_read_bool(HIDE_SEC_KEY)),
    TupletInteger(HOURLYVIBE_KEY, persist_read_bool(HOURLYVIBE_KEY)),
    TupletInteger(BLUETOOTHVIBE_KEY, persist_read_bool(BLUETOOTHVIBE_KEY)),
  };
  
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, NULL, NULL);
   
  appStarted = true;
  
  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);  
  handle_tick(tick_time, DAY_UNIT + HOUR_UNIT + MINUTE_UNIT + SECOND_UNIT );

  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
  bluetooth_connection_service_subscribe(bluetooth_connection_callback);
  battery_state_service_subscribe(&update_battery);

  // draw first frame
  force_update();

}

static void deinit(void) {

  app_sync_deinit(&sync);
  
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();

 /* layer_remove_from_parent(bitmap_layer_get_layer(time_format_layer));
  bitmap_layer_destroy(time_format_layer);
  gbitmap_destroy(time_format_image);
  time_format_image = NULL;
	*/
	
  layer_remove_from_parent(bitmap_layer_get_layer(separator_layer));
  bitmap_layer_destroy(separator_layer);
  gbitmap_destroy(separator_image);
  separator_image = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(layer_batt_img));
  bitmap_layer_destroy(layer_batt_img);	
  gbitmap_destroy(digds_batt_100);
  gbitmap_destroy(digds_batt_90);
  gbitmap_destroy(digds_batt_80);
  gbitmap_destroy(digds_batt_70);
  gbitmap_destroy(digds_batt_60);
  gbitmap_destroy(digds_batt_50);
  gbitmap_destroy(digds_batt_40);
  gbitmap_destroy(digds_batt_30);
  gbitmap_destroy(digds_batt_20);
  gbitmap_destroy(digds_batt_10);
	
  layer_remove_from_parent(bitmap_layer_get_layer(day_name_layer));
  bitmap_layer_destroy(day_name_layer);
  gbitmap_destroy(day_name_image);
  day_name_image = NULL;

   for (int i = 0; i < TOTAL_MAIN_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers[i]));
    gbitmap_destroy(time_digits_images[i]);
    time_digits_images[i] = NULL;
    bitmap_layer_destroy(time_digits_layers[i]);
	time_digits_layers[i] = NULL;
   }

   for (int i = 0; i < TOTAL_SECONDS_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(seconds_digits_layers[i]));
    gbitmap_destroy(seconds_digits_images[i]);
    seconds_digits_images[i] = NULL;
    bitmap_layer_destroy(seconds_digits_layers[i]);
	seconds_digits_layers[i] = NULL;  
  }

   for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
	layer_remove_from_parent(bitmap_layer_get_layer(date_digits_layers[i]));
    gbitmap_destroy(date_digits_images[i]);
    date_digits_layers[i] = NULL;
    bitmap_layer_destroy(date_digits_layers[i]);
	date_digits_layers[i] = NULL;
   }
	/*
   for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
	layer_remove_from_parent(bitmap_layer_get_layer(battery_percent_layers[i]));
    gbitmap_destroy(battery_percent_images[i]);
    battery_percent_images[i] = NULL;
    bitmap_layer_destroy(battery_percent_layers[i]);
	battery_percent_layers[i] = NULL;
  }
	*/
  layer_remove_from_parent(window_layer);
  layer_destroy(window_layer);

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}