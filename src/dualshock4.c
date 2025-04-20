#include "dualshock4.h"

ds4_report_t default_ds4_report() {
  ds4_report_t report = {.report_id = 0x01,
                         .left_stick_x = DS4_JOYSTICK_MID,
                         .left_stick_y = DS4_JOYSTICK_MID,
                         .right_stick_x = DS4_JOYSTICK_MID,
                         .right_stick_y = DS4_JOYSTICK_MID,
                         .dpad = 0x08,
                         .button_west = 0,
                         .button_south = 0,
                         .button_east = 0,
                         .button_north = 0,
                         .button_l1 = 0,
                         .button_r1 = 0,
                         .button_l2 = 0,
                         .button_r2 = 0,
                         .button_select = 0,
                         .button_start = 0,
                         .button_l3 = 0,
                         .button_r3 = 0,
                         .button_home = 0,
                         .sensor_data = {},
                         .touchpad_active = 0,
                         .padding = 0,
                         .tpad_increment = 0,
                         .touchpad_data = {},
                         .mystery_2 = {}};
  return report;
}
