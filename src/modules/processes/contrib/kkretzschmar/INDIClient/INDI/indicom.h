//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 02.01.07.0873
// ----------------------------------------------------------------------------
// Standard INDIClient Process Module Version 01.00.15.0217
// ----------------------------------------------------------------------------
// indicom.h - Released 2017-08-01T14:26:58Z
// ----------------------------------------------------------------------------
// This file is part of the standard INDIClient PixInsight module.
//
// Copyright (c) 2014-2017 Klaus Kretzschmar
//
// Redistribution and use in both source and binary forms, with or without
// modification, is permitted provided that the following conditions are met:
//
// 1. All redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. All redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the names "PixInsight" and "Pleiades Astrophoto", nor the names
//    of their contributors, may be used to endorse or promote products derived
//    from this software without specific prior written permission. For written
//    permission, please contact info@pixinsight.com.
//
// 4. All products derived from this software, in any form whatsoever, must
//    reproduce the following acknowledgment in the end-user documentation
//    and/or other materials provided with the product:
//
//    "This product is based on software from the PixInsight project, developed
//    by Pleiades Astrophoto and its contributors (http://pixinsight.com/)."
//
//    Alternatively, if that is where third-party acknowledgments normally
//    appear, this acknowledgment must be reproduced in the product itself.
//
// THIS SOFTWARE IS PROVIDED BY PLEIADES ASTROPHOTO AND ITS CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL PLEIADES ASTROPHOTO OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, BUSINESS
// INTERRUPTION; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; AND LOSS OF USE,
// DATA OR PROFITS) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// ----------------------------------------------------------------------------

/*
    INDI LIB
    Common routines used by all drivers
    Copyright (C) 2003 by Jason Harris (jharris@30doradus.org)
    			  Elwood C. Downey
			  Jasem Mutlaq

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

/** \file indicom.h
    \brief Implementations for common driver routines.

    The INDI Common Routine Library provides formatting and serial routines employed by many INDI drivers. Currently, the library is composed of the following sections:

    <ul>
    <li>Formatting Functions</li>
    <li>Conversion Functions</li>
    <li>TTY Functions</li>


    </ul>

    \author Jason Harris
    \author Elwood C. Downey
    \author Jasem Mutlaq
*/

#ifndef INDICOM_H
#define INDICOM_H

#include <time.h>

#define J2000 2451545.0
#define ERRMSG_SIZE 1024
#define INDI_DEBUG

extern const char * Direction[];
extern const char * SolarSystem[];

struct ln_date;

/* TTY Error Codes */
enum TTY_ERROR { TTY_OK=0, TTY_READ_ERROR=-1, TTY_WRITE_ERROR=-2, TTY_SELECT_ERROR=-3, TTY_TIME_OUT=-4, TTY_PORT_FAILURE=-5, TTY_PARAM_ERROR=-6, TTY_ERRNO = -7};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup ttyFunctions TTY Functions: Functions to perform common terminal access routines.
*/

/*@{*/

/** \brief read buffer from terminal
    \param fd file descriptor
    \param buf pointer to store data. Must be initilized and big enough to hold data.
    \param nbytes number of bytes to read.
    \param timeout number of seconds to wait for terminal before a timeout error is issued.
    \param nbytes_read the number of bytes read.
    \return On success, it returns TTY_OK, otherwise, a TTY_ERROR code.
*/
int tty_read(int fd, char *buf, int nbytes, int timeout, int *nbytes_read);

/** \brief read buffer from terminal with a delimiter
    \param fd file descriptor
    \param buf pointer to store data. Must be initilized and big enough to hold data.
    \param stop_char if the function encounters \e stop_char then it stops reading and returns the buffer.
    \param timeout number of seconds to wait for terminal before a timeout error is issued.
    \param nbytes_read the number of bytes read.
    \return On success, it returns TTY_OK, otherwise, a TTY_ERROR code.
*/

int tty_read_section(int fd, char *buf, char stop_char, int timeout, int *nbytes_read);


/** \brief Writes a buffer to fd.
    \param fd file descriptor
    \param buffer a null-terminated buffer to write to fd.
    \param nbytes number of bytes to write from \e buffer
    \param nbytes_written the number of bytes written
    \return On success, it returns TTY_OK, otherwise, a TTY_ERROR code.
*/
int tty_write(int fd, const char * buffer, int nbytes, int *nbytes_written);

/** \brief Writes a null terminated string to fd.
    \param fd file descriptor
    \param buffer the buffer to write to fd.
    \param nbytes_written the number of bytes written
    \return On success, it returns TTY_OK, otherwise, a TTY_ERROR code.
*/
int tty_write_string(int fd, const char * buffer, int *nbytes_written);


/** \brief Establishes a tty connection to a terminal device.
    \param device the device node. e.g. /dev/ttyS0
    \param bit_rate bit rate
    \param word_size number of data bits, 7 or 8, USE 8 DATA BITS with modbus
    \param parity 0=no parity, 1=parity EVEN, 2=parity ODD
    \param stop_bits number of stop bits : 1 or 2
    \param fd \e fd is set to the file descriptor value on success.
    \return On success, it returns TTY_OK, otherwise, a TTY_ERROR code.
    \author Wildi Markus
*/

int tty_connect(const char *device, int bit_rate, int word_size, int parity, int stop_bits, int *fd);

/** \brief Closes a tty connection and flushes the bus.
    \param fd the file descriptor to close.
    \return On success, it returns TTY_OK, otherwise, a TTY_ERROR code.
*/
int tty_disconnect(int fd);

/** \brief Retrieve the tty error message
    \param err_code the error code return by any TTY function.
    \param err_msg an initialized buffer to hold the error message.
    \param err_msg_len length in bytes of \e err_msg
*/
void tty_error_msg(int err_code, char *err_msg, int err_msg_len);

int tty_timeout(int fd, int timeout);
/*@}*/

/**
 * \defgroup convertFunctions Formatting Functions: Functions to perform handy formatting and conversion routines.
 */
/*@{*/

/** \brief Converts a sexagesimal number to a string.

   sprint the variable a in sexagesimal format into out[].

  \param out a pointer to store the sexagesimal number.
  \param a the sexagesimal number to convert.
  \param w the number of spaces in the whole part.
  \param fracbase is the number of pieces a whole is to broken into; valid options:\n
          \li 360000:	\<w\>:mm:ss.ss
	  \li 36000:	\<w\>:mm:ss.s
 	  \li 3600:	\<w\>:mm:ss
 	  \li 600:	\<w\>:mm.m
 	  \li 60:	\<w\>:mm

  \return number of characters written to out, not counting final null terminator.
 */
int fs_sexa (char *out, double a, int w, int fracbase);

/** \brief convert sexagesimal string str AxBxC to double.

    x can be anything non-numeric. Any missing A, B or C will be assumed 0. Optional - and + can be anywhere.

    \param str0 string containing sexagesimal number.
    \param dp pointer to a double to store the sexagesimal number.
    \return return 0 if ok, -1 if can't find a thing.
 */
int f_scansexa (const char *str0, double *dp);

/** \brief Extract ISO 8601 time and store it in a tm struct.
    \param timestr a string containing date and time in ISO 8601 format.
    \param iso_date a pointer to a \e ln_date structure to store the extracted time and date (libnova).
    \return 0 on success, -1 on failure.
*/
int extractISOTime(char *timestr, struct ln_date *iso_date);

void getSexComponents(double value, int *d, int *m, int *s);

/** \brief Fill buffer with properly formatted INumber string.
    \param buf to store the formatted string.
    \param format format in sprintf style.
    \param value the number to format.
    \return length of string.
*/
int numberFormat (char *buf, const char *format, double value);

/** \brief Create an ISO 8601 formatted time stamp. The format is YYYY-MM-DDTHH:MM:SS
    \return The formatted time stamp.
*/
const char *timestamp (void);

/*@}*/

#ifdef __cplusplus
}
#endif


#endif

// ----------------------------------------------------------------------------
// EOF indicom.h - Released 2017-08-01T14:26:58Z
