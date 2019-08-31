/*
 * gps.h
 *
 *  Created on: Aug 22, 2019
 *      Author: junaidkhan
 */

#ifndef GPS_H_
#define GPS_H_

#include <stdint.h>
#include <stddef.h>



/**
 * \brief           Enables `1` or disables `0` `GGA` statement parsing.
 *
 * \note            This statement must be enabled to parse:
 *                      - Latitude, Longitude, Altitude
 *                      - Number of satellites in use, fix (no fix, GPS, DGPS), UTC time
 */
#ifndef GPS_CFG_STATEMENT_GPGGA
#define GPS_CFG_STATEMENT_GPGGA             1
#endif

/**
 * \brief           Enables `1` or disables `0` `RMC` statement parsing.
 *
 * \note            This statement must be enabled to parse:
 *                      - Validity of GPS signal
 *                      - Ground speed in knots and coarse in degrees
 *                      - Magnetic variation
 *                      - UTC date
 */
#ifndef GPS_CFG_STATEMENT_GPRMC
#define GPS_CFG_STATEMENT_GPRMC             1
#endif

/**
 * \brief           GPS float definition `float`
 *
 */
typedef double gps_float_t;

/**
 *   GPS structure
 */
typedef struct{

        /* Information related to GPGGA statement */
        gps_float_t latitude;                       /*!< Latitude in units of degrees */
        gps_float_t longitude;                      /*!< Longitude in units of degrees */
        gps_float_t altitude;                       /*!< Altitude in units of meters */
        gps_float_t geo_sep;                        /*!< Geoid separation in units of meters */
        uint8_t sats_in_use;                        /*!< Number of satellites in use */
        uint8_t fix;                                /*!< Fix status. `0` = invalid, `1` = GPS fix, `2` = DGPS fix, `3` = PPS fix */
        uint8_t hours;                              /*!< Hours in UTC */
        uint8_t minutes;                            /*!< Minutes in UTC */
        uint8_t seconds;                            /*!< Seconds in UTC */

        /* Information related to GPRMC statement */
          uint8_t is_valid;                           /*!< GPS valid status */
          gps_float_t speed;                          /*!< Ground speed in knots */
          gps_float_t coarse;                         /*!< Ground coarse */
          gps_float_t variation;                      /*!< Magnetic variation */
          uint8_t date;                               /*!< Fix date */
          uint8_t month;                              /*!< Fix month */
          uint8_t year;                               /*!< Fix year */

    struct {
        uint8_t stat;                           /*!< Statement index */
        char term_str[13];                      /*!< Current term in string format */
        uint8_t term_pos;                       /*!< Current index position in term */
        uint8_t term_num;                       /*!< Current term number */
        uint8_t star;                           /*!< Star detected flag */
        uint8_t crc_calc;                       /*!< Calculated CRC string */ // CRC = cyclic redundacy Checksum, used for checking integrity of data being transferred.

        union{
                uint8_t dummy;                      /*!< Dummy byte */
                struct {
                    gps_float_t latitude;           /*!< GPS latitude position in degrees */
                    gps_float_t longitude;          /*!< GPS longitude position in degrees */
                    gps_float_t altitude;           /*!< GPS altitude in meters */
                    gps_float_t geo_sep;            /*!< Geoid separation in units of meters */
                    uint8_t sats_in_use;            /*!< Number of satellites currently in use */
                    uint8_t fix;                    /*!< Type of current fix, `0` = Invalid, `1` = GPS fix, `2` = Differential GPS fix */
                    uint8_t hours;                  /*!< Current UTC hours */
                    uint8_t minutes;                /*!< Current UTC minutes */
                    uint8_t seconds;                /*!< Current UTC seconds */
                } gga;                              /*!< GPGGA message */
                struct{
                   uint8_t is_valid;               /*!< Status whether GPS status is valid or not */
                   uint8_t date;                   /*!< Current UTF date */
                   uint8_t month;                  /*!< Current UTF month */
                   uint8_t year;                   /*!< Current UTF year */
                   gps_float_t speed;              /*!< Current spead over the ground in knots */
                   gps_float_t coarse;             /*!< Current coarse made good */
                   gps_float_t variation;          /*!< Current magnetic variation in degrees */
               } rmc;                              /*!< GPRMC message */

        } data;                                    /*!< Union with data for each information */
    } p;                                           /*!< Structure with private data */
}gps_t;

/*
 *  GPS Module Prototype Functions
 */

uint8_t     gps_init(gps_t* gh);
uint8_t     gps_process(gps_t* gh, const void* data, size_t len);



#endif /* GPS_H_ */
