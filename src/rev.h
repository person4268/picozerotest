#pragma once
#include "can.h"

void rev_can_frame_callback(struct can_msg* frame);
void rev_fun_task(void* params);
void rev_register_commands();