#include "comm.h"

shared_data_t g_shared_data = {
    .timestamp = 0,
    .ctrl = NULL,
    .lock = NULL,
};