#pragma once
#include "can.h"

void rev_can_frame_callback(struct can_msg* frame);
void rev_fun_task(void* params);
void rev_register_commands();

float rev_get_position();
float rev_get_velocity();
float rev_get_error();
void rev_set_setpoint(float setpoint);

float rev_get_kp();
float rev_get_ki();
float rev_get_kd();

void rev_set_kp(float kp);
void rev_set_ki(float ki);
void rev_set_kd(float kd);