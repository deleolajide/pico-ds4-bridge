#ifndef COMM_H_
#define COMM_H_

#include <stdint.h>

#include <hardware/sync.h>

#include "dualshock4.h"
#include "seqlock.h"

typedef struct {
  uint32_t timestamp;
  uni_gamepad_t gamepad;
  uint8_t battery;
} ds4_frame_t;

SEQLOCK_DECL(ds4_shared_t, ds4_frame_t);

extern ds4_shared_t g_ds4_shared __attribute__((aligned(32)));

#endif  // COMM_H_