#include "pico_bluetooth.h"

#include <stddef.h>
#include <string.h>

#include <controller/uni_gamepad.h>
#include <pico/cyw43_arch.h>
#include <pico/time.h>
#include <uni.h>

#include "comm.h"
#include "dualshock4.h"
#include "sdkconfig.h"

#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// Declarations
static void trigger_event_on_gamepad(uni_hid_device_t* d);
static ds4_report_t* ds4_report_ptr = NULL;

// Platform Overrides
static void pico_bluetooth_init(int argc, const char** argv) {
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  ds4_report_ptr = (ds4_report_t*)malloc(sizeof(ds4_report_t));
  if (ds4_report_ptr != NULL) {
    memset(ds4_report_ptr, 0, sizeof(ds4_report_t));
  }
}

static void pico_bluetooth_on_init_complete(void) {
  // Safe to call "unsafe" functions since they are called from BT thread

  // Start scanning and autoconnect to supported controllers.
  uni_bt_start_scanning_and_autoconnect_unsafe();

  // Based on runtime condition, you can delete or list the stored BT keys.
  if (1)
    uni_bt_del_keys_unsafe();
  else
    uni_bt_list_keys_unsafe();

  uni_property_dump_all();
}

static uni_error_t pico_bluetooth_on_device_discovered(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) {
  // You can filter discovered devices here. Return any value different from UNI_ERROR_SUCCESS;
  // @param addr: the Bluetooth address
  // @param name: could be NULL, could be zero-length, or might contain the name.
  // @param cod: Class of Device. See "uni_bt_defines.h" for possible values.
  // @param rssi: Received Signal Strength Indicator (RSSI) measured in dBms. The higher (255) the better.

  printf("[INFO] Discovered device: %s (%02X:%02X:%02X:%02X:%02X:%02X) rssi=%d\n", name, addr[0], addr[1], addr[2],
         addr[3], addr[4], addr[5], rssi);

  // As an example, if you want to filter out keyboards, do:
  if (((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_KEYBOARD) == UNI_BT_COD_MINOR_KEYBOARD) {
    logi("Ignoring keyboard\n");
    return UNI_ERROR_IGNORE_DEVICE;
  }

  return UNI_ERROR_SUCCESS;
}

static void pico_bluetooth_on_device_connected(uni_hid_device_t* d) {
  // Disable scanning when a device is connected.
  // This is optional, but it will save power.
  uni_bt_stop_scanning_safe();
}

static void pico_bluetooth_on_device_disconnected(uni_hid_device_t* d) {
  // Re-enable scanning when a device is disconnected.
  // This is optional, but it will save power.
  uni_bt_start_scanning_and_autoconnect_safe();
}

static uni_error_t pico_bluetooth_on_device_ready(uni_hid_device_t* d) {
  // You can reject the connection by returning an error.
  return UNI_ERROR_SUCCESS;
}

static const uni_property_t* pico_bluetooth_get_property(uni_property_idx_t idx) {
  ARG_UNUSED(idx);
  return NULL;
}

static void convert_uni_to_ds4(const uni_controller_t* uni, ds4_report_t* ds4) {
  const uni_gamepad_t* gamepad = &uni->gamepad;

  ds4->report_id = 0x01;
  ds4->left_stick_x = (uint8_t)(gamepad->axis_x / 4 + 127);
  ds4->left_stick_y = (uint8_t)(gamepad->axis_y / 4 + 127);
  ds4->right_stick_x = (uint8_t)(gamepad->axis_rx / 4 + 127);
  ds4->right_stick_y = (uint8_t)(gamepad->axis_ry / 4 + 127);
  ds4->dpad = (uint32_t)(dpad_mask_to_hat(gamepad->dpad & 0x0F));

  uint16_t buttons = gamepad->buttons;
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

  ds4->left_trigger = (uint8_t)(gamepad->brake / 4);
  ds4->right_trigger = (uint8_t)(gamepad->throttle / 4);
  ds4->axis_timing = 0;

  ds4->battery = uni->battery / 25;
  ds4->sensor_data.gyro.x = gamepad->gyro[0];
  ds4->sensor_data.gyro.y = gamepad->gyro[1];
  ds4->sensor_data.gyro.z = gamepad->gyro[2];
  ds4->sensor_data.accel.x = gamepad->accel[0];
  ds4->sensor_data.accel.y = gamepad->accel[1];
  ds4->sensor_data.accel.z = gamepad->accel[2];

  ds4->touchpad_active = 0;
  ds4->padding = 0;
  ds4->tpad_increment = 0;
  memset(&ds4->touchpad_data, 0, sizeof(ds4->touchpad_data));
  memset(ds4->mystery_2, 0, sizeof(ds4->mystery_2));
}

static void pico_bluetooth_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
  uint32_t now = 0;
  uint32_t save = 0;
  ds4_report_t* tmp = NULL;

  switch (ctl->klass) {
    case UNI_CONTROLLER_CLASS_GAMEPAD:
      // Print device Id and dump gamepad.
      // logi("(%p) id=%d ", d, uni_hid_device_get_idx_for_instance(d));
      // uni_controller_dump(ctl);

      now = to_us_since_boot(get_absolute_time());
      {
        save = spin_lock_blocking(g_shared_data.lock);
        convert_uni_to_ds4(ctl, g_shared_data.ctrl);
        g_shared_data.timestamp = now;

        spin_unlock(g_shared_data.lock, save);
      }
      break;
    case UNI_CONTROLLER_CLASS_BALANCE_BOARD:
      // DO NOTHING
      break;
    case UNI_CONTROLLER_CLASS_MOUSE:
      // DO NOTHING
      break;
    case UNI_CONTROLLER_CLASS_KEYBOARD:
      // DO NOTHING
      break;
    default:
      loge("Unsupported controller class: %d\n", ctl->klass);
      break;
  }
}

static void pico_bluetooth_on_oob_event(uni_platform_oob_event_t event, void* data) {
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

static void trigger_event_on_gamepad(uni_hid_device_t* d) {
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
      .init = pico_bluetooth_init,
      .on_init_complete = pico_bluetooth_on_init_complete,
      .on_device_discovered = pico_bluetooth_on_device_discovered,
      .on_device_connected = pico_bluetooth_on_device_connected,
      .on_device_disconnected = pico_bluetooth_on_device_disconnected,
      .on_device_ready = pico_bluetooth_on_device_ready,
      .on_oob_event = pico_bluetooth_on_oob_event,
      .on_controller_data = pico_bluetooth_on_controller_data,
      .get_property = pico_bluetooth_get_property,
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