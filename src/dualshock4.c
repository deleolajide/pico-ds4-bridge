#include "dualshock4.h"

#include <stdbool.h>
#include <string.h>

ds4_report_t default_ds4_report() {
  ds4_report_t report = {
      .report_id = 0x01,
      .left_stick_x = DS4_JOYSTICK_MID,
      .left_stick_y = DS4_JOYSTICK_MID,
      .right_stick_x = DS4_JOYSTICK_MID,
      .right_stick_y = DS4_JOYSTICK_MID,
      .dpad = 0x0F,
      .button_west = 0,
      .button_south = 0,
      .button_east = 0,
      .button_north = 0,
      .button_l1 = 0,
      .button_r1 = 0,
      .button_l2 = 0,
      .button_r2 = 0,
      .button_l3 = 0,
      .button_r3 = 0,
      .button_select = 0,
      .button_start = 0,
      .button_home = 0,
      .report_counter = 0,
      .left_trigger = 0,
      .right_trigger = 0,
      .axis_timing = 0,
      .battery = 0,
      .touchpad_active = 0,
      .padding = 0,
      .tpad_increment = 0,
  };

  memset(&(report.sensor_data), 0, sizeof(ds4_sensor_data_t));
  memset(&(report.touchpad_data), 0, sizeof(ds4_touchpad_data_t));
  memset(&(report.mystery_2), 0, sizeof(report.mystery_2));
  report.sensor_data.batteryLevel = 0x5;

  return report;
}

uint8_t dpad_mask_to_hat(uint8_t mask) {
  switch (mask) {
    case 0x00:
      return 0x0F;  // none
    case DS4_BP_UP:
      return 0x00;  // up
    case DS4_BP_UP | DS4_BP_RIGHT:
      return 0x01;  // up‑right
    case DS4_BP_RIGHT:
      return 0x02;  // right
    case DS4_BP_DOWN | DS4_BP_RIGHT:
      return 0x03;  // right‑down
    case DS4_BP_DOWN:
      return 0x04;  // down
    case DS4_BP_DOWN | DS4_BP_LEFT:
      return 0x05;  // down‑left
    case DS4_BP_LEFT:
      return 0x06;  // left
    case DS4_BP_UP | DS4_BP_LEFT:
      return 0x07;  // up-left
    default:
      return 0x0F;  // default
  }
}

void convert_uni_to_ds4(const uni_controller_t* uni, ds4_report_t* ds4) {
  const uni_gamepad_t* gamepad = &uni->gamepad;

  ds4->report_id = 0x01;
  ds4->left_stick_x = (uint8_t)(gamepad->axis_x / 4 + 127);
  ds4->left_stick_y = (uint8_t)(gamepad->axis_y / 4 + 127);
  ds4->right_stick_x = (uint8_t)(gamepad->axis_rx / 4 + 127);
  ds4->right_stick_y = (uint8_t)(gamepad->axis_ry / 4 + 127);
  ds4->dpad = (uint32_t)(dpad_mask_to_hat(gamepad->dpad & 0x0F));

  uint16_t buttons = gamepad->buttons;
  uint16_t misc_buttons = gamepad->misc_buttons;

  // buttons
  // printf("buttons: %04X\n", buttons);
  ds4->button_south = (buttons >> 0) & 0x01;
  ds4->button_east = (buttons >> 1) & 0x01;
  ds4->button_west = (buttons >> 2) & 0x01;
  ds4->button_north = (buttons >> 3) & 0x01;
  ds4->button_l1 = (buttons >> 4) & 0x01;
  ds4->button_r1 = (buttons >> 5) & 0x01;
  ds4->button_l2 = (buttons >> 6) & 0x01;
  ds4->button_r2 = (buttons >> 7) & 0x01;
  ds4->button_l3 = (buttons >> 8) & 0x01;
  ds4->button_r3 = (buttons >> 9) & 0x01;

  // misc_buttons
  // printf("buttons: %04X\n", misc_buttons);
  ds4->button_home = (misc_buttons >> 0) & 0x01;
  ds4->button_select = (misc_buttons >> 1) & 0x01;
  ds4->button_start = (misc_buttons >> 2) & 0x01;

  static uint8_t report_counter = 0;
  ds4->report_counter = report_counter++ & 0x3F;

  ds4->left_trigger = (uint8_t)(gamepad->brake / 4);
  ds4->right_trigger = (uint8_t)(gamepad->throttle / 4);
  ds4->axis_timing = 0;

  // sensor data
  ds4->battery = (uni->battery / 25) & 0x0F;
  ds4->sensor_data.batteryLevel = (uni->battery / 25) & 0x0F;
  ds4->sensor_data.usb = 0x0;
  ds4->sensor_data.microphone = 0x0;
  ds4->sensor_data.headphone = 0x0;
  ds4->sensor_data.extension = 0x0;
  ds4->sensor_data.misc2 = 0x00;

  ds4->sensor_data.gyro.x = gamepad->gyro[0];
  ds4->sensor_data.gyro.y = gamepad->gyro[1];
  ds4->sensor_data.gyro.z = gamepad->gyro[2];
  ds4->sensor_data.accel.x = gamepad->accel[0];
  ds4->sensor_data.accel.y = gamepad->accel[1];
  ds4->sensor_data.accel.z = gamepad->accel[2];

  ds4->touchpad_active = 0x0;
  ds4->padding = 0;
  ds4->tpad_increment = 0;
  memset(&ds4->touchpad_data, 0, sizeof(ds4->touchpad_data));
  memset(ds4->mystery_2, 0, sizeof(ds4->mystery_2));
}
