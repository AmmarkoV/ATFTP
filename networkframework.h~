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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _NETWORKFRAMEWORK_H_
#define _NETWORKFRAMEWORK_H_


struct TFTP_PACKET // <- XWRAEI OPOIODIPOTE ALLO PAKETO AN DN KSEROUME TI EINAI
{
    unsigned char Op1,Op2;
    unsigned char data[514];
};

struct DATA_TFTP_PACKET
{
    unsigned char Op1,Op2;
    unsigned short Block;
    unsigned char data[512];
};

struct ACK_TFTP_PACKET
{
    unsigned char Op1,Op2;
    unsigned short Block; 
    unsigned char data[512];
};



unsigned int MINDATAPORT;
unsigned int MAXDATAPORT;

int  TFTPServer(unsigned int port);
int  TFTPClient(char * server,unsigned int port,char * filename,int operation);



#endif
