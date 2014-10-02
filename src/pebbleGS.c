#include <pebble.h>
 
Window* window;
TextLayer *visitor_layer, *max_layer;
Layer *path_layer;

char visitor_buffer[32], stats_buffer[64], error_buffer[64];

enum {
	KEY_VISITORS = 0,
	KEY_STATS = 1,
	KEY_ERROR = 2,
};

void graphics_draw_arc(GContext *ctx, GPoint p, int radius, int thickness, int start, int end) {
  start = start % 360;
  end = end % 360;
 
  while (start < 0) start += 360;
  while (end < 0) end += 360;
 
  if (end == 0) end = 360;
  
  float sslope = (float)cos_lookup(start * TRIG_MAX_ANGLE / 360) / (float)sin_lookup(start * TRIG_MAX_ANGLE / 360);
  float eslope = (float)cos_lookup(end * TRIG_MAX_ANGLE / 360) / (float)sin_lookup(end * TRIG_MAX_ANGLE / 360);
 
  if (end == 360) eslope = -1000000;
 
  int ir2 = (radius - thickness) * (radius - thickness);
  int or2 = radius * radius;
 
  for (int x = -radius; x <= radius; x++)
    for (int y = -radius; y <= radius; y++)
    {
      int x2 = x * x;
      int y2 = y * y;
 
      if (
        (x2 + y2 < or2 && x2 + y2 >= ir2) &&
        (
          (y > 0 && start < 180 && x <= y * sslope) ||
          (y < 0 && start > 180 && x >= y * sslope) ||
          (y < 0 && start <= 180) ||
          (y == 0 && start <= 180 && x < 0) ||
          (y == 0 && start == 0 && x > 0)
        ) &&
        (
          (y > 0 && end < 180 && x >= y * eslope) ||
          (y < 0 && end > 180 && x <= y * eslope) ||
          (y > 0 && end >= 180) ||
          (y == 0 && end >= 180 && x < 0) ||
          (y == 0 && start == 0 && x > 0)
        )
      )
        graphics_draw_pixel(ctx, GPoint(p.x + x, p.y + y));
    }
}

static void path_layer_update_callback(Layer *me, GContext *ctx) {
	(void)me;

	GPoint center = {72,85};

	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_draw_arc(ctx, center, 62, 3, 180, 360);
}

void process_tuple(Tuple *t)
{
	int key = t->key;
	int value = t->value->int32;
	char string_value[64];
	strcpy(string_value, t->value->cstring);

	//Decide what to do
	switch(key) {
		case KEY_VISITORS:
			snprintf(visitor_buffer, sizeof("XXXXXX"), "%d", value);
			text_layer_set_font(visitor_layer, fonts_get_system_font("RESOURCE_ID_BITHAM_30_BLACK"));
			text_layer_set_text(visitor_layer, (char*) &visitor_buffer);
			break;
		case KEY_STATS:
			snprintf(stats_buffer, sizeof("XXXXXX active xxxx max: XXXXXX xxxx avg: XXXXXX"), "%s", string_value);
			text_layer_set_text(max_layer, (char*) &stats_buffer);
			break;
		case KEY_ERROR:
			snprintf(error_buffer, sizeof("XXXXXX active xxxx max: XXXXXX xxxx avg: XXXXXX"), "%s", string_value);
			text_layer_set_text(visitor_layer, (char*) &error_buffer);
			break;
	}
}

static void in_received_handler(DictionaryIterator *iter, void *context)
{
	(void) context;
	
	//Get data
	Tuple *t = dict_read_first(iter);
	while(t != NULL)
	{
		process_tuple(t);
		
		//Get next
		t = dict_read_next(iter);
	}
}

static TextLayer* init_text_layer(GRect location, GColor colour, GColor background, const char *res_id, GTextAlignment alignment)
{
	TextLayer *layer = text_layer_create(location);
	text_layer_set_text_color(layer, colour);
	text_layer_set_background_color(layer, background);
	text_layer_set_font(layer, fonts_get_system_font(res_id));
	text_layer_set_text_alignment(layer, alignment);

	return layer;
}
 
void window_load(Window *window)
{
	visitor_layer = init_text_layer(GRect(0, 55, 144, 80), GColorWhite, GColorClear, "RESOURCE_ID_GOTHIC_18_BOLD", GTextAlignmentCenter);
	text_layer_set_text(visitor_layer, "fetching...");
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(visitor_layer));

	max_layer = init_text_layer(GRect(0, 95, 144, 80), GColorWhite, GColorClear, "RESOURCE_ID_GOTHIC_18_BOLD", GTextAlignmentCenter);
	text_layer_set_text(max_layer, " ");
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(max_layer));
}
 
void window_unload(Window *window)
{
	text_layer_destroy(visitor_layer);
	text_layer_destroy(max_layer);
}

void send_int(uint8_t key, uint8_t cmd)
{
	DictionaryIterator *iter;
 	app_message_outbox_begin(&iter);
 	
 	Tuplet value = TupletInteger(key, cmd);
 	dict_write_tuplet(iter, &value);
 	
 	app_message_outbox_send();
}

void tick_callback(struct tm *tick_time, TimeUnits units_changed)
{
	//Every five minutes
	if(tick_time->tm_min % 5 == 0)
	{
		//Send an arbitrary message, the response will be handled by in_received_handler()
		send_int(5, 5);
	}
}
 
void init()
{
	window = window_create();
	WindowHandlers handlers = {
		.load = window_load,
		.unload = window_unload
	};
	window_set_window_handlers(window, handlers);
	window_set_background_color(window, GColorBlack);

	//Register AppMessage events
	app_message_register_inbox_received(in_received_handler);					 
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());		//Largest possible input and output buffer sizes

	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);

	path_layer = layer_create(bounds);
	layer_set_update_proc(path_layer, path_layer_update_callback);
	layer_add_child(window_layer, path_layer);
	
	//Register to receive minutely updates
	tick_timer_service_subscribe(MINUTE_UNIT, tick_callback);

	window_stack_push(window, true);
}
 
void deinit()
{
	tick_timer_service_unsubscribe();
	
	window_destroy(window);
}
 
int main(void)
{
	init();
	app_event_loop();
	deinit();
}
