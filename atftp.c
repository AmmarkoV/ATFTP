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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "networkframework.h"

void
usage(const char* program)
{
  if ( program == NULL )
  {
      program = "yatftp";
  }
  printf("\n----------------------------------------------------------------------\n");
  printf("YATFTP 0.20 - for code see link @ git://github.com/AmmarkoV/ATFTP.git\n");
  printf("----------------------------------------------------------------------\n");
  printf("\nUsage for TFTP client : \n");
  printf("%s -%c -%c filename -%c address -%c port :\t "
         "read filename from address @ port \n",
         program, READ_OPT, FILE_OPT, ADDRESS_OPT, PORT_OPT);
  printf("%s -%c -%c filename -%c address -%c port :\t "
         "write filename to address @ port \n",
         program, WRITE_OPT, FILE_OPT, ADDRESS_OPT, PORT_OPT);
  printf("\nUsage for TFTP server : \n");
  printf("%s -%c [-%c port] :\t "
         "begin tftp server binded @ port (default #port %d)\n",
         program, SERVR_OPT, PORT_OPT, DEF_SERV_PORT);
  printf("\nCommon Options : \n");
  printf("-%c :\t log output to file %s\n", LOG_OPT, DEF_LOG_FILE);
  printf("-%c :\t use verbose output\n", VERBOSE_OPT);
  printf("-%c :\t use debug output\n", DEBUG_OPT);
}

void
rootwarn()
{
  printf("\n\n\n\n-----------------------------------------------------\n");
  printf("------------------!!!! WARNING !!!!------------------\n");
  printf("-----------------------------------------------------\n\n");
  printf("Due to the unsecure nature of the TFTP protocol it is a VERY "
         "bad idea to run the YATFTP server from a root account , ");
  printf("the safest way to run yatftp server is to create a new user "
         "named tftp_service ( or something like that ) , and thus ");
  printf("isolating the program from all other files to prevent damage "
         "from malicious clients to your system ..!");
  printf("\n-----------------------------------------------------\n");
  printf("------------------!!!! WARNING !!!!------------------\n");
  printf("-----------------------------------------------------\n\n\n\n\n");
  printf("ATFTP Server will now quit\n");
  fflush(stdout);
}

int
matchexpr(const char* expression, const char* pattern)
{
  regex_t regex;
  unsigned status;
  regcomp(&regex, pattern, 0);
  status = regexec(&regex, expression, (size_t) 0, NULL, 0);
  regfree(&regex);
  return status;
}

int
mixerr(const char* program)
{
  fprintf(stderr, "%s: Wrong arguments. Do not mix server/client options\n", program);
  usage(program);
  return EXIT_FAILURE;
}

void
argerr(char* program, char option, char* expr)
{
  fprintf(stderr, "%s: Wrong argument for -%c option: %s\n", program, option, expr);
}

void
client(const int operation, const char* filename, char* address, int port)
{
  printf("Starting TFTP Client.. \n\n");
  TFTPClient(address, port, filename, operation);
}

void
server(unsigned servr_port)
{
  printf("Starting TFTP Server at port %u .. \n\n", servr_port);
  TFTPServer(servr_port);
}

int
main(int argc, char *argv[])
{
  int opt, mode, operation, port = DEF_SERV_PORT, verbose = 0, log = 0, debug = 0;
  int rdflg = 0, wrflg = 0, errflg = 0, addrflg = 1, flflg = 1; /* flags */
  char *filename, *logfile, *address;
  char options[] = { SERVR_OPT, READ_OPT, WRITE_OPT, PORT_OPT, NEED_ARG, LOG_OPT,
      ADDRESS_OPT, NEED_ARG, FILE_OPT, NEED_ARG, VERBOSE_OPT, DEBUG_OPT, '\0' };
  extern char *optarg;
  /* check if root */
  if ( !getuid() )
  {
      rootwarn();
      return EXIT_FAILURE;
  }
  /* check no arguments */
  if ( argc < 2 )
  {
      usage(argv[0]);
      return EXIT_FAILURE;
  }
  /* read arguments and set initial values */
  /* opterr = 0; /* do not print getargs() error messages */
  while ((opt = getopt(argc, argv, options)) != -1)
  {
      switch (opt)
      {
          case SERVR_OPT:
              if ( (wrflg | rdflg) || mode == CLIENT_MODE )
              {
                  return mixerr(argv[0]);
              }
              mode = SERVER_MODE;
              flflg = addrflg = 0; /* we don't a filename, nor an address */
              break;
          case READ_OPT:
              if ( mode == SERVER_MODE )
              {
                  return mixerr(argv[0]);
              }
              if ( wrflg )
              {
                  usage(argv[0]);
                  return EXIT_FAILURE;
              }
              mode = CLIENT_MODE;
              operation = READ;
              rdflg = 1;
              break;
          case WRITE_OPT:
              if ( mode == SERVER_MODE )
              {
                  return mixerr(argv[0]);
              }
              if ( rdflg )
              {
                  usage(argv[0]);
                  return EXIT_FAILURE;
              }
              mode = CLIENT_MODE;
              operation = WRITE;
              wrflg = 1;
              break;
          case FILE_OPT:
              if ( mode == SERVER_MODE )
              {
                  return mixerr(argv[0]);
              }
              mode = CLIENT_MODE;
              filename = optarg;
              flflg = 0;
              break;
          case ADDRESS_OPT:
              if ( mode == SERVER_MODE )
              {
                  return mixerr(argv[0]);
              }
              if ( matchexpr(optarg, ADDRESS_PATTERN) )
              {
                  argerr(argv[0], opt, optarg);
                  usage(argv[0]);
                  return EXIT_FAILURE;
              }
              else
              {
                  address = optarg;
              }
              mode = CLIENT_MODE;
              addrflg = 0;
              break;
          case PORT_OPT:
              if ( matchexpr(optarg, PORT_PATTERN) )
              {
                  argerr(argv[0], opt, optarg);
                  usage(argv[0]);
                  return EXIT_FAILURE;
              }
              else
              {
                  port = atoi(optarg);
              }
              break;
          case LOG_OPT:
              log = 1;
              logfile = DEF_LOG_FILE;
              break;
          case DEBUG_OPT:
              debug = 1;
          case VERBOSE_OPT:
              verbose = 1;
              break;
          case '?':
          default:
              usage(argv[0]);
              return EXIT_FAILURE;
      }
  }
  if ( flflg )
  {
      fprintf(stderr, "CLIENT: No file set. "
              "Use '-%c filename' option.\n", FILE_OPT);
      usage(argv[0]);
      return EXIT_FAILURE;
  }
  if ( addrflg )
  {
      fprintf(stderr, "CLIENT: No address set. "
              "Use '-%c address' option.\n", ADDRESS_OPT);
      usage(argv[0]);
      return EXIT_FAILURE;
  }
  if ( mode == CLIENT_MODE && !rdflg && !wrflg )
  {
      fprintf(stderr, "CLIENT: No operation set. "
              "Use -%c or -%c option\n", READ_OPT, WRITE_OPT);
      usage(argv[0]);
      return EXIT_FAILURE;
  }
  if ( mode == SERVER_MODE )
  {
      server(port);
  }
  else // if (mode == CLIENT_MODE)
  {
      client(operation, filename, address, port);
  }
  return EXIT_SUCCESS;
}
