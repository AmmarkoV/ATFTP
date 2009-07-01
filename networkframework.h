/***************************************************************************
 *   Copyright (C) 2009 by Ammar Qammaz, Ivan kanakarakis                  *
 *   ammarkov@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _NETWORKFRAMEWORK_H_
#define _NETWORKFRAMEWORK_H_

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define _PNAME "yatftp"
#define ROOT_ID 0
#define USER_ID 1000
#define MINDATAPORT 30000
#define MAXDATAPORT 337000
#define MAX_FAILED_RETRIES 10
#define MAX_WAIT 20
#define DEF_SERV_PORT 69
#define DEF_LOG_FILE "tftp.log"
#define ADDRESS_PATTERN "^\\([0-9]\\{1,3\\}\\.\\)\\{3\\}[0-9]\\{1,3\\}$"
#define PORT_PATTERN "^[0-9]\\{1,5\\}$"
#define SERVR_OPT 's'
#define READ_OPT 'r'
#define WRITE_OPT 'w'
#define PORT_OPT 'p'
#define ADDRESS_OPT 'a'
#define FILE_OPT 'f'
#define LOG_OPT 'l'
#define VERBOSE_OPT 'v'
#define SILENT_OPT 'q'
#define DEBUG_OPT 'd'
#define NEED_ARG ':'

enum values
{
  ZERO, READ, WRITE, SERVER_MODE, CLIENT_MODE,
};

/* XWRAEI OPOIODIPOTE ALLO PAKETO AN DN KSEROUME TI EINAI */
struct TFTP_PACKET
{
  unsigned char Op1, Op2;
  char data[514];
};

/* GIA DATA PAKETA */
struct DATA_TFTP_PACKET
{
  unsigned char Op1, Op2;
  unsigned short Block;
  char data[512];
};

/*  GIA ACKNOWLEDGMENT PAKETA */
struct ACK_TFTP_PACKET
{
  unsigned char Op1, Op2;
  unsigned short Block;
  char data[512];
};

/* XWRAEI OPOIODIPOTE ALLO PAKETO AN DN KSEROUME TI EINAI */
struct ERROR_TFTP_PACKET
{
  unsigned char Op1, Op2;
  unsigned short ErrorCode;
  char data[512];
};

/* verbosity levels:
 * 0 No messages at all,
 * 1 Error Messages,
 * 2 +Status Messages,
 * 3 +Trivial Debug messages */
unsigned short verbosity;
/* where to send output */
FILE *outstrm;

#endif
