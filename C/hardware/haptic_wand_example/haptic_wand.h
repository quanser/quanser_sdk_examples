#ifndef _haptic_wand_h
#define _haptic_wand_h

#define NUM_JOINTS  (6)			/* number of joints */
#define NUM_WORLD   (5)	        /* number of world coordinates */

#define L1          (5.670       * 0.0254)  /* length of link 1 in meters */
#define L2          (7.717       * 0.0254)  /* length of link 2 in meters */
#define L3          (8.32232120  * 0.0254)  /* length of link 3 in meters */
#define L4          (1.417202685 * 0.0254)  /* length of link 4 in meters */
#define L5          (1.750       * 0.0254)  /* length of link 5 in meters */
#define L6          (7.000       * 0.0254)  /* length of link 6 in meters */
#define L7          (2.078363285 * 0.0254)  /* length of link 7 in meters */

#define KT_SMALL    (52.5e-3)   /* N-m per amp */
#define KT_LARGE    (191e-3)    /* N-m per amp */

#define LIMIT_1_SMALL   (7.00)  /* current limits in amps */
#define LIMIT_2_SMALL   (2.15)  /* current limits in amps */
#define LIMIT_1_LARGE   (5.00)  /* current limits in amps */
#define LIMIT_2_LARGE   (1.69)  /* current limits in amps */

#define TIMEOUT_1_SMALL   ( 0.2)    /* timeout in seconds */
#define TIMEOUT_2_SMALL   (10.0)    /* timeout in seconds */
#define TIMEOUT_1_LARGE   ( 0.2)    /* timeout in seconds */
#define TIMEOUT_2_LARGE   (10.0)    /* timeout in seconds */

#define PI          (3.1415926535897932384626433832795)

#endif
