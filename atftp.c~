/***************************************************************************
 *   Copyright (C) 2009 by Ammar Qammaz , Ivan kanakarakis  *
 *   ammarkov@gmail.com   *
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
 *   along with this program; if not, write to the    out                     *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "networkframework.h"

void
usage()
{
<<<<<<< HEAD:atftp.c
  printf ("-----------------------------------------------------\n");
  printf ("A-TFTP 0.10 - for info see :\t http://62.103.22.50 \n");
  printf ("-----------------------------------------------------\n");
  printf ("Usage for TFTP client : \n");
  printf ("atftp -r filename address port \t- read filename from address @ port \n");
  printf ("atftp -w filename address port \t- write filename to address @ port \n");
  printf ("\nUsage for TFTP server : \n");
  printf ("atftp -s port \t- begin tftp server binded @ address|port\n");
=======
  printf("-----------------------------------------------------\n");
  printf("A-TFTP 0.09 - for info see :\t http://62.103.22.50 \n");
  printf("-----------------------------------------------------\n");
  printf("Usage for TFTP client : \n");
  printf("atftp -r filename address port \t- read filename from address @ port \n");
  printf("atftp -w filename address port \t- write filename to address @ port \n");
  printf("\nUsage for TFTP server : \n");
  printf("atftp -s port \t- begin tftp server binded @ port\n");
>>>>>>> origin/master:atftp.c
}

void
paramErr(unsigned npar)
{
  printf("Parameter error, too little parameters (%u)\n\n", npar);
  usage();
}

void
clientMode(int mode, char* arg2, char* arg3, char* arg4)
{
  printf("Starting TFTP Client.. \n\n");
  TFTPClient(arg3, atoi(arg4), arg2, mode);
}

void
serverMode(unsigned servr_port)
{
  if (!servr_port)
  {
      printf("No port specified, default to port %u\n", DEF_SERV_PORT);
      servr_port = DEF_SERV_PORT;
  }
  printf("Starting TFTP Server at port %u .. \n\n", servr_port);
  TFTPServer(servr_port);
}

int
main(int argc, char *argv[])
{
  if (argc < 2)
  {
      usage();
  }
  else if (strcmp(ARG_READ, argv[1]) == 0)
  {
      if (argc <= 4)
      {
          paramErr(argc - 1);
          return EXIT_FAILURE;
      }
      clientMode(READ, argv[2], argv[3], argv[4]);
  }
  else if (strcmp(ARG_WRITE, argv[1]) == 0)
  {
      if (argc <= 4)
      {
          paramErr(argc - 1);
          return EXIT_FAILURE;
      }
      clientMode(WRITE, argv[2], argv[3], argv[4]);
  }
  else if (strcmp(ARG_START_SERVR, argv[1]) == 0)
  {
      serverMode(argv[2] == NULL ? 0 : atoi(argv[2]));
  }
  else
  {
      usage();
  }
  return EXIT_SUCCESS;
}
