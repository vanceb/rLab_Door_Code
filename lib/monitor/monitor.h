#ifndef MONITOR_H
#define MONITOR_H

#define LOOP_FREQ   (50)  // How many times per second should the main loop run?
#define DOOR_OPEN_MAX_MS    (5000)

/* Voltage Measurement */
/* Conversion factors */
#define VIN_FACTOR          (75)
#define VBATT_FACTOR        (115)
#define V5_FACTOR           (359)
#define V3_FACTOR           (553)
/* Voltage limits */
#define VIN_LOW             (17.5)
#define VIN_HI              (25.0)
#define VBATT_LOW           (12.0)
#define VBATT_CHARGE        (13.5)
#define VBATT_HI            (14.0)
#define V5_LOW              (4.0)
#define V5_HI               (6.0)
#define V3_LOW              (3.0)
#define V3_HI               (3.5)
/* Voltage Error Bits */
#define ERR_V3_LOW          0x01
#define ERR_V3_HI           0x02
#define ERR_V5_LOW          0x04
#define ERR_V5_HI           0x08
#define ERR_VBATT_LOW       0x10
#define ERR_VBATT_HI        0x20
#define ERR_VIN_LOW         0x40
#define ERR_VIN_HI          0x80

void monitorTask( void * pvParameters);


#endif