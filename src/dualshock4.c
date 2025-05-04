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