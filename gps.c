/*
 * gps.c
 *
 *  Created on: Aug 22, 2019
 *      Author: junaidkhan
 */

#include "gps.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

#define STAT_UNKNOWN        0
#define STAT_GGA            1
#define STAT_RMC            4

#define CRC_ADD(_gh, ch)    (_gh)->p.crc_calc ^= (uint8_t)(ch)
#define TERM_ADD(_gh, ch)   do {    \
    if ((_gh)->p.term_pos < (sizeof((_gh)->p.term_str) - 1)) {  \
        (_gh)->p.term_str[(_gh)->p.term_pos++] = (ch);  \
        (_gh)->p.term_str[(_gh)->p.term_pos] = 0;   \
    }                               \
} while (0)

#define CIN(x)              ((x) >= '0' && (x) <= '9')
#define CTN(x)              ((x) - '0')
#define CHTN(x)             (((x) >= '0' && (x) <= '9') ? ((x) - '0') : (((x) >= 'a' && (x) <= 'z') ? ((x) - 'a' + 10) : (((x) >= 'A' && (x) <= 'Z') ? ((x) - 'A' + 10) : 0)))
#define TERM_NEXT(_gh)      do { (_gh)->p.term_str[((_gh)->p.term_pos = 0)] = 0; (_gh)->p.term_num++; } while (0)
#define FLT(x)              ((gps_float_t)(x))

/**
 * \brief           Compare calculated CRC with received CRC
 * \param[in]       gh: GPS handle
 * \return          `1` on success, `0` otherwise
 */
static uint8_t
check_crc(gps_t* gh) {
    uint8_t crc;
    crc = (uint8_t)((CHTN(gh->p.term_str[0]) & 0x0F) << 0x04) | (CHTN(gh->p.term_str[1]) & 0x0F);   /* Convert received CRC from string (hex) to number */
    return gh->p.crc_calc == crc;               /* They must match! */
 }


/**
 * \brief           Parse number as integer
 * \param[in]       gh: GPS handle
 * \param[in]       t: Text to parse. Set to `NULL` to parse current GPS term
 * \return          Parsed integer
 */
static int32_t
parse_number(gps_t* gh, const char* t) {
    int32_t res = 0;
    uint8_t minus;

    if (t == NULL) {
        t = gh->p.term_str;
    }
    for (; t != NULL && *t == ' '; t++) {}      /* Strip leading spaces */

    minus = (*t == '-' ? (t++, 1) : 0);
    for (; t != NULL && CIN(*t); t++) {
        res = 10 * res + CTN(*t);
    }
    return minus ? -res : res;
}


/**
 * \brief           Parse number as double and convert it to \ref gps_float_t
 * \param[in]       gh: GPS handle
 * \param[in]       t: Text to parse. Set to `NULL` to parse current GPS term
 * \return          Parsed double in \ref gps_float_t format
 */
static gps_float_t
parse_float_number(gps_t* gh, const char* t) {
    gps_float_t res;

       if (t == NULL) {
           t = gh->p.term_str;
       }
       for (; t != NULL && *t == ' '; t++) {}      /* Strip leading spaces */
       res = strtof(t, NULL);                      /* Parse string to float */

       return FLT(res);                            /* Return casted value, based on float size */
}


/**
 * \brief           Parse latitude/longitude NMEA format to double
 *
 *                  NMEA output for latitude is ddmm.sss and longitude is dddmm.sss
 * \param[in]       gh: GPS handle
 * \return          Latitude/Longitude value in degrees
 */
static gps_float_t
parse_lat_long(gps_t* gh) {
    gps_float_t ll, deg, min;

    ll = parse_float_number(gh, NULL);          /* Parse value as double */
    deg = FLT((int)((int)ll / 100));            /* Get absolute degrees value, interested in integer part only */
    min = ll - (deg * FLT(100));                /* Get remaining part from full number, minutes */
    ll = deg + (min / FLT(60.0));               /* Calculate latitude/longitude */

    return ll;
}

/**
 * \brief           Parse received term
 * \param[in]       gh: GPS handle
 * \return          `1` on success, `0` otherwise
 */
static uint8_t
parse_term(gps_t* gh) {
    if (gh->p.term_num == 0) {                  /* Check string type */
        if (0) {
#if GPS_CFG_STATEMENT_GPGGA
        } else if (!strncmp(gh->p.term_str, "$GPGGA", 6) || !strncmp(gh->p.term_str, "$GNGGA", 6)) {
            gh->p.stat = STAT_GGA;
#endif /* GPS_CFG_STATEMENT_GPGGA */
#if GPS_CFG_STATEMENT_GPRMC
        } else if (!strncmp(gh->p.term_str, "$GPRMC", 6) || !strncmp(gh->p.term_str, "$GNRMC", 6)) {
            gh->p.stat = STAT_RMC;
#endif /* GPS_CFG_STATEMENT_GPRMC */
        } else {
            gh->p.stat = STAT_UNKNOWN;          /* Invalid statement for library */
        }
        return 1;
    }

    /* Start parsing terms */
      if (gh->p.stat == STAT_UNKNOWN) {
#if GPS_CFG_STATEMENT_GPGGA
    } else if (gh->p.stat == STAT_GGA) {        /* Process GPGGA statement */
        switch (gh->p.term_num) {
            case 1:                             /* Process UTC time */
                gh->p.data.gga.hours = 10 * CTN(gh->p.term_str[0]) + CTN(gh->p.term_str[1]);
                gh->p.data.gga.minutes = 10 * CTN(gh->p.term_str[2]) + CTN(gh->p.term_str[3]);
                gh->p.data.gga.seconds = 10 * CTN(gh->p.term_str[4]) + CTN(gh->p.term_str[5]);
                break;
            case 2:                             /* Latitude */
                gh->p.data.gga.latitude = parse_lat_long(gh);   /* Parse latitude */
                break;
            case 3:                             /* Latitude north/south information */
                if (gh->p.term_str[0] == 'S' || gh->p.term_str[0] == 's') {
                    gh->p.data.gga.latitude = -gh->p.data.gga.latitude;
                }
                break;
            case 4:                             /* Longitude */
                gh->p.data.gga.longitude = parse_lat_long(gh);  /* Parse longitude */
                break;
            case 5:                             /* Longitude east/west information */
                if (gh->p.term_str[0] == 'W' || gh->p.term_str[0] == 'w') {
                    gh->p.data.gga.longitude = -gh->p.data.gga.longitude;
                }
                break;
            case 6:                             /* Fix status */
                gh->p.data.gga.fix = (uint8_t)parse_number(gh, NULL);
                break;
            case 7:                             /* Satellites in use */
                gh->p.data.gga.sats_in_use = (uint8_t)parse_number(gh, NULL);
                break;
            case 9:                             /* Altitude */
                gh->p.data.gga.altitude = parse_float_number(gh, NULL);
                break;
            case 11:                            /* Altitude above ellipsoid */
                gh->p.data.gga.geo_sep = parse_float_number(gh, NULL);
                break;
            default: break;
        }
#endif /* GPS_CFG_STATEMENT_GPGGA */
#if GPS_CFG_STATEMENT_GPRMC
      } else if (gh->p.stat == STAT_RMC) {        /* Process GPRMC statement */
              switch (gh->p.term_num) {
                  case 2:                             /* Process valid status */
                      gh->p.data.rmc.is_valid = (gh->p.term_str[0] == 'A');
                      break;
                  case 7:                             /* Process ground speed in knots */
                      gh->p.data.rmc.speed = parse_float_number(gh, NULL);
                      break;
                  case 8:                             /* Process true ground coarse */
                      gh->p.data.rmc.coarse = parse_float_number(gh, NULL);
                      break;
                  case 9:                             /* Process date */
                      gh->p.data.rmc.date = (uint8_t)(10 * CTN(gh->p.term_str[0]) + CTN(gh->p.term_str[1]));
                      gh->p.data.rmc.month = (uint8_t)(10 * CTN(gh->p.term_str[2]) + CTN(gh->p.term_str[3]));
                      gh->p.data.rmc.year = (uint8_t)(10 * CTN(gh->p.term_str[4]) + CTN(gh->p.term_str[5]));
                      break;
                  case 10:                            /* Process magnetic variation */
                      gh->p.data.rmc.variation = parse_float_number(gh, NULL);
                      break;
                  case 11:                            /* Process magnetic variation east/west */
                      if (gh->p.term_str[0] == 'W' || gh->p.term_str[0] == 'w') {
                          gh->p.data.rmc.variation = -gh->p.data.rmc.variation;
                      }
                      break;
                  default: break;
              }
      #endif /* GPS_CFG_STATEMENT_GPRMC */
          }
      return 1;
}

/**
 * \brief           Copy temporary memory to user memory
 * \param[in]       gh: GPS handle
 * \return          `1` on success, `0` otherwise
 */
static uint8_t
copy_from_tmp_memory(gps_t* gh) {
    if (0) {
#if GPS_CFG_STATEMENT_GPGGA
    } else if (gh->p.stat == STAT_GGA) {
        gh->latitude = gh->p.data.gga.latitude;
        gh->longitude = gh->p.data.gga.longitude;
        gh->altitude = gh->p.data.gga.altitude;
        gh->geo_sep = gh->p.data.gga.geo_sep;
        gh->sats_in_use = gh->p.data.gga.sats_in_use;
        gh->fix = gh->p.data.gga.fix;
        gh->hours = gh->p.data.gga.hours;
        gh->minutes = gh->p.data.gga.minutes;
        gh->seconds = gh->p.data.gga.seconds;
#endif /* GPS_CFG_STATEMENT_GPGGA */
#if GPS_CFG_STATEMENT_GPRMC
    } else if (gh->p.stat == STAT_RMC) {
        gh->coarse = gh->p.data.rmc.coarse;
        gh->is_valid = gh->p.data.rmc.is_valid;
        gh->speed = gh->p.data.rmc.speed;
        gh->variation = gh->p.data.rmc.variation;
        gh->date = gh->p.data.rmc.date;
        gh->month = gh->p.data.rmc.month;
        gh->year = gh->p.data.rmc.year;
#endif /* GPS_CFG_STATEMENT_GPRMC */
    }
    return 1;
}


/**
 * \brief           Init GPS handle
 * \param[in]       gh: GPS handle structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t
gps_init(gps_t* gh) {
    memset(gh, 0x00, sizeof(*gh));              /* Reset structure */
    return 1;                                  /* memset copies the 'unsigned character '0' 'to the first '(*gh)' characters of the string pointed to gh*/

}

/**
 * \brief           Process NMEA data from GPS receiver
 * \param[in]       gh: GPS handle structure
 * \param[in]       data: Received data
 * \param[in]       len: Number of bytes to process
 * \return          `1` on success, `0` otherwise
 */
uint8_t
gps_process(gps_t* gh, const void* data, size_t len){
    const uint8_t* d = data;

    while (len--) {                                     /* Process all bytes */
        if (*d == '$'){                                 /* Check for beginning of NMEA line */
                    memset(&gh->p, 0x00, sizeof(gh->p));/* Reset private memory */
                    TERM_ADD(gh, *d);                   /* Add character to term */
        }else if (*d == ',') {                         /* Term separator character */
                parse_term(gh);                        /* Parse term we have currently in memory */
                CRC_ADD(gh, *d);                       /* Add character to CRC computation */
                TERM_NEXT(gh);                         /* Start with next term */
        }else if (*d == '*') {                         /* Start indicates end of data for CRC computation */
            parse_term(gh);                            /* Parse term we have currently in memory */
            gh->p.star = 1;                            /* STAR detected */
            TERM_NEXT(gh);                             /* Start with next term */
        }else if (*d == '\r') {
            if (check_crc(gh)){                        /* Check for CRC result */
                /* CRC is OK, in theory we can copy data from statements to user data */
                copy_from_tmp_memory(gh);               /* Copy memory from temporary to user memory */
            }
        }else {
            if (!gh->p.star){                  /* Add to CRC only if star not yet detected */
                        CRC_ADD(gh, *d);                /* Add to CRC */
                    }
                TERM_ADD(gh, *d);                   /* Add character to term */
            }
            d++;                                    /* Process next character */
        }
        return 1;
}

