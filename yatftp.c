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
usage()
{
  fprintf(stderr,
          "\n----------------------------------------------------------------------\n"
          "YATFTP 0.20 - for code see link @ git://github.com/AmmarkoV/ATFTP.git\n"
          "----------------------------------------------------------------------\n"
          "\nUsage for TFTP client : \n"
          "%s -%c -%c filename -%c address -%c port :\t "
          "read filename from address @ port \n",
          _PNAME, READ_OPT, FILE_OPT, ADDRESS_OPT, PORT_OPT);
  fprintf(stderr, "%s -%c -%c filename -%c address -%c port :\t "
          "write filename to address @ port \n",
          _PNAME, WRITE_OPT, FILE_OPT, ADDRESS_OPT, PORT_OPT);
  fprintf(stderr, "\nUsage for TFTP server : \n"
          "%s -%c [-%c port] :\t "
          "begin tftp server binded @ port (default #port %d)\n",
          _PNAME, SERVR_OPT, PORT_OPT, DEF_SERV_PORT);
  fprintf(stderr, "\nCommon Options : \n"
          "-%c :\t log output to file %s\n", LOG_OPT, DEF_LOG_FILE);
  fprintf(stderr, "-%c :\t silent mode, don't print any output\n"
          "-%c :\t use verbose output\n-%c :\t use debug output\n",
          SILENT_OPT, VERBOSE_OPT, DEBUG_OPT);
}

/* match the given expression int the defined pattern */
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

/* print error when user has mixed server and client options */
int
mixerr()
{
  fprintf(stderr, "%s: Wrong arguments. Do not mix server/client options\n", _PNAME);
  usage();
  return EXIT_FAILURE;
}

/* print error when the argument is wrong for the given option */
void
argerr(char option, char* expr)
{
  fprintf(stderr, "%s: Wrong argument for -%c option: %s\n", _PNAME, option, expr);
}

/* start client operation */
void
client(const int operation, const char* filename, char* address, int port)
{
  fprintf(outstrm, "Starting TFTP Client.. \n\n");
  TFTPClient(address, port, filename, operation);
}

/* start server operation */
void
server(unsigned servr_port)
{
  fprintf(outstrm, "Starting TFTP Server at port %u .. \n\n", servr_port);
  TFTPServer(servr_port);
}

/* this is when the program starts, lol wtf !? :P */
int
main(int argc, char *argv[])
{
  extern char *optarg;
  int opt, mode, operation, port = DEF_SERV_PORT,
          rdflg = 0, wrflg = 0, addrflg = 1, flflg = 1;
  char *filename, *address, options[] = { SERVR_OPT, READ_OPT, WRITE_OPT,
                                         PORT_OPT, NEED_ARG, LOG_OPT,
                                         ADDRESS_OPT, NEED_ARG, VERBOSE_OPT,
                                         FILE_OPT, NEED_ARG, DEBUG_OPT,
                                         SILENT_OPT, '\0' };
  /* default output stream is stdout,
   * unless -l option is selected,
   * then we switch to a logfile */
  outstrm = stdout;
  /* check if no arguments present */
  if ( argc < 2 )
  {
      usage();
      return EXIT_FAILURE;
  }
  /* do not print getargs() error messages */
  /* opterr = 0; */
  /* read arguments and set initial values */
  while ((opt = getopt(argc, argv, options)) != -1)
  {
      switch (opt)
      {
              /* read the option
               * check the flags
               * if invalid print error and exit
               * else set the flags properly */
          case SERVR_OPT:
              if ( (wrflg | rdflg) || mode == CLIENT_MODE )
                  return mixerr();
              mode = SERVER_MODE;
              /* we don't a filename, nor an address */
              flflg = addrflg = 0;
              break;
          case READ_OPT:
              if ( mode == SERVER_MODE )
                  return mixerr();
              if ( wrflg )
              {
                  usage();
                  return EXIT_FAILURE;
              }
              mode = CLIENT_MODE;
              operation = READ;
              rdflg = 1;
              break;
          case WRITE_OPT:
              if ( mode == SERVER_MODE )
                  return mixerr();
              if ( rdflg )
              {
                  usage();
                  return EXIT_FAILURE;
              }
              mode = CLIENT_MODE;
              operation = WRITE;
              wrflg = 1;
              break;
          case FILE_OPT:
              if ( mode == SERVER_MODE )
                  return mixerr();
              mode = CLIENT_MODE;
              filename = optarg;
              flflg = 0;
              break;
          case ADDRESS_OPT:
              if ( mode == SERVER_MODE )
                  return mixerr();
              if ( matchexpr(optarg, ADDRESS_PATTERN) )
              {
                  argerr(opt, optarg);
                  usage();
                  return EXIT_FAILURE;
              }
              else address = optarg;
              mode = CLIENT_MODE;
              addrflg = 0;
              break;
          case PORT_OPT:
              if ( matchexpr(optarg, PORT_PATTERN) )
              {
                  argerr(opt, optarg);
                  usage();
                  return EXIT_FAILURE;
              }
              else port = atoi(optarg);
              break;
          case LOG_OPT:
              outstrm = fopen(DEF_LOG_FILE, "w");
              break;
          case DEBUG_OPT:
              verbosity = 3;
          case VERBOSE_OPT:
              verbosity = 2;
              break;
          case SILENT_OPT:
              verbosity = 0;
              break;
          case '?':
          default:
              usage();
              return EXIT_FAILURE;
      }
  }
  if ( flflg )
  {
      fprintf(stderr, "CLIENT: No file set. "
              "Use '-%c filename' option.\n", FILE_OPT);
      usage();
      return EXIT_FAILURE;
  }
  if ( addrflg )
  {
      fprintf(stderr, "CLIENT: No address set. "
              "Use '-%c address' option.\n", ADDRESS_OPT);
      usage();
      return EXIT_FAILURE;
  }
  if ( mode == CLIENT_MODE && !rdflg && !wrflg )
  {
      fprintf(stderr, "CLIENT: No operation set. "
              "Use -%c or -%c option\n", READ_OPT, WRITE_OPT);
      usage();
      return EXIT_FAILURE;
  }
  if ( mode == SERVER_MODE )
      server(port);
  else if ( mode == CLIENT_MODE )
      client(operation, filename, address, port);
  return EXIT_SUCCESS;
}
