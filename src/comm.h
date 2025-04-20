#ifndef COMM_H_
#define COMM_H_

#include <stdint.h>

#include <hardware/sync.h>

#include "dualshock4.h"

typedef struct {
  uint32_t timestamp;
  ds4_report_t* ctrl;
  volatile spin_lock_t* lock;
} shared_data_t;

extern shared_data_t g_shared_data;

#endif  // COMM_H_