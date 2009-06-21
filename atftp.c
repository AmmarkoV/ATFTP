/***************************************************************************
 *   Copyright (C) 2009 by Ammar Qammaz   *
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
#include <string.h>


int main(int argc, char *argv[])
{
  printf("A-TFTP 0.07 - for info  http://62.103.22.50 \n");
  printf("-----------------------------------------------------\n");
  if (argc <= 2) { printf("Parameter error , you provided %u/3 parameters \nFor TFTP client , usage can be : \n",argc-1);
                  printf("atftp -r filename address port ( read filename from address @ port )\n");
                  printf("atftp -w filename address port ( write filename to address @ port )\n");
                  printf("\nFor TFTP server , usage can be : \n");
                  printf("atftp -s address port ( begin tftp server binded @ address/port )\n");
                  return 1;
                }
 

  if ((strcmp("-r",argv[1])==0)||(strcmp("-w",argv[1])==0)) 
                    {
                      if (argc <= 4) { 
                                      printf("Parameter error, you provided %u/4 parameters for client mode \n",argc-1);
                                      printf("For TFTP client usage can be : \n");
                                      printf("atftp -r filename address port ( read filename from address @ port )\n");
                                      printf("atftp -w filename address port ( write filename to address @ port )\n");
                                      return 1;
                                    }
                      printf("Starting TFTP Client.. \n");
                      if (strcmp("-r",argv[1])==0) { TFTPClient(argv[3],atoi(argv[4]),argv[2],1); } else
                      if (strcmp("-w",argv[1])==0) { TFTPClient(argv[3],atoi(argv[4]),argv[2],2); }
                    } else
  if (strcmp("-s",argv[1])==0) 
                    {
                      printf("Starting TFTP Server at port %u .. \n",atoi(argv[3]));
                      TFTPServer(atoi(argv[3]));
                     }

  printf("\n");
  
  return 0;
}

