//////////////////////////////////////////////////////////////////
//
/* haptic_wand_example.c - C file */
//
/* This example demonstrates how to do force control using the Quanser HIL SDK. */
/* It is designed to control Quanser's 5DOF Haptic Wand. The joint coordinates */
/* are read from encoder channels 0-4. Analog output channels 0-4 are used to */
/* drive the motors. */
//
/* This example runs until Esc is pressed. Do not press Ctrl+C. */
//
/* This example demonstrates the use of the following functions: */
/*    hil_open */
/*    hil_set_encoder_counts */
/*    hil_task_create_encoder_reader */
/*    hil_task_start */
/*    hil_task_read_encoder */
/*    hil_write_analog */
/*    hil_task_stop */
/*    hil_task_delete */
/*    hil_close */
//
/* Copyright ©2008 Quanser Inc. */
//////////////////////////////////////////////////////////////////
    
#include "haptic_wand_example.h"
#include "haptic_wand.h"

struct limiter_state
{
    double mean;
    double time;
    int    count;
    int    state;
};

static int  stop = 0;      /* a flag used to stop the controller */
static char message[512];  /* a buffer used for error messages */

static const char      board_type[]                   = "q8";                           /* type of board controlling the haptic wand */
static const char      board_identifier[]             = "0";                            /* the instance of the board controlling the haptic wand */
static const double    l[7]                           = { L1, L2, L3, L4, L5, L6, L7 }; /* link lengths of the haptic wand links */
static const t_uint32  analog_channels[NUM_JOINTS]    = { 0, 1, 2, 3,  4,  5 };         /* analog output channels driving the haptic wand motors */
static const t_uint32  encoder_channels[NUM_JOINTS]   = { 0, 1, 2, 3,  4,  5 };         /* encoder input channels for the haptic wand joint encoders */
static const t_uint32  digital_channels[NUM_JOINTS]   = { 0, 1, 2, 3, 16, 17 };         /* digital output channels driving the amplifier enable lines */
static const t_double  frequency                      = 1000;                           /* sampling frequency of the controller */

static void 
signal_handler(int signal)
{
    stop = 1;
}

/*
    Convert the encoder counts to joint angles in radians.
*/
static void 
encoder_counts_to_joint_angles(const t_int32 counts[NUM_JOINTS], double joint_angles[NUM_JOINTS])
{
    static const double offsets[] = { 0.12760527954869, 3.0139873740411, 0.12760527954869, 3.0139873740411, 0.0000, 0.0000 };
    static const double factors[] = { 2*PI/20000, 2*PI/20000, -2*PI/20000, -2*PI/20000, 2*PI/20000, 2*PI/20000 };

    int i;
    for (i=NUM_JOINTS-1; i >= 0; --i) // convert counts to radians and rearrange channels appropriately
        joint_angles[i] = counts[i] * factors[i] + offsets[i];
}

/*
    Compute the end-effector position and orientation, X, from the joint angles, theta.
*/
static void 
forward_kinematics(const double theta[NUM_JOINTS], double X[NUM_WORLD])
{
    X[0] = l[4] / 0.2e1 + cos(theta[2]) * l[0] / 0.2e1 + cos(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1] / 0.2e1 + cos(theta[0]) * l[0] / 0.2e1 + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1] / 0.2e1;
    X[1] = cos(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) / 0.2e1 - sin(theta[5]) * (-l[6] + l[3]) / 0.2e1 + cos(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) / 0.2e1 - sin(theta[4]) * (l[6] - l[3]) / 0.2e1;
    X[2] = sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) / 0.2e1 + cos(theta[5]) * (-l[6] + l[3]) / 0.2e1 + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) / 0.2e1 + cos(theta[4]) * (l[6] - l[3]) / 0.2e1;
    X[3] = -atan((cos(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) - sin(theta[4]) * (l[6] - l[3]) - cos(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) + sin(theta[5]) * (-l[6] + l[3])) / (l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3])));
    X[4] = atan((cos(theta[0]) * l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1] - cos(theta[2]) * l[0] - cos(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) / (l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3])));
}

/*
    Compute the motor torques, tau, from the end-effector generalized forces, F. Since this conversion
    is configuration-dependent, the joint tangles, theta, are required.
*/
static void 
inverse_force_kinematics(const double theta[NUM_JOINTS], const double F[NUM_WORLD], double tau[NUM_JOINTS])
{
    double J[NUM_WORLD][NUM_JOINTS]; /* Jacobian */

    J[0][0] = -sin(theta[0]) * l[0] / 0.2e1 - sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin
      (theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * 
      (pow(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (-0.2e1 *
      (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) * sin(theta[0]) * l[0] + 0.2e1 * (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * cos(theta[0]) * l[0]) * 
      pow(0.4e1 - (pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), 
      -0.1e1 / 0.2e1) / 0.2e1 + (cos(theta[0]) * l[0] / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) + (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * pow(l[4
      ] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1) * sin(theta[0]) * l[0]) / (0.1e1 + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1) * pow(l[4]
      + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1))) * l[1] / 0.2e1;

    J[0][1] = -sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0
      ], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[
      0]) * l[0] - cos(theta[1]) * l[0]))) * (pow(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 
      0.2e1), -0.1e1 / 0.2e1) / l[1] * (0.2e1 * (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) * sin(theta[1]) * l[0] - 0.2e1 * (sin(theta[0]) * l[0] - sin(
      theta[1]) * l[0]) * cos(theta[1]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1
      ]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (-cos(theta[1]) * l[0] / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) - (sin(theta[0
      ]) * l[0] - sin(theta[1]) * l[0]) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1) * sin(theta[1]) * l[0]) / (0.1e1 + pow(sin(theta[0]) * l[0
      ] - sin(theta[1]) * l[0], 0.2e1) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1))) * l[1] / 0.2e1;

    J[0][2] = -sin(theta[2]) * l[0] / 0.2e1
      - sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1)
      + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * (pow(pow(l[4] + cos(theta[2]) * l[0] - cos(
      theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (-0.2e1 * (l[4] + cos(theta[2]) * l[0] - cos(theta[3
      ]) * l[0]) * sin(theta[2]) * l[0] + 0.2e1 * (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * cos(theta[2]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[2]) * l[
      0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (cos(theta[2]) * l
      [0] / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) + (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) *
      l[0], -0.2e1) * sin(theta[2]) * l[0]) / (0.1e1 + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l
      [0], -0.2e1))) * l[1] / 0.2e1;

    J[0][3] = -sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin
      (theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * 
      (pow(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (0.2e1 * 
      (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) * sin(theta[3]) * l[0] - 0.2e1 * (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * cos(theta[3]) * l[0]) * 
      pow(0.4e1 - (pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), 
      -0.1e1 / 0.2e1) / 0.2e1 + (-cos(theta[3]) * l[0] / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) - (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * pow(l[
      4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1) * sin(theta[3]) * l[0]) / (0.1e1 + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1) * pow(l[4
      ] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1))) * l[1] / 0.2e1;

    J[0][4] = 0.0e0;

    J[0][5] = 0.0e0;
  
    J[1][0] = cos(theta[4]) * (cos(theta[0]) * 
      l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 
      0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * (pow(pow(l[4] + cos(theta[0]) * l[0] - 
      cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (-0.2e1 * (l[4] + cos(theta[0]) * l[0] - cos(
      theta[1]) * l[0]) * sin(theta[0]) * l[0] + 0.2e1 * (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * cos(theta[0]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[0]
      ) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (cos(theta[0
      ]) * l[0] / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) + (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta
      [1]) * l[0], -0.2e1) * sin(theta[0]) * l[0]) / (0.1e1 + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[
      1]) * l[0], -0.2e1))) * l[1]) / 0.2e1;

    J[1][1] = cos(theta[4]) * cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin
      (theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(
      theta[1]) * l[0]))) * (pow(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1), -0.1e1 / 
      0.2e1) / l[1] * (0.2e1 * (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) * sin(theta[1]) * l[0] - 0.2e1 * (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * 
      cos(theta[1]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) 
      * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (-cos(theta[1]) * l[0] / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) - (sin(theta[0]) * l[0] - sin(
      theta[1]) * l[0]) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1) * sin(theta[1]) * l[0]) / (0.1e1 + pow(sin(theta[0]) * l[0] - sin(theta[1]) *
      l[0], 0.2e1) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1))) * l[1] / 0.2e1;

    J[1][2] = cos(theta[5]) * (cos(theta[2]) * l[0] + cos(PI - 
      acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((
      sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * (pow(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l
      [0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (-0.2e1 * (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0])
      * sin(theta[2]) * l[0] + 0.2e1 * (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * cos(theta[2]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[2]) * l[0] - cos(
      theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (cos(theta[2]) * l[0] / (l[4]
      + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) + (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1)
      * sin(theta[2]) * l[0]) / (0.1e1 + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1
      ))) * l[1]) / 0.2e1;

    J[1][3] = cos(theta[5]) * cos(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] 
      - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) 
      * (pow(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (0.2e1
      * (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) * sin(theta[3]) * l[0] - 0.2e1 * (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * cos(theta[3]) * l[0
      ]) * pow(0.4e1 - (pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) * pow(l[1], -0.2e1
      ), -0.1e1 / 0.2e1) / 0.2e1 + (-cos(theta[3]) * l[0] / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) - (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * 
      pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1) * sin(theta[3]) * l[0]) / (0.1e1 + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1) * 
      pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1))) * l[1] / 0.2e1;

    J[1][4] = -sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[
      4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[
      0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) / 0.2e1 - cos(theta[4]) * (l[6] - l[3]) / 0.2e1;

    J[1][5] = -sin(
      theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3
      ]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) / 0.2e1
      - cos(theta[5]) * (-l[6] + l[3]) / 0.2e1;

    J[2][0] = sin(theta[4]) * (cos(theta[0]) * l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) 
      * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] +
      cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * (pow(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]
      ) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (-0.2e1 * (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) * sin(theta[0]) * l[0] + 0.2e1 * (sin(theta[0]) * l[
      0] - sin(theta[1]) * l[0]) * cos(theta[0]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - 
      sin(theta[1]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (cos(theta[0]) * l[0] / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) + 
      (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1) * sin(theta[0]) * l[0]) / (0.1e1 + pow(sin(theta
      [0]) * l[0] - sin(theta[1]) * l[0], 0.2e1) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1))) * l[1]) / 0.2e1;

    J[2][1] = sin(theta[4]) * cos(PI - 
      acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + 
      atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * (pow(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[
      1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (0.2e1 * (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) *
      l[0]) * sin(theta[1]) * l[0] - 0.2e1 * (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * cos(theta[1]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[0]) * l[0] - 
      cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (-cos(theta[1]) * l[0] 
      / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) - (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0
      ], -0.2e1) * sin(theta[1]) * l[0]) / (0.1e1 + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]
      , -0.2e1))) * l[1] / 0.2e1;

    J[2][2] = sin(theta[5]) * (cos(theta[2]) * l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1)
       + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * 
      l[0] - cos(theta[3]) * l[0]))) * (pow(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)
      , -0.1e1 / 0.2e1) / l[1] * (-0.2e1 * (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) * sin(theta[2]) * l[0] + 0.2e1 * (sin(theta[2]) * l[0] - sin(theta[3
      ]) * l[0]) * cos(theta[2]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * 
      l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (cos(theta[2]) * l[0] / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) + (sin(theta[2]) * l
      [0] - sin(theta[3]) * l[0]) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1) * sin(theta[2]) * l[0]) / (0.1e1 + pow(sin(theta[2]) * l[0] - 
      sin(theta[3]) * l[0], 0.2e1) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1))) * l[1]) / 0.2e1;

    J[2][3] = sin(theta[5]) * cos(PI - acos(sqrt
      (pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2
      ]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * (pow(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1)
      + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (0.2e1 * (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) * sin(
      theta[3]) * l[0] - 0.2e1 * (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * cos(theta[3]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) *
      l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (-cos(theta[3]) * l[0] / (l[4] + cos(
      theta[2]) * l[0] - cos(theta[3]) * l[0]) - (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1) * sin
      (theta[3]) * l[0]) / (0.1e1 + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1))) * l[
      1] / 0.2e1;

    J[2][4] = cos(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) 
      * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta
      [1]) * l[0]))) * l[1]) / 0.2e1 - sin(theta[4]) * (l[6] - l[3]) / 0.2e1;

    J[2][5] = cos(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(
      theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(
      theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) / 0.2e1 - sin(theta[5]) * (-l[6] + l[3]) / 0.2e1;

    J[3][0] = -(cos(theta[4
      ]) * (cos(theta[0]) * l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[
      0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * (pow(pow(l[4] + 
      cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (-0.2e1 * (l[4] + cos(
      theta[0]) * l[0] - cos(theta[1]) * l[0]) * sin(theta[0]) * l[0] + 0.2e1 * (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * cos(theta[0]) * l[0]) * pow(0.4e1 - (
      pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) 
      / 0.2e1 + (cos(theta[0]) * l[0] / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) + (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * pow(l[4] + cos(theta[
      0]) * l[0] - cos(theta[1]) * l[0], -0.2e1) * sin(theta[0]) * l[0]) / (0.1e1 + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1) * pow(l[4] + cos(theta[0
      ]) * l[0] - cos(theta[1]) * l[0], -0.2e1))) * l[1]) / (l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos
      (theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l
      [4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(
      l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * 
      l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3])) - (cos(theta[4]) * (sin(theta[
      0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin
      (theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1
      ]) - sin(theta[4]) * (l[6] - l[3]) - cos(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1)
      + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0]
      - cos(theta[3]) * l[0]))) * l[1]) + sin(theta[5]) * (-l[6] + l[3])) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(
      theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(
      theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(
      PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + 
      atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3]), -0.2e1) * 
      sin(theta[4]) * (cos(theta[0]) * l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(
      theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * (pow(pow
      (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (-0.2e1 * (l[4
      ] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) * sin(theta[0]) * l[0] + 0.2e1 * (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * cos(theta[0]) * l[0]) * pow(
      0.4e1 - (pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) * pow(l[1], -0.2e1)
      , -0.1e1 / 0.2e1) / 0.2e1 + (cos(theta[0]) * l[0] / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) + (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * pow
      (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1) * sin(theta[0]) * l[0]) / (0.1e1 + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1) * pow(
      l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1))) * l[1])) / (0.1e1 + pow(cos(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + 
      cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - 
      sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) - sin(theta[4]) * (l[6] - l[3]) - cos(theta[5]) * (sin(theta[2]) * l[0] +
      sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) +
      atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) + sin(theta[5]) * (-l[6] + l[3]), 0.2e1) *
      pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0
      ] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))
      ) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0],
      0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2])
      * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3]), -0.2e1));

    J[3][1] = -(cos(theta[4]) * cos(PI - acos(sqrt(pow(l[4] + cos(theta[0])
      * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) 
      * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * (pow(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l
      [0] - sin(theta[1]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (0.2e1 * (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) * sin(theta[1]) * l[0] - 0.2e1 * 
      (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * cos(theta[1]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(
      theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (-cos(theta[1]) * l[0] / (l[4] + cos(theta[0]) * l[0] - cos(
      theta[1]) * l[0]) - (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1) * sin(theta[1]) * l[0]) / 
      (0.1e1 + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1))) * l[1] / (l[5] + sin(theta[4]
      ) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0
      ], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4
      ]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta
      [2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]
      ) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3])) - (cos(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(
      theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)
      ) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) - sin(theta[4]) * (l[6
      ] - l[3]) - cos(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[
      0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0])
      )) * l[1]) + sin(theta[5]) * (-l[6] + l[3])) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(
      theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] 
      + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4]
      + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] 
      - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3]), -0.2e1) * sin(theta[4]) * cos(PI - 
      acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((
      sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * (pow(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l
      [0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (0.2e1 * (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) 
      * sin(theta[1]) * l[0] - 0.2e1 * (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * cos(theta[1]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[0]) * l[0] - cos(
      theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (-cos(theta[1]) * l[0]
      / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) - (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0
      ], -0.2e1) * sin(theta[1]) * l[0]) / (0.1e1 + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]
      , -0.2e1))) * l[1]) / (0.1e1 + pow(cos(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) +
      pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] -
      cos(theta[1]) * l[0]))) * l[1]) - sin(theta[4]) * (l[6] - l[3]) - cos(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] -
      cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) /
      (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) + sin(theta[5]) * (-l[6] + l[3]), 0.2e1) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + 
      sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + 
      atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(
      theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) *
      l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(
      theta[5]) * (-l[6] + l[3]), -0.2e1));

    J[3][2] = -(-cos(theta[5]) * (cos(theta[2]) * l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * 
      l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(
      theta[2]) * l[0] - cos(theta[3]) * l[0]))) * (pow(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0
      ], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (-0.2e1 * (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) * sin(theta[2]) * l[0] + 0.2e1 * (sin(theta[2]) * l[0] - 
      sin(theta[3]) * l[0]) * cos(theta[2]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(
      theta[3]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (cos(theta[2]) * l[0] / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) + (sin(
      theta[2]) * l[0] - sin(theta[3]) * l[0]) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1) * sin(theta[2]) * l[0]) / (0.1e1 + pow(sin(theta[2]) * 
      l[0] - sin(theta[3]) * l[0], 0.2e1) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1))) * l[1]) / (l[5] + sin(theta[4]) * (sin(theta[0]) * l[0
      ] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1)
      + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin
      (theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[
      3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - 
      cos(theta[5]) * (-l[6] + l[3])) + (cos(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + 
      pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] 
      - cos(theta[1]) * l[0]))) * l[
      1]) - sin(theta[4]) * (l[6] - l[3]) - cos(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1
      ) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[
      0] - cos(theta[3]) * l[0]))) * l[1]) + sin(theta[5]) * (-l[6] + l[3])) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(
      theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(
      theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + 
      sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + 
      atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3]), -0.2e1) 
      * sin(theta[5]) * (cos(theta[2]) * l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(
      theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * (pow(
      pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (-0.2e1 * (
      l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) * sin(theta[2]) * l[0] + 0.2e1 * (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * cos(theta[2]) * l[0]) * 
      pow(0.4e1 - (pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) * pow(l[1], -0.2e1
      ), -0.1e1 / 0.2e1) / 0.2e1 + (cos(theta[2]) * l[0] / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) + (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * 
      pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1) * sin(theta[2]) * l[0]) / (0.1e1 + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1) * pow
      (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1))) * l[1])) / (0.1e1 + pow(cos(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + 
      cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - 
      sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) - sin(theta[4]) * (l[6] - l[3]) - cos(theta[5]) * (sin(theta[2]) * l[0] 
      + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1)
      + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) + sin(theta[5]) * (-l[6] + l[3]), 0.2e1)
      * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l
      [0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]
      ))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0
      ], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[
      2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3]), -0.2e1));

    J[3][3] = -(-cos(theta[5]) * cos(PI - acos(sqrt(pow(l[4] + cos(theta[2]) 
      * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) 
      * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * (pow(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2
      ]) * l[0] - sin(theta[3]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (0.2e1 * (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) * sin(theta[3]) * l[0] - 0.2e1 
      * (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * cos(theta[3]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(
      sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (-cos(theta[3]) * l[0] / (l[4] + cos(theta[2]) * l[0] - 
      cos(theta[3]) * l[0]) - (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1) * sin(theta[3]) * l[0]) 
      / (0.1e1 + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1))) * l[1] / (l[5] + sin(
      theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1])
      * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(
      theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(
      theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(
      theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3])) + (cos(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos
      (theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1
      )) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) - sin(theta[4]) * (l[
      6] - l[3]) - cos(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l
      [0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]
      ))) * l[1]) + sin(theta[5]) * (-l[6] + l[3])) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(
      theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4]
      + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4]
      + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] 
      - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3]), -0.2e1) * sin(theta[5]) * cos(PI - 
      acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((
      sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * (pow(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l
      [0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (0.2e1 * (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) 
      * sin(theta[3]) * l[0] - 0.2e1 * (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * cos(theta[3]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[2]) * l[0] - cos(
      theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (-cos(theta[3]) * l[0
      ] / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) - (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l
      [0], -0.2e1) * sin(theta[3]) * l[0]) / (0.1e1 + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[
      0], -0.2e1))) * l[1]) / (0.1e1 + pow(cos(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1)
      + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0]
      - cos(theta[1]) * l[0]))) * l[1]) - sin(theta[4]) * (l[6] - l[3]) - cos(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0]
      - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0])
      / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) + sin(theta[5]) * (-l[6] + l[3]), 0.2e1) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] +
      sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) +
      atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(
      theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) *
      l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5])
      * (-l[6] + l[3]), -0.2e1));

    J[3][4] = -((-sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) *
      l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(
      theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) - cos(theta[4]) * (l[6] - l[3])) / (l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4]
      + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] 
      - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[
      0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1)
      + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3])) - 
      (cos(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(
      theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) 
      - sin(theta[4]) * (l[6] - l[3]) - cos(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + 
      pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] -
      cos(theta[3]) * l[0]))) * l[1]) + sin(theta[5]) * (-l[6] + l[3])) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta
      [0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta
      [1]) * l[0]) / (l[4] + 
      cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + 
      cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - 
      sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3]), -0.2e1) * (cos(theta[4]) * (sin(theta[
      0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[
      1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) - sin(theta[4]) * (l[6] - l[
      3]))) / (0.1e1 + pow(cos(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta
      [0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]
      ) * l[0]))) * l[1]) - sin(theta[4]) * (l[6] - l[3]) - cos(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3
      ]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + 
      cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) + sin(theta[5]) * (-l[6] + l[3]), 0.2e1) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - 
      acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(
      theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (
      sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0
      ], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5
      ]) * (-l[6] + l[3]), -0.2e1));

    J[3][5] = -((sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0
      ], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[
      2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) + cos(theta[5]) * (-l[6] + l[3])) / (l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] +
      cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - 
      sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0]
      + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1)
      + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3])) - (cos
      (theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[
      1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) - 
      sin(theta[4]) * (l[6] - l[3]) - cos(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(
      sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos
      (theta[3]) * l[0]))) * l[1]) + sin(theta[5]) * (-l[6] + l[3])) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]
      ) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]
      ) * l[0]) / (l[4] + 
      cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + 
      cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - 
      sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3]), -0.2e1) * (-cos(theta[5]) * (sin(theta
      [2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l
      [1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) + sin(theta[5]) * (-l[6] + 
      l[3]))) / (0.1e1 + pow(cos(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(
      theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[
      1]) * l[0]))) * l[1]) - sin(theta[4]) * (l[6] - l[3]) - cos(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta
      [3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + 
      cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) + sin(theta[5]) * (-l[6] + l[3]), 0.2e1) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - 
      acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((
      sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * 
      (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l
      [0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta
      [5]) * (-l[6] + l[3]), -0.2e1));

    J[4][0] = ((-sin(theta[0]) * l[0] - sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1
      ]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * (pow(pow(l[
      4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (-0.2e1 * (l[4] +
      cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) * sin(theta[0]) * l[0] + 0.2e1 * (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * cos(theta[0]) * l[0]) * pow(0.4e1
      - (pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1)
      / 0.2e1 + (cos(theta[0]) * l[0] / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) + (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * pow(l[4] + cos(
      theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1) * sin(theta[0]) * l[0]) / (0.1e1 + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1) * pow(l[4] + cos(
      theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1))) * l[1]) / (l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0]
      - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0])
      / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(
      pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]
      ) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3])) - (cos(theta[0]) * l[0] + 
      cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1]
      / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1] - cos(theta[2]) * l[0] - cos(PI 
      - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((
      sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[
      0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1)
      + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - 
      sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta
      [3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - 
      cos(theta[5]) * (-l[6] + l[3]), -0.2e1) * sin(theta[4]) * (cos(theta[0]) * l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1)
      + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) *
      l[0] - cos(theta[1]) * l[0]))) * (pow(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)
      , -0.1e1 / 0.2e1) / l[1] * (-0.2e1 * (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) * sin(theta[0]) * l[0] + 0.2e1 * (sin(theta[0]) * l[0] - sin(theta[1
      ]) * l[0]) * cos(theta[0]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * 
      l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (cos(theta[0]) * l[0] / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) + (sin(theta
      [0]) * l[0] - sin(theta[1]) * l[0]) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1) * sin(theta[0]) * l[0]) / (0.1e1 + pow(sin(theta[0]) * l
      [0] - sin(theta[1]) * l[0], 0.2e1) * pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1))) * l[1])) / (0.1e1 + pow(cos(theta[0]) * l[0] + cos(PI -
      acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((
      sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1] - cos(theta[2]) * l[0] - cos(PI - acos(sqrt(pow(l[4
      ] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0
      ] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1], 0.2e1) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI 
      - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((
      sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) 
      * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0],
      0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) 
      * (-l[6] + l[3]), -0.2e1));

    J[4][1] = (-sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(
      theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * (pow(
      pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (0.2e1 * (l
      [4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) * sin(theta[1]) * l[0] - 0.2e1 * (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * cos(theta[1]) * l[0]) * 
      pow(0.4e1 - (pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1
      / 0.2e1) / 0.2e1 + (-cos(theta[1]) * l[0] / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) - (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * pow(l[4]
      + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1) * sin(theta[1]) * l[0]) / (0.1e1 + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1) * pow(l[4] +
      cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1))) * l[1] / (l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * 
      l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * 
      l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos
      (sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(
      theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3])) - (cos(theta[0]) * l
      [0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1)
      + atan((sin(
      theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1] - cos(theta[2]) * l[0] - cos(PI - acos(sqrt(pow(l[4] + 
      cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - 
      sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(
      pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]
      ) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta
      [2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l
      [1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + 
      l[3]), -0.2e1) * sin(theta[4]) * cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) 
      * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * (pow(pow(l[4] 
      + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (0.2e1 * (l[4] + cos
      (theta[0]) * l[0] - cos(theta[1]) * l[0]) * sin(theta[1]) * l[0] - 0.2e1 * (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * cos(theta[1]) * l[0]) * pow(0.4e1 -
      (pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1
      ) / 0.2e1 + (-cos(theta[1]) * l[0] / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]) - (sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) * pow(l[4] + 
      cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1) * sin(theta[1]) * l[0]) / (0.1e1 + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1) * pow(l[4] + 
      cos(theta[0]) * l[0] - cos(theta[1]) * l[0], -0.2e1))) * l[1]) / (0.1e1 + pow(cos(theta[0]) * l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(
      theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4
      ] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1] - cos(theta[2]) * l[0] - cos(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 
      0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2])
      * l[0] - cos(theta[3]) * l[0]))) * l[1], 0.2e1) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(
      theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[
      4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l
      [4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l
      [0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3]), -0.2e1));

    J[4][2] = ((sin(
      theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) /
      l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * (pow(pow(l[4] + cos(theta[2]) * 
      l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (-0.2e1 * (l[4] + cos(theta[2]) * l[0]
      - cos(theta[3]) * l[0]) * sin(theta[2]) * l[0] + 0.2e1 * (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * cos(theta[2]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(
      theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (cos(
      theta[2]) * l[0] / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) + (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * pow(l[4] + cos(theta[2]) * l[0] - 
      cos(theta[3]) * l[0], -0.2e1) * sin(theta[2]) * l[0]) / (0.1e1 + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1) * pow(l[4] + cos(theta[2]) * l[0] - cos
      (theta[3]) * l[0], -0.2e1))) * l[1]) / (l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[
      0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta
      [0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(
      theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(
      theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3])) + (cos(theta[0]) * l[0] + cos(PI - acos(sqrt(
      pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] 
      / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1] - cos(theta[2]) * l[0] - cos(PI 
      - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((
      sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[
      0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1)
      + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - 
      sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta
      [3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - 
      cos(theta[5]) * (-l[6] + l[3]), -0.2e1) * sin(theta[5]) * (cos(theta[2]) * l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1)
      + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) *
      l[0] - cos(theta[3]) * l[0]))) * (pow(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)
      , -0.1e1 / 0.2e1) / l[1] * (-0.2e1 * (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) * sin(theta[2]) * l[0] + 0.2e1 * (sin(theta[2]) * l[0] - sin(theta[3
      ]) * l[0]) * cos(theta[2]) * l[0]) * pow(0.4e1 - (pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * 
      l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1) / 0.2e1 + (cos(theta[2]) * l[0] / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) + (sin(theta[
      2]) * l[0] - sin(theta[3]) * l[0]) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1) * sin(theta[2]) * l[0]) / (0.1e1 + pow(sin(theta[2]) * l[
      0] - sin(theta[3]) * l[0], 0.2e1) * pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1))) * l[1])) / (0.1e1 + pow(cos(theta[0]) * l[0] + cos(PI - 
      acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((
      sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1] - cos(theta[2]) * l[0] - cos(PI - acos(sqrt(pow(l[4
      ] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0
      ] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1], 0.2e1) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI 
      - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((
      sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) 
      * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0],
      0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) 
      * (-l[6] + l[3]), -0.2e1));

    J[4][3] = (sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(
      theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * (pow(
      pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (0.2e1 * (l[
      4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) * sin(theta[3]) * l[0] - 0.2e1 * (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * cos(theta[3]) * l[0]) * pow
      (0.4e1 - (pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1
      / 0.2e1) / 0.2e1 + (-cos(theta[3]) * l[0] / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) - (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * pow(l[4] 
      + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1) * sin(theta[3]) * l[0]) / (0.1e1 + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1) * pow(l[4] +
      cos(theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1))) * l[1] / (l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * 
      l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * 
      l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos
      (sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(
      theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3])) + (cos(theta[0]) * l
      [0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1)
      + atan((sin(
      theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1] - cos(theta[2]) * l[0] - cos(PI - acos(sqrt(pow(l[4] + 
      cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - 
      sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(
      pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0])
      * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2
      ]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1
      ] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[
      3]), -0.2e1) * sin(theta[5]) * cos(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * 
      l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * (pow(pow(l[4] + 
      cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1), -0.1e1 / 0.2e1) / l[1] * (0.2e1 * (l[4] + cos(
      theta[2]) * l[0] - cos(theta[3]) * l[0]) * sin(theta[3]) * l[0] - 0.2e1 * (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * cos(theta[3]) * l[0]) * pow(0.4e1 - (
      pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) * pow(l[1], -0.2e1), -0.1e1 / 0.2e1)
      / 0.2e1 + (-cos(theta[3]) * l[0] / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]) - (sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) * pow(l[4] + cos(
      theta[2]) * l[0] - cos(theta[3]) * l[0], -0.2e1) * sin(theta[3]) * l[0]) / (0.1e1 + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1) * pow(l[4] + cos(theta
      [2]) * l[0] - cos(theta[3]) * l[0], -0.2e1))) * l[1]) / (0.1e1 + pow(cos(theta[0]) * l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1])
      * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(
      theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1] - cos(theta[2]) * l[0] - cos(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + 
      pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] -
      cos(theta[3]) * l[0]))) * l[1], 0.2e1) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1])
      * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(
      theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(
      theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(
      theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3]), -0.2e1));

    J[4][4] = -(cos(theta[0]) * l
      [0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1)
      + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1] - cos(theta[2]) * l[0] - cos(PI - 
      acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(
      theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] +
      sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) +
      atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(
      theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) *
      l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(
      theta[5]) * (-l[6] + l[3]), -0.2e1) * (cos(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) +
      pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] -
      cos(theta[1]) * l[0]))) * l[1]) - sin(theta[4]) * (l[6] - l[3])) / (0.1e1 + pow(cos(theta[0]) * l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - 
      cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (
      l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1] - cos(theta[2]) * l[0] - cos(PI - acos(sqrt(pow(l[4] + 
      cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - 
      sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1], 0.2e1) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - 
      acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(
      theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (
      sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)
      ) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (
      -l[6] + l[3]), -0.2e1));

    J[4][5] = -(cos(theta[0]) * l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(
      theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1
      ]) * l[0]))) * l[1] - cos(theta[2]) * l[0] - cos(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - 
      sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[
      1]) * pow(l[5] + sin(theta[4]) * (sin(theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0])
      * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l
      [0]))) * l[1]) + cos(theta[4]) * (l[6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * 
      l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(
      theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) - cos(theta[5]) * (-l[6] + l[3]), -0.2e1) * (-cos(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l
      [4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l
      [0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1]) + sin(theta[5]) * (-l[6] + l[3])) / (0.1e1 + pow(cos(theta[0]) * 
      l[0] + cos(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1)) / l[1] / 0.2e1)
       + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1] - cos(theta[2]) * l[0] - cos(PI - 
      acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l[0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) 
      + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]))) * l[1], 0.2e1) * pow(l[5] + sin(theta[4]) * (sin
      (theta[0]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0], 0.2e1) + pow(sin(theta[0]) * l[0] - sin(theta[1]) * l[0], 0.2e1
      )) / l[1] / 0.2e1) + atan((sin(theta[0]) * l[0] - sin(theta[1]) * l[0]) / (l[4] + cos(theta[0]) * l[0] - cos(theta[1]) * l[0]))) * l[1]) + cos(theta[4]) * (l[
      6] - l[3]) - sin(theta[5]) * (sin(theta[2]) * l[0] + sin(PI - acos(sqrt(pow(l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0], 0.2e1) + pow(sin(theta[2]) * l
      [0] - sin(theta[3]) * l[0], 0.2e1)) / l[1] / 0.2e1) + atan((sin(theta[2]) * l[0] - sin(theta[3]) * l[0]) / (l[4] + cos(theta[2]) * l[0] - cos(theta[3]) * l[0]
      ))) * l[1]) - cos(theta[5]) * (-l[6] + l[3]), -0.2e1));

    tau[0] = J[0][0]*F[0] + J[1][0]*F[1] + J[2][0]*F[2] + J[3][0]*F[3] + J[4][0]*F[4];
	tau[1] = J[0][1]*F[0] + J[1][1]*F[1] + J[2][1]*F[2] + J[3][1]*F[3] + J[4][1]*F[4];
	tau[2] = J[0][2]*F[0] + J[1][2]*F[1] + J[2][2]*F[2] + J[3][2]*F[3] + J[4][2]*F[4];
	tau[3] = J[0][3]*F[0] + J[1][3]*F[1] + J[2][3]*F[2] + J[3][3]*F[3] + J[4][3]*F[4];
	tau[4] = J[0][4]*F[0] + J[1][4]*F[1] + J[2][4]*F[2] + J[3][4]*F[3] + J[4][4]*F[4];
	tau[5] = J[0][5]*F[0] + J[1][5]*F[1] + J[2][5]*F[2] + J[3][5]*F[3] + J[4][5]*F[4]; 
}

/*
    Limit the motor currents such that the thermal rating for the motors is not exceeded
    while continuing to provide the peak torque when necessary.
*/
static void 
limit_currents(struct limiter_state current_limiters[NUM_JOINTS], double dt, double motor_currents[NUM_JOINTS])
{
    static const double current_limit_1[NUM_JOINTS]  = {LIMIT_1_SMALL,   LIMIT_1_SMALL,   LIMIT_1_SMALL,   LIMIT_1_SMALL,   LIMIT_1_LARGE,   LIMIT_1_LARGE};
    static const double current_limit_2[NUM_JOINTS]  = {LIMIT_2_SMALL,   LIMIT_2_SMALL,   LIMIT_2_SMALL,   LIMIT_2_SMALL,   LIMIT_2_LARGE,   LIMIT_2_LARGE};
    static const double timeout_1[NUM_JOINTS]        = {TIMEOUT_1_SMALL, TIMEOUT_1_SMALL, TIMEOUT_1_SMALL, TIMEOUT_1_SMALL, TIMEOUT_1_LARGE, TIMEOUT_1_LARGE};
    static const double timeout_2[NUM_JOINTS]        = {TIMEOUT_2_SMALL, TIMEOUT_2_SMALL, TIMEOUT_2_SMALL, TIMEOUT_2_SMALL, TIMEOUT_2_LARGE, TIMEOUT_2_LARGE};

    int index;
    for (index = NUM_JOINTS-1; index >= 0; --index)
    {
        double value = fabs(motor_currents[index]);

        switch (current_limiters[index].state)
        {
        case 0: // Limiting to the upper limits and waiting for input to exceed lower limits
            if (value > current_limit_2[index])
            {
                current_limiters[index].state = 1;
                current_limiters[index].time  = 0;
                current_limiters[index].count = 0;
                current_limiters[index].mean  = value;

                if (value > current_limit_1[index])
                    motor_currents[index] = (motor_currents[index] > 0) ? current_limit_1[index] : -current_limit_1[index];
                else
                    motor_currents[index] = motor_currents[index];
            }
            else
                motor_currents[index] = motor_currents[index];

            break;

        case 1: // Limiting to the upper limits and waiting for peak time limit
            current_limiters[index].mean += value;
            current_limiters[index].time += dt;
            current_limiters[index].count++;

            if (current_limiters[index].time >= timeout_1[index])
            {
                current_limiters[index].mean /= current_limiters[index].count + 1;
                if (current_limiters[index].mean > current_limit_2[index])
                {
                    current_limiters[index].state = 2;
                    current_limiters[index].time  = 0;
                    current_limiters[index].count = 0;
                    motor_currents[index] = (motor_currents[index] > 0) ? current_limit_2[index] : -current_limit_2[index];
                }
                else
                {
                    current_limiters[index].state = 0;
                    motor_currents[index] = motor_currents[index];
                }
            }
            else if (value > current_limit_1[index])
                motor_currents[index] = (motor_currents[index] > 0) ? current_limit_1[index] : -current_limit_1[index];
            else
                motor_currents[index] = motor_currents[index];

            break;

        case 2: // Limiting to the lower limits and waiting for motors to recover
            current_limiters[index].time += dt;
            current_limiters[index].count++;

            if (current_limiters[index].time >= timeout_2[index])
            {
                if (value > current_limit_2[index])
                {
                    current_limiters[index].state = 1;
                    current_limiters[index].time  = 0;
                    current_limiters[index].count = 0;
                    current_limiters[index].mean  = value;

                    if (value > current_limit_1[index])
                        motor_currents[index] = (motor_currents[index] > 0) ? current_limit_1[index] : -current_limit_1[index];
                    else
                        motor_currents[index] = motor_currents[index];
                }
                else
                {
                    current_limiters[index].state = 0;
                    motor_currents[index] = motor_currents[index];
                }
            }
            else if (value > current_limit_2[index])
                motor_currents[index] = (motor_currents[index] > 0) ? current_limit_2[index] : -current_limit_2[index];
            else
                motor_currents[index] = motor_currents[index];

            break;
        }
    }
}

/*
    Determine the motor currents in amps needed to produce the specified joint torques in N-m.
*/
static void
joint_torques_to_motor_currents(const double joint_torques[NUM_JOINTS], double motor_currents[NUM_JOINTS])
{
    static const double torque_constants[NUM_JOINTS] = {KT_SMALL, KT_SMALL, KT_SMALL, KT_SMALL, KT_LARGE, KT_LARGE};

    int i;
    for (i=NUM_JOINTS-1; i >= 0; --i)
        motor_currents[i] = joint_torques[i] / torque_constants[i]; // convert N-m to amps
}

/*
    Determine the voltages with which we need to drive the current amplifiers
    to get the motor currents desired.
*/
static void
motor_currents_to_output_voltages(const double motor_currents[NUM_JOINTS], double voltages[NUM_JOINTS])
{
    int i;
    for (i=NUM_JOINTS-1; i >= 0; --i)
        voltages[i] = 0.5*motor_currents[i];

    voltages[2] = -voltages[2];
    voltages[3] = -voltages[3];
}

/*
    Disable the motor amplifiers and drive the motors with zero current.
*/
static void
disable_wand(t_card board)
{
    static const t_double  zero_torques[NUM_JOINTS]       = { 0, 0, 0, 0, 0, 0 };
    static const t_boolean disable_amplifiers[NUM_JOINTS] = { 0, 0, 0, 0, 0, 0 };

    /* Make sure the amplifiers are disabled and the motors programmed for zero torque */
	hil_set_digital_directions(board, NULL, 0, digital_channels, ARRAY_LENGTH(digital_channels));   /* program digital I/O lines as outputs */
	hil_write_digital(board, digital_channels, ARRAY_LENGTH(digital_channels), disable_amplifiers); /* disable amplifiers */
    hil_write_analog(board, analog_channels, ARRAY_LENGTH(analog_channels), zero_torques);          /* drive the motors with 0 V */
}

/*
    Enable a motor amplifiers.
*/
static void
enable_wand(t_card board)
{
    static const t_boolean enable_amplifiers[NUM_JOINTS]  = { 1, 1, 1, 1, 1, 1 };

    /* Enable the amplifiers */
	hil_write_digital(board, digital_channels, ARRAY_LENGTH(digital_channels), enable_amplifiers); /* enable amplifiers */
}

/*
    Calibrate the haptic wand. The user is expected to press Enter to begin the calibration procedure.
*/
static int
calibrate_wand(t_card board)
{
    static t_int32 counts[NUM_JOINTS] = { 0, 0, 0, 0, 0, 0 };
    int result;

    disable_wand(board);

    printf("Please place the Quanser 5DOF Haptic Wand in the calibration position.\n");
    printf("Then press Enter to calibrate the Haptic Wand.\n");
    getchar();

    result = hil_set_encoder_counts(board, encoder_channels, ARRAY_LENGTH(encoder_channels), counts); /* reset the encoder counts to zero */
    if (result == 0)
        return 1;
    else
    {
        msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
        printf("Unable to calibrate the haptic wand. %s Error %d.\n", message, -result);
    }
    return 0;
}

/*
    Stop the controller for the haptic wand.
*/
static void
stop_controller(t_card board, t_task task)
{
    /* Stop the controller */
    hil_task_stop(task);

    /* Turn off motors */
    disable_wand(board);

    /* Delete the task */
    hil_task_delete(task);

    printf("The motors have been disabled and the controller has been stopped.\n");
}

/*
    Start the controller for the haptic wand.
*/
static int
start_controller(t_card board, t_task * task_pointer)
{
    static const t_uint32 samples           = -1; /* read continuously */
    const t_uint32        samples_in_buffer = (t_uint32)(0.1 * frequency);

    qsched_param_t scheduling_parameters;
    int result;

    /* Create a task to read the encoders at regular intervals */
    result = hil_task_create_encoder_reader(board, samples_in_buffer, encoder_channels, ARRAY_LENGTH(encoder_channels), task_pointer);
    if (result == 0)
    {
        enable_wand(board);

        printf("The motors have been enabled. Starting the controller...\n");

        /* Bump up the priority of our thread to minimize latencies */
        scheduling_parameters.sched_priority = qsched_get_priority_max(QSCHED_FIFO);
        qthread_setschedparam(qthread_self(), QSCHED_FIFO, &scheduling_parameters);

        /* Start the controller at the chosen sampling rate. */
        result = hil_task_start(*task_pointer, SYSTEM_CLOCK_1, frequency, samples);
        if (result == 0)
            return 1;
        else
        {
            msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
            printf("Unable to start task. %s Error %d.\n", message, -result);
        }

        /* Disable the amplifiers and delete the task */
        stop_controller(board, *task_pointer);
    }
    else
    {
        msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
        printf("Unable to create a task. %s Error %d.\n", message, -result);
    }

    return 0;
}

/*
    Drive the haptic wand motors to produce the desired world-space generalized forces.
    Motor currents are dynamically limited heuristically to prevent overheating of the motors
    while continuing to provide peak torque.
*/
static void
generate_forces(t_card board, struct limiter_state current_limiters[NUM_JOINTS], double period,
                const double joint_angles[NUM_JOINTS], const double world_forces[NUM_WORLD])
{
    double joint_torques[NUM_JOINTS];       /* joint torques in N-m */
    double motor_currents[NUM_JOINTS];      /* motor currents in amps */
    double output_voltages[NUM_JOINTS];     /* output voltages in volts */

    /* Compute the output voltages needed to produce the desired world-space generalized forces at the end-effector */
    inverse_force_kinematics(joint_angles, world_forces, joint_torques);    /* convert generalized forces to joint torques */
    joint_torques_to_motor_currents(joint_torques, motor_currents);         /* convert joint torques to motor currents */
    limit_currents(current_limiters, period, motor_currents);               /* limit motor currents to prevent overheating */
    motor_currents_to_output_voltages(motor_currents, output_voltages);     /* compute output voltages required to produce the motor currents */

    /* Write the voltages to the outputs */
    hil_write_analog(board, analog_channels, ARRAY_LENGTH(analog_channels), output_voltages);
}

/*
    The main entry point for the program.
*/
int main(int argc, char * argv[])
{
    qsigaction_t action;
	t_card board;
    t_int  result;

    /* Catch Ctrl+C so application may shut down cleanly */
    action.sa_handler = signal_handler;
    action.sa_flags   = 0;
    qsigemptyset(&action.sa_mask);

    qsigaction(SIGINT, &action, NULL);

    printf("This example controls the Quanser 5DOF Haptic Wand at %g Hz.\n", frequency);
    
    /* Open the board that controls the haptic wand */
    result = hil_open(board_type, board_identifier, &board);
    if (result == 0)
    {
        printf("Press CTRL-C to stop the controller.\n\n");

        if (calibrate_wand(board))
        {
            const t_double period = 1.0 / frequency;

            t_int32  counts[NUM_JOINTS];
            t_int    samples_read;
            t_task   task;

            printf("The Quanser 5DOF Haptic Wand has been calibrated. Remove the Wand\n");
            printf("from the calibration position and press Enter to start the controller.\n");
            getchar();

            if (start_controller(board, &task))
            {
                const double k[NUM_WORLD]    = { 0, 0, 0, 0, 0 }; /* set elements to get springs in different world coordinates */
                const double home[NUM_WORLD] = { 0.25, 0, 0, 0, 0 }; /* home position, in front of calibration position */

                struct limiter_state current_limiters[NUM_JOINTS];
                memset(current_limiters, 0, sizeof(current_limiters));

                samples_read = hil_task_read_encoder(task, 1, counts); /* read one sample of the encoders */
                while (samples_read > 0 && stop == 0)
                {
                    double joint_angles[NUM_JOINTS];        /* joint angles in radians */
                    double world_coordinates[NUM_WORLD];    /* world coordinates X, Y, Z in m and roll (about X) and pitch (about Y) in radians */
                    double world_forces[NUM_WORLD];         /* world forces in N and world torques in N-m */
                    int i;

                    /* Compute the world-space coordinates for the end-effector */
                    encoder_counts_to_joint_angles(counts, joint_angles);
                    forward_kinematics(joint_angles, world_coordinates);

                    /* Compute forces in world coordinates (simple spring in this example) */
                    for (i = 0; i < NUM_WORLD; i++)
                        world_forces[i] = -k[i] * (world_coordinates[i] - home[i]);
                    
                    /* Drive the motors to produce the desired world-space forces and torques */
                    generate_forces(board, current_limiters, period, joint_angles, world_forces);

                    /* Prepare for the next sampling instant */
                    samples_read = hil_task_read_encoder(task, 1, counts);  /* read the encoders for the next sampling instant */
                }

                stop_controller(board, task);

                if (samples_read < 0)
                {
                    msg_get_error_message(NULL, samples_read, message, ARRAY_LENGTH(message));
                    printf("Unable to read encoder channels. %s Error %d.\n", message, -samples_read);
                }
                else
                {
                    printf("\nController has been stopped. Press Enter to continue.\n");
                    getchar(); /* absorb Ctrl-C */
                }
            }
        }

        hil_close(board);
    }
    else
    {
        msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
        printf("Unable to open board. %s Error %d.\n", message, -result);
    }

    return 0;
}
