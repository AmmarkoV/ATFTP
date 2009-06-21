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
#include "networkframework.h"  
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include	<stdlib.h>
#include	<strings.h>
#include	<errno.h>
#include <unistd.h>
#include <sys/uio.h> 

unsigned int MINDATAPORT=30000;
unsigned int MAXDATAPORT=34000;

//     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//                      GENERIC FUNCTIONS  PART 
//
//     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


void error(char *msg)
{
    perror(msg);
    exit(0);
}

int OpCodeValidTFTP(unsigned char op1,unsigned char op2)
{
   if (op1!=0) { printf("Wrong TFTP magic number! %u \n",op1);
                 fflush(stdout);
                 return 1;
               } else
               {
                  switch (op2)
                  {
                    case 1: printf("RRQ - Read Request\n"); break;
                    case 2: printf("WRQ - Write Request\n"); break;
                    case 3: printf("Data Message - Should not be accepted here\n"); break;
                    case 4: printf("ACK - Data Acknowledge , Should not be accepted here\n"); break;
                    case 5: printf("ERROR - Error Request\n"); break;
                    default: return 1; break;
                   };
               }
   return 0;
}

int TransmitTFTPFile(char * filename,int server_sock,struct sockaddr_in  client_sock,int client_length)
{
   printf("TransmitTFTPFile ( Opening local file for read ) called\n");
   printf("TransmitTFTPFile from port %u \n",ntohs(client_sock.sin_port));
   FILE *filetotransmit;
   filetotransmit=fopen(filename,"rb");
   if (filetotransmit!=NULL)
    {
       //FILE CAN BE OPENED , CHECK FILE SIZE
       unsigned int filesize=0,filepos=0,dataread=0,datarecv=0,datatrans=0;
       fseek(filetotransmit,0,SEEK_END);
       filesize=ftell(filetotransmit);
       rewind(filetotransmit);

       printf("Requested file ( %s ) for transmission has %u bytes size!\n",filename,filesize); 
       //FILE SIZE OK
        // MAKE ACK TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        struct ACK_TFTP_PACKET ackpacket;
        //          2 bytes   2 bytes
        // ACK    | opcode | block # 
        //            A         B
        /* A part */ ackpacket.Op1=0; ackpacket.Op2=3;
        /* B part */ ackpacket.Block=0; 
        // MAKE ACK TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        // MAKE DATA TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        struct DATA_TFTP_PACKET request;
        //          2 bytes   2 bytes    n bytes
        // DATA   | opcode | block # |  Data
        //            A         B         C
        /* A part */ request.Op1=0; request.Op2=3;
        /* B part */ request.Block=0;
        /* C part */ request.data[0]=0;
        // MAKE DATA TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        filepos=0; datatrans=0;
        while (filepos<filesize)
           {
           if (filepos>0)
            { // OTAN EIMASTE STO FILEPOSITION 0 DEN PERIMENOUME AKOMA ACKNOWLEDGMENT  
             printf("Waiting to receive acknowledgement\n"); fflush(stdout);
             //RECEIVE ACKNOWLEDGMENT!
             datarecv=recvfrom(server_sock,(const char*) & ackpacket,4,0,&client_sock,client_length);
             if (datarecv < 0) {
                                 printf("Error while receiving acknowledgement for file %s ",filename);
                                 fclose(filetotransmit);
                                 return 1;
                                }  else
                                {
                                 printf("Received acknowledgement for block %u ",ackpacket.Block); 
                                }
            }

             //READ DATA
             dataread=fread(request.data,1,512,filetotransmit); 

             if (dataread!=512) { if ( ferror(filetotransmit) )
                                       {  printf("Error while reading file %s ",filename);
                                          fclose(filetotransmit);
                                          return 1; }
                                }

             //SEND DATA
             ++request.Block;
             filepos+=dataread;

 
             printf("Sending data\n"); fflush(stdout);
             datatrans=sendto(server_sock,(const char*) & request,dataread+4,0,&client_sock,client_length);
             if (datatrans < 0) { printf("Error while sending file %s ",filename);
                                  fclose(filetotransmit);
                                  return 1; }

           }

       fclose(filetotransmit);
    } else
    {
      printf("Could not open file %s \n",filename);
      return 1;
    }

   printf("TransmitTFTPFile has finished\n");
  return 0;
}







int ReceiveTFTPFile(char * filename,int server_sock,struct sockaddr_in  client_sock,int client_length)
{
   printf("ReceiveTFTPFile  ( Opening local file for write )  called\n");
   printf("ReceiveTFTPFile from port %u \n",ntohs(client_sock.sin_port));
   FILE *filetotransmit;
   filetotransmit=fopen(filename,"wb");
   if (filetotransmit!=NULL)
    { 
       //FILE CAN BE OPENED , CHECK FILE SIZE
       unsigned int filesize=0,filepos=0,datawrite=0,datatrans=0,reachedend=0;
       int datarecv=0;
        // MAKE ACK TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        struct ACK_TFTP_PACKET ackpacket;
        //          2 bytes   2 bytes
        // ACK    | opcode | block # 
        //            A         B
        /* A part */ ackpacket.Op1=0; ackpacket.Op2=3;
        /* B part */ ackpacket.Block=0; 
        // MAKE ACK TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


        // MAKE DATA TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        struct DATA_TFTP_PACKET request;
        //          2 bytes   2 bytes    n bytes
        // DATA   | opcode | block # |  Data
        //            A         B         C
        /* A part */ request.Op1=0; request.Op2=3;
        /* B part */ request.Block=0;
        /* C part */ request.data[0]=0;
        // MAKE DATA TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        filepos=0; datatrans=0; reachedend=0;
        while (reachedend==0)
           {

            //RECEIVE DATA
             printf("Waiting to receive data\n"); fflush(stdout);
             datarecv=recvfrom(server_sock,(const char*) & request,514,0,&client_sock,client_length);
             if (datarecv < 0) {
                                 printf("Error while receiving file %s ",filename);
                                 fclose(filetotransmit);
                                 return 1;
                                } else
                                 // 516 giati exoume 512byte data , 2 byte Op kai 2 byte block#
             printf("Received %u bytes from socket ",datarecv);
             if ( datarecv<516 ) { reachedend=1; } 

             //READ DATA
             datawrite=fwrite(request.data,1,datarecv-4,filetotransmit); 
             printf("Data2Write(%s)\n",request.data); fflush(stdout);

             if (datawrite!=512) { if ( ferror(filetotransmit) )
                                       {  printf("Error while writing file %s ",filename);
                                          fclose(filetotransmit);
                                          return 1;
                                       }
                                 }

             //SEND ACKNOWLEDGMENT
             ++ackpacket.Block;  

             printf("Sending acknowledgement\n"); fflush(stdout);
             datatrans=sendto(server_sock,(const char*) & ackpacket,4,0,&client_sock,client_length);
             if (datatrans < 0) { printf("Error while sending acknowledgment %s ",filename);
                                  fclose(filetotransmit);
                                  return 1; }
           }
      fclose(filetotransmit);
    } else
    {
      printf("Could not open file %s \n",filename);
      return 1;
    }
  printf("ReceiveTFTPFile has finished\n");
  return 0;
}



//     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//                      SERVER  PART 
//
//     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

    
int HandleClient(unsigned char * filename,int froml,struct sockaddr_in fromsock,int operation)
{
   printf("HandleClient\n");
   int clsock,bindres,length,n;
   struct sockaddr_in server;


   clsock=socket(AF_INET, SOCK_DGRAM, 0);
   if (clsock < 0) { printf("Unable to handle new client ( could not create socket ).. :( \n");
                     fflush(stdout);
                     exit(0);
                   }


   length = sizeof(server);
   bzero(&server,length);
   server.sin_family=AF_INET;
   server.sin_addr.s_addr=INADDR_ANY;



   unsigned int cl_port=MINDATAPORT;
   while  ( cl_port < MAXDATAPORT )
        { server.sin_port=htons(cl_port);
          bindres=bind(clsock,(struct sockaddr *)&server,length); 
          if (bindres < 0) { printf("Binding port number %u is not availiable \n",cl_port); 
                             ++cl_port;
                           } else
                           { printf("Binding to port number %u is OK \n",cl_port);
                             break;
                           }
        } 

   if (cl_port>=MAXDATAPORT) { printf("Unable to handle client  ( could not find free port ).. :( \n");
                               fflush(stdout);
                               exit(0); }
 


  if (operation==2) 
  {
   // IF client sends WRQ we reply with a ACK 0 message!
   struct ACK_TFTP_PACKET ackpacket;
   // MAKE TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   //        2 bytes   2 bytes
   // ACK  | opcode |  block # 
   //          A         B
   // A part 
   ackpacket.Op1=0;
   ackpacket.Op2=4;
   // B part 
   ackpacket.Block=0;
   // MAKE TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   n==sendto(clsock,(const char*) & ackpacket,4,0,(struct sockaddr *)&fromsock,froml);
  }



   if (operation==2) // WRQ
   {
     ReceiveTFTPFile(filename,clsock,fromsock,froml); 
   } else
   if (operation==1) // RRQ
   {
     TransmitTFTPFile(filename,clsock,fromsock,froml);
   }



   fflush(stdout);
   write(1,"Exiting thread..!\n",18);
   exit(0);
}




int TFTPServer(unsigned int port)
{
   printf("TFTPServer\n");
   int sock, length, fromlen, n;
   struct sockaddr_in server;
   struct sockaddr_in from;

   sock=socket(AF_INET, SOCK_DGRAM, 0);
   if (sock < 0) error("Opening socket");

   length = sizeof(server);
   bzero(&server,length);
   server.sin_family=AF_INET;
   server.sin_addr.s_addr=INADDR_ANY;
   server.sin_port=htons(port);

   if (bind(sock,(struct sockaddr *)&server,length)<0)  error("binding master port for atftp!");
   fromlen = sizeof(struct sockaddr_in);
   unsigned char filename[512];

   unsigned int newport,fork_res,packeterror=0;
   while (1) {

               struct TFTP_PACKET request={0};
 
               printf("\nWaiting for client\n");
               n=recvfrom(sock,(const char*) & request,514,0,(struct sockaddr *)&from,&fromlen);
               if (n < 0) error("recvfrom");

               packeterror=0;
               // DISASSEMBLE TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
               //             2 bytes              1 byte         1byte
               // RRQ/WRQ   | opcode |  filename  |  0  |  Mode  |  0 
               //               A          B         C       D      E
               // A part 
                   if (request.Op1!=0) { packeterror=1; }
                   if ((request.Op2!=2)&&(request.Op2!=1)) { packeterror=1; } 
               // B part 
                   //write(1,request.data,n-2);
                   strcpy(filename,request.data);
                   unsigned int fnm_end=strlen(filename);
                   if (fnm_end==0) { packeterror=1; }
               // DISASSEMBLE TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 

               if (packeterror==0)
               {
                 fork_res=fork();
                 if (fork_res<0) { write(1,"Fork failed!\n",13); } else
                 if (fork_res==0)
                  {
                    int f_fromlen=fromlen;
                    struct sockaddr_in f_from=from;

                    printf("New UDP server fork to serve file %s , operation type %u \n",filename,request.Op2); fflush(stdout);
                    HandleClient(filename,f_fromlen,f_from,request.Op2);
                  } else
                  { /* Server loop */ }
                } else
                { printf("TFTP Server master thread - Incoming Request Denied..\n");
                  fflush(stdout);
                  write(1,request.data,n-2);
                }
              }
 return;
}


//     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//                      CLIENT  PART 
//
//     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                                                          // AN OPERATION = 1 read , An OPERATION = 2 write
int TFTPClient(char * server_ip,unsigned int port,char * filename,int operation)
{
   if ( (operation!=2) && (operation!=1) ) { printf("Wrong Client Operation \n"); return 1; }
   if ( strlen(filename)<=0 ) { printf("Wrong filename \n"); return 1; }
   int sock, length, n;
   struct sockaddr_in server, from;
   struct hostent *hp;



   sock= socket(AF_INET, SOCK_DGRAM, 0);
   if (sock < 0) error("Could not open socket for TFTP connection");

   server.sin_family = AF_INET;
   hp = gethostbyname(server_ip);
   if (hp==0) error("Unknown host for TFTP connection ");

   bcopy((char *)hp->h_addr,(char *)&server.sin_addr,hp->h_length);
   server.sin_port = htons(port);
   length=sizeof(struct sockaddr_in);





   // MAKE TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   struct TFTP_PACKET request;
   //             2 bytes              1 byte         1byte
   // RRQ/WRQ   | opcode |  filename  |  0  |  Mode  |  0 
   //               A          B         C       D      E
   // A part 
   request.Op1=0;
   request.Op2=operation;
   // B part 
   strcpy(request.data,filename);
   unsigned int fnm_end=strlen(filename);
   // C part 
   request.data[fnm_end++]=0;
   // D part 
   // Mode forced octet
   request.data[fnm_end++]='o'; request.data[fnm_end++]='c'; request.data[fnm_end++]='t'; 
   request.data[fnm_end++]='e'; request.data[fnm_end++]='t';
   // E part 
   request.data[fnm_end++]=0;
   // MAKE TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

   n=sendto(sock,(const char*) & request,fnm_end+2,0,&server,length);
   if (n < 0) error("Sending initial TFTP packet");
   //Mexri edw exoume steilei i RRQ , i WRQ to opoio einai sigouro..


   if (operation==1) // READ OPERATION
     {
       ReceiveTFTPFile(filename,sock,server,length); 
     } else
   if (operation==2) // WRITE OPERATION
     {
       TransmitTFTPFile(filename,sock,server,length);
     }

   printf("Stopping TFTP client..\n");
   return(0);
}