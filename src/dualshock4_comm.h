#ifndef DUALSHOCK4_COMM_H_
#define DUALSHOCK4_COMM_H_

#include <stdint.h>

#include <hardware/sync.h>

#include "dualshock4.h"

typedef struct {
  ds4_report_t controller;
  uint32_t timestamp;
  volatile spin_lock_t* lock;
} dualshock4_shared_data_t;

extern dualshock4_shared_data_t g_ds4_shared_data;

#endif  // DUALSHOCK4_COMM_H_