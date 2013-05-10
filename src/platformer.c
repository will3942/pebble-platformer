#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0x52, 0xE1, 0x4D, 0xA0, 0x67, 0x93, 0x49, 0xF8, 0xAB, 0xDA, 0x5E, 0x0B, 0xC7, 0x3C, 0xA9, 0xC4 }
PBL_APP_INFO(MY_UUID,
            "Platformer", "Defined Code Ltd",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);

Window window;
Layer layer, level, base;

int in_air;
int move_clicked = 0;
int selectdata = 0;
int current_direction; //0 = right, 1 = left, 2 = up

typedef struct {
  int x;
  int y;
  bool ladder;
} block;

typedef struct {
  PropertyAnimation moveAnimation;
  PropertyAnimation downAnimation;
  RotBmpPairContainer playerContainer;
} Player;

Player player;

static block blocks[20] = {
  {.x = 20, .y = 128, .ladder = false},
  {.x = 32, .y = 116, .ladder = false},
  {.x = 44, .y = 104, .ladder = false},
  {.x = 56, .y = 92, .ladder = false},
  {.x = 68, .y = 104, .ladder = false},
  {.x = 80, .y = 116, .ladder = false},
  {.x = 92, .y = 116, .ladder = false},
  {.x = 104, .y = 116, .ladder = false},
  {.x = 116, .y = 116, .ladder = false},
  {.x = 132, .y = 104, .ladder = true},
  {.x = 132, .y = 92, .ladder = true},
  {.x = 132, .y = 80, .ladder = true},
  {.x = 132, .y = 68, .ladder = true},
  {.x = 132, .y = 56, .ladder = true},
  {.x = 120, .y = 44, .ladder = false},
  {.x = 108, .y = 44, .ladder = false},
  {.x = 96, .y = 44, .ladder = false},
  {.x = 84, .y = 44, .ladder = false},
  {.x = 72, .y = 44, .ladder = false},
  {.x = 60, .y = 44, .ladder = false},
};

bool valueInRange(int value, int min, int max) {
    return (value >= min) && (value <= max);
}
void draw_base(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer->bounds, 0, GCornerNone);
}
void draw_player(RotBmpPairContainer *bmp_container) {
  rotbmp_pair_init_container(RESOURCE_ID_IMAGE_PLAYER_WHITE, RESOURCE_ID_IMAGE_PLAYER_BLACK, bmp_container);
  layer_set_frame(&bmp_container->layer.layer, GRect(-5, 115, 22, 25));
}
void draw_level(Layer *layer, GContext *ctx) {
  int bcount;
  for (bcount = 0; bcount < 20; bcount++) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_draw_round_rect(ctx, GRect(blocks[bcount].x, blocks[bcount].y, 12, 12), 1);
  }
}
void down_started(Animation *animation, void *data) {
  int digit = *(int*)data;
}
void down_stopped(Animation *animation, void *data) {
  int digit = *(int*)data;
  in_air = 0;
}
void animation_started(Animation *animation, void *data) {
  int digit = *(int*)data;
  digit = 1;
  selectdata = digit;
}
void animation_stopped(Animation *animation, void *data) {
  int digit = *(int*)data;
  int abovebk;
  int intersected;
  int bcount;
  digit = 1;
  selectdata = digit;
  intersected = 0;
  GRect r1 = layer_get_frame(&player.playerContainer.layer.layer);
  for (bcount = 0; bcount < 20; bcount++) {
    GRect r2 = GRect(blocks[bcount].x, blocks[bcount].y, 12, 12);
    bool aboveblock = valueInRange(r1.origin.x + 5, r2.origin.x, r2.origin.x + r2.size.w) || valueInRange(r2.origin.x, r1.origin.x + 5, r1.origin.x + r1.size.w - 5);
    bool notbelow = (r2.origin.y - (r1.origin.y + 25)) >= 0;
    if (aboveblock && notbelow) {
      abovebk = bcount;
      intersected = 1;
    }
  }
  if (intersected == 0) {
    GRect ground = layer_get_frame(&base);
    GRect rect = layer_get_frame(&player.playerContainer.layer.layer);
    rect.origin.y -= (rect.origin.y - ground.origin.y) + 25;
    property_animation_init_layer_frame(&player.downAnimation, &player.playerContainer.layer.layer, NULL, &rect);
    animation_set_handlers(&player.downAnimation.animation, (AnimationHandlers) {
         .started = (AnimationStartedHandler) down_started,
         .stopped = (AnimationStoppedHandler) down_stopped,
      }, &(selectdata));
    animation_schedule(&player.downAnimation.animation);  
    move_clicked = 0;
  }
  else {
    GRect block = GRect(blocks[abovebk].x, blocks[abovebk].y, 12, 12);
    GRect rect = layer_get_frame(&player.playerContainer.layer.layer);
    rect.origin.y -= (rect.origin.y - block.origin.y) + 25;
    property_animation_init_layer_frame(&player.downAnimation, &player.playerContainer.layer.layer, NULL, &rect);
    animation_set_handlers(&player.downAnimation.animation, (AnimationHandlers) {
         .started = (AnimationStartedHandler) down_started,
         .stopped = (AnimationStoppedHandler) down_stopped,
      }, &(selectdata));
    animation_schedule(&player.downAnimation.animation);
    move_clicked = 0;
  }
}
void move_right(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  GRect rect = layer_get_frame(&player.playerContainer.layer.layer);
  rect.origin.x += 5;
  property_animation_init_layer_frame(&player.moveAnimation, &player.playerContainer.layer.layer, NULL, &rect);
  animation_set_handlers(&player.moveAnimation.animation, (AnimationHandlers) {
        .started = (AnimationStartedHandler) animation_started,
        .stopped = (AnimationStoppedHandler) animation_stopped,
      }, &(selectdata));
  animation_schedule(&player.moveAnimation.animation);
  if (current_direction != 0) {
    layer_remove_from_parent(&player.playerContainer.layer.layer);
    rotbmp_pair_deinit_container(&player.playerContainer);
    rotbmp_pair_init_container(RESOURCE_ID_IMAGE_PLAYER_WHITE, RESOURCE_ID_IMAGE_PLAYER_BLACK, &player.playerContainer);
    layer_set_frame(&player.playerContainer.layer.layer, rect);
    layer_add_child(&layer, &player.playerContainer.layer.layer);
    current_direction = 0;
  }
}
void move_left(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  GRect rect = layer_get_frame(&player.playerContainer.layer.layer);
  rect.origin.x -= 5;
  property_animation_init_layer_frame(&player.moveAnimation, &player.playerContainer.layer.layer, NULL, &rect);
  animation_set_handlers(&player.moveAnimation.animation, (AnimationHandlers) {
        .started = (AnimationStartedHandler) animation_started,
        .stopped = (AnimationStoppedHandler) animation_stopped,
      }, &(selectdata));
  animation_schedule(&player.moveAnimation.animation);
  if (current_direction != 1) {
    layer_remove_from_parent(&player.playerContainer.layer.layer);
    rotbmp_pair_deinit_container(&player.playerContainer);
    rotbmp_pair_init_container(RESOURCE_ID_IMAGE_PLAYER_LEFT_WHITE, RESOURCE_ID_IMAGE_PLAYER_LEFT_BLACK, &player.playerContainer);
    layer_set_frame(&player.playerContainer.layer.layer, rect);
    layer_add_child(&layer, &player.playerContainer.layer.layer);
    current_direction = 1;
  }
}
void jump(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  GRect rect = layer_get_frame(&player.playerContainer.layer.layer);
  rect.origin.y -= 14;
  property_animation_init_layer_frame(&player.moveAnimation, &player.playerContainer.layer.layer, NULL, &rect);
  animation_set_handlers(&player.moveAnimation.animation, (AnimationHandlers) {
        .started = (AnimationStartedHandler) animation_started,
        .stopped = (AnimationStoppedHandler) animation_stopped,
      }, &(selectdata));
  if (!(in_air == 1)) {
    animation_schedule(&player.moveAnimation.animation);
    in_air = 1;
  }
  if (current_direction != 2) {
    layer_remove_from_parent(&player.playerContainer.layer.layer);
    rotbmp_pair_deinit_container(&player.playerContainer);
    rotbmp_pair_init_container(RESOURCE_ID_IMAGE_PLAYER_JUMP_WHITE, RESOURCE_ID_IMAGE_PLAYER_JUMP_BLACK, &player.playerContainer);
    layer_set_frame(&player.playerContainer.layer.layer, rect);
    layer_add_child(&layer, &player.playerContainer.layer.layer);
    current_direction = 2;
  }
}
void click_config_provider(ClickConfig **config, Window *window) {
  (void)window;

  config[BUTTON_ID_UP]->click.handler = (ClickHandler) move_left;
  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) jump;
  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) move_right;
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "Main");
  window_stack_push(&window, true /* Animated */);

  resource_init_current_app(&APP_RESOURCES);  

  layer_init(&layer, GRect(0, 0, 10000, 152));
  layer_init(&level, GRect(0, 0, 10000, 152));
  layer_init(&base, GRect(0, 140, 10000, 20));
  base.update_proc = draw_base;
  layer_add_child(&window.layer, &layer);
  layer_add_child(&layer, &level);
  layer_add_child(&level, &base);
  draw_player(&player.playerContainer);
  layer_add_child(&layer, &player.playerContainer.layer.layer);
  level.update_proc = draw_level;
  window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init
  };
  app_event_loop(params, &handlers);
}
