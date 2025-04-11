#include "pico_bluetooth.h"

#include <stddef.h>
#include <string.h>

#include <pico/cyw43_arch.h>
#include <pico/time.h>
#include <uni.h>

#include "dualshock4_shared_data.h"
#include "sdkconfig.h"

#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// Declarations
static void trigger_event_on_gamepad(uni_hid_device_t* d);

//
// Platform Overrides
//
static void my_platform_init(int argc, const char** argv) {
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  logi("my_platform: init()\n");

#if 0
    uni_gamepad_mappings_t mappings = GAMEPAD_DEFAULT_MAPPINGS;

    // Inverted axis with inverted Y in RY.
    mappings.axis_x = UNI_GAMEPAD_MAPPINGS_AXIS_RX;
    mappings.axis_y = UNI_GAMEPAD_MAPPINGS_AXIS_RY;
    mappings.axis_ry_inverted = true;
    mappings.axis_rx = UNI_GAMEPAD_MAPPINGS_AXIS_X;
    mappings.axis_ry = UNI_GAMEPAD_MAPPINGS_AXIS_Y;

    // Invert A & B
    mappings.button_a = UNI_GAMEPAD_MAPPINGS_BUTTON_B;
    mappings.button_b = UNI_GAMEPAD_MAPPINGS_BUTTON_A;

    uni_gamepad_set_mappings(&mappings);
#endif
}

static void my_platform_on_init_complete(void) {
  logi("my_platform: on_init_complete()\n");

  // Safe to call "unsafe" functions since they are called from BT thread

  // Start scanning and autoconnect to supported controllers.
  uni_bt_start_scanning_and_autoconnect_unsafe();

  // // Based on runtime condition, you can delete or list the stored BT keys.
  // if (1)
  //     uni_bt_del_keys_unsafe();
  // else
  uni_bt_list_keys_unsafe();

  // Turn off LED once init is done.
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

  //    uni_bt_service_set_enabled(true);

  uni_property_dump_all();
}

static uni_error_t my_platform_on_device_discovered(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) {
  // You can filter discovered devices here. Return any value different from UNI_ERROR_SUCCESS;
  // @param addr: the Bluetooth address
  // @param name: could be NULL, could be zero-length, or might contain the name.
  // @param cod: Class of Device. See "uni_bt_defines.h" for possible values.
  // @param rssi: Received Signal Strength Indicator (RSSI) measured in dBms. The higher (255) the better.

  // As an example, if you want to filter out keyboards, do:
  if (((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_KEYBOARD) == UNI_BT_COD_MINOR_KEYBOARD) {
    logi("Ignoring keyboard\n");
    return UNI_ERROR_IGNORE_DEVICE;
  }

  return UNI_ERROR_SUCCESS;
}

static void my_platform_on_device_connected(uni_hid_device_t* d) {
  logi("my_platform: device connected: %p\n", d);
}

static void my_platform_on_device_disconnected(uni_hid_device_t* d) {
  logi("my_platform: device disconnected: %p\n", d);
}

static uni_error_t my_platform_on_device_ready(uni_hid_device_t* d) {
  logi("my_platform: device ready: %p\n", d);

  // You can reject the connection by returning an error.
  return UNI_ERROR_SUCCESS;
}

static void convert_uni_to_ds4(const uni_controller_t* uni, ds4_report_t* ds4) {
  uni_gamepad_t* uni_ctl = &uni->gamepad;

  ds4->report_id = 0x01;
  ds4->left_stick_x = (uint8_t)(uni_ctl->axis_x / 4 + 0x80 - 1);
  ds4->left_stick_y = (uint8_t)(uni_ctl->axis_y / 4 + 0x80 - 1);
  ds4->right_stick_x = (uint8_t)(uni_ctl->axis_rx / 4 + 0x80 - 1);
  ds4->right_stick_y = (uint8_t)(uni_ctl->axis_ry / 4 + 0x80 - 1);
  ds4->dpad = uni_ctl->dpad & 0x0F;

  uint16_t buttons = uni_ctl->buttons;
  ds4->button_west = (buttons >> 0) & 0x01;
  ds4->button_south = (buttons >> 1) & 0x01;
  ds4->button_east = (buttons >> 2) & 0x01;
  ds4->button_north = (buttons >> 3) & 0x01;
  ds4->button_l1 = (buttons >> 4) & 0x01;
  ds4->button_r1 = (buttons >> 5) & 0x01;
  ds4->button_l2 = (buttons >> 6) & 0x01;
  ds4->button_r2 = (buttons >> 7) & 0x01;
  ds4->button_select = (buttons >> 8) & 0x01;
  ds4->button_start = (buttons >> 9) & 0x01;
  ds4->button_l3 = (buttons >> 10) & 0x01;
  ds4->button_r3 = (buttons >> 11) & 0x01;
  ds4->button_home = (buttons >> 12) & 0x01;
  ds4->button_touchpad = (buttons >> 13) & 0x01;

  static uint8_t report_counter = 0;
  ds4->report_counter = report_counter & 0x3F;
  report_counter++;

  ds4->left_trigger = (uint8_t)(uni_ctl->brake / 4);
  ds4->right_trigger = (uint8_t)(uni_ctl->throttle / 4);
  ds4->axis_timing = 0;

  ds4->sensor_data.gyroscope.x = uni_ctl->gyro[0];
  ds4->sensor_data.gyroscope.y = uni_ctl->gyro[1];
  ds4->sensor_data.gyroscope.z = uni_ctl->gyro[2];
  ds4->sensor_data.accelerometer.x = uni_ctl->accel[0];
  ds4->sensor_data.accelerometer.y = uni_ctl->accel[1];
  ds4->sensor_data.accelerometer.z = uni_ctl->accel[2];
  ds4->sensor_data.battery = uni->battery;
  ds4->touchpad_active = 0;
  ds4->tpad_increment = 0;
  memset(&ds4->touchpad_data, 0, sizeof(ds4->touchpad_data));
  memset(ds4->mystery_2, 0, sizeof(ds4->mystery_2));
}

static void my_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
  assert(g_ds4_shared_data.timestamp != 0);

  static uint8_t leds = 0;
  static uint8_t enabled = true;
  static uni_controller_t prev = {0};
  uni_gamepad_t* gp;

  // Used to prevent spamming the log, but should be removed in production.
  //    if (memcmp(&prev, ctl, sizeof(*ctl)) == 0) {
  //        return;
  //    }
  prev = *ctl;
  // Print device Id before dumping gamepad.
  // logi("(%p) id=%d ", d, uni_hid_device_get_idx_for_instance(d));
  // uni_controller_dump(ctl);

  ds4_report_t control;
  convert_uni_to_ds4(ctl, &control);

  uint32_t save = spin_lock_blocking(g_ds4_shared_data.lock);
  g_ds4_shared_data.timestamp = to_ms_since_boot(get_absolute_time());
  memcpy(&g_ds4_shared_data.controller, &control, sizeof(control));
  spin_unlock(g_ds4_shared_data.lock, save);

  switch (ctl->klass) {
    case UNI_CONTROLLER_CLASS_GAMEPAD:
      gp = &ctl->gamepad;

      // // Debugging
      // // Axis ry: control rumble
      // if ((gp->buttons & BUTTON_A) && d->report_parser.play_dual_rumble != NULL) {
      //   d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 250 /* duration ms */, 128 /* weak magnitude
      //   */,
      //                                     0 /* strong magnitude */);
      // }

      // if ((gp->buttons & BUTTON_B) && d->report_parser.play_dual_rumble != NULL) {
      //   d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 250 /* duration ms */, 0 /* weak magnitude */,
      //                                     128 /* strong magnitude */);
      // }
      // Buttons: Control LEDs On/Off
      if ((gp->buttons & BUTTON_X) && d->report_parser.set_player_leds != NULL) {
        d->report_parser.set_player_leds(d, leds++ & 0x0f);
      }
      // Axis: control RGB color
      if ((gp->buttons & BUTTON_Y) && d->report_parser.set_lightbar_color != NULL) {
        uint8_t r = (gp->axis_x * 256) / 512;
        uint8_t g = (gp->axis_y * 256) / 512;
        uint8_t b = (gp->axis_rx * 256) / 512;
        d->report_parser.set_lightbar_color(d, r, g, b);
      }

      // // Toggle Bluetooth connections
      // if ((gp->buttons & BUTTON_SHOULDER_L) && enabled) {
      //   logi("*** Disabling Bluetooth connections\n");
      //   uni_bt_stop_scanning_safe();
      //   enabled = false;
      // }
      // if ((gp->buttons & BUTTON_SHOULDER_R) && !enabled) {
      //   logi("*** Enabling Bluetooth connections\n");
      //   uni_bt_start_scanning_and_autoconnect_safe();
      //   enabled = true;
      // }
      break;
    case UNI_CONTROLLER_CLASS_BALANCE_BOARD:
      // Do something
      uni_balance_board_dump(&ctl->balance_board);
      break;
    case UNI_CONTROLLER_CLASS_MOUSE:
      // Do something
      uni_mouse_dump(&ctl->mouse);
      break;
    case UNI_CONTROLLER_CLASS_KEYBOARD:
      // Do something
      uni_keyboard_dump(&ctl->keyboard);
      break;
    default:
      loge("Unsupported controller class: %d\n", ctl->klass);
      break;
  }
}

static const uni_property_t* my_platform_get_property(uni_property_idx_t idx) {
  ARG_UNUSED(idx);
  return NULL;
}

static void my_platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
  switch (event) {
    case UNI_PLATFORM_OOB_GAMEPAD_SYSTEM_BUTTON:
      // Optional: do something when "system" button gets pressed.
      trigger_event_on_gamepad((uni_hid_device_t*)data);
      break;

    case UNI_PLATFORM_OOB_BLUETOOTH_ENABLED:
      // When the "bt scanning" is on / off. Could be triggered by different events
      // Useful to notify the user
      logi("my_platform_on_oob_event: Bluetooth enabled: %d\n", (bool)(data));
      break;

    default:
      logi("my_platform_on_oob_event: unsupported event: 0x%04x\n", event);
  }
}

//
// Helpers
//
static void trigger_event_on_gamepad(uni_hid_device_t* d) {
  if (d->report_parser.play_dual_rumble != NULL) {
    d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 50 /* duration ms */, 128 /* weak magnitude */,
                                      40 /* strong magnitude */);
  }

  if (d->report_parser.set_player_leds != NULL) {
    static uint8_t led = 0;
    led += 1;
    led &= 0xf;
    d->report_parser.set_player_leds(d, led);
  }

  if (d->report_parser.set_lightbar_color != NULL) {
    static uint8_t red = 0x10;
    static uint8_t green = 0x20;
    static uint8_t blue = 0x40;

    red += 0x10;
    green -= 0x20;
    blue += 0x40;
    d->report_parser.set_lightbar_color(d, red, green, blue);
  }
}

struct uni_platform* get_my_platform(void) {
  static struct uni_platform plat = {
      .name = "Naverlabs Pico2 W",
      .init = my_platform_init,
      .on_init_complete = my_platform_on_init_complete,
      .on_device_discovered = my_platform_on_device_discovered,
      .on_device_connected = my_platform_on_device_connected,
      .on_device_disconnected = my_platform_on_device_disconnected,
      .on_device_ready = my_platform_on_device_ready,
      .on_oob_event = my_platform_on_oob_event,
      .on_controller_data = my_platform_on_controller_data,
      .get_property = my_platform_get_property,
  };

  return &plat;
}

void bluetooth_init(void) {
  // Must be called before uni_init()
  uni_platform_set_custom(get_my_platform());

  // Initialize BP32
  uni_init(0, NULL);
}

void bluetooth_run(void) {
  btstack_run_loop_execute();
}