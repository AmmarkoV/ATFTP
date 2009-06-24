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

#include "networkframework.h"

unsigned int MINDATAPORT = 30000;
unsigned int MAXDATAPORT = 35000;

//     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//                      GENERIC FUNCTIONS  PART 
//
//     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void
error(char *msg)
{
  perror(msg);
  exit(0);
}

void
printerror(int errnum)
{
  if ( errnum == EBADF )
  {
      printf("The argument is an invalid descriptor\n");
  }
  else if ( errnum == ECONNREFUSED )
  {
      printf("A remote host refused to allow the network connection\n");
  }
  else if ( errnum == ENOTCONN )
  {
      printf("The socket is associated with a connection oriented protocol and is not connected\n");
  }
  else if ( errnum == ENOTSOCK )
  {
      printf("The argument s does not refer to a socket\n");
  }
  else if ( errnum == EAGAIN )
  {
      printf("Timeout expired / The socket is marked non-blocking and the op would block \n");
  }
  else if ( errnum == EINTR )
  {
      printf("Receive interrupted by signal\n");
  }
  else if ( errnum == EFAULT )
  {
      printf("The receive buffer is outside the process address space\n");
  }
  else if ( errnum == EINVAL )
  {
      printf("Invalid argument passed\n");
  }
  else if ( errnum == ENOPROTOOPT )
  {
      printf("Option not supported by the protocol\n");
  }
  else if ( errnum == EDOM )
  {
      printf("Too big values to fit\n");
  }
  else
  {
      printf("Unknown error %i ", errnum);
  }
  fflush(stdout);
}

int
OpCodeValidTFTP(unsigned char op1, unsigned char op2)
{
  if ( op1 != 0 )
  {
      printf("Wrong TFTP magic number! %u \n", op1);
      fflush(stdout);
      return 1;
  }
  else
  {
      switch (op2)
      {
          case 1:
              printf("RRQ - Read Request\n");
              break;
          case 2:
              printf("WRQ - Write Request\n");
              break;
          case 3:
              printf("Data Message - Should not be accepted here\n");
              break;
          case 4:
              printf("ACK - Data Acknowledge , Should not be accepted here\n");
              break;
          case 5:
              printf("ERROR - Error Request\n");
              break;
          default:
              return 1;
              break;
      };
  }
  return 0;
}

int
ReceiveNullACK(int server_sock, struct sockaddr_in* client_sock, int client_length)
{
  // MAKE ACK TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  struct ACK_TFTP_PACKET ackpacket;
  //          2 bytes   2 bytes
  // ACK    | opcode | block #
  //            A         B
  /* A part */ ackpacket.Op1 = 0;
  ackpacket.Op2 = 4;
  /* B part */ ackpacket.Block = 0;
  // MAKE ACK TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  int datarecv, retransmit_attempts = 1;
  while ((retransmit_attempts != 0) && (retransmit_attempts < MAX_FAILED_RETRIES))
  {
      printf("Waiting to receive null acknowledgement\n");
      fflush(stdout);
      //RECEIVE ACKNOWLEDGMENT!
      datarecv = recvfrom(server_sock, (char*) & ackpacket, 4, 0,
                          (struct sockaddr *) & client_sock, &client_length);
      if ( datarecv < 0 )
      {
          printf("Error while receiving null acknowledgement \n");
          printerror(errno);
          ++retransmit_attempts;
          return -1;
      }
      else
      {
          printf("Received acknowledgement for block %u from port %u ( waiting for 0 ) \n",
                 ackpacket.Block, ntohs(client_sock->sin_port));
          if ( (ackpacket.Op1 != 0) || (ackpacket.Op2 != 4) )
          {
              printf("\nWrong Packet!\n");
              ++retransmit_attempts;
          }
          else if ( ackpacket.Block != 0 )
          {
              printf("\n\nOut of sync acknowledge.!\n\n");
              ++retransmit_attempts;
          }
          else
          {
              printf("\nNull Acknowledge received!!\n");
              retransmit_attempts = 0;
              break;
          }
      }
  }
  printf("Stopping null acknowledgement wait..\n");
  return 0;
}

int
FindFreePortInRange(int thesock, struct sockaddr_in* server)
{
  unsigned int cl_port = MINDATAPORT;
  int bindres = 0, length = sizeof (struct sockaddr_in);
  while (cl_port <= MAXDATAPORT)
  {
      server->sin_port = htons(cl_port);
      bindres = bind(thesock, (struct sockaddr *) server, length);
      if ( bindres < 0 )
      {
          //Den einai aparaitito na grafei to kathe bind step
          //printf("Binding port number %u is not availiable \n", cl_port);
          ++cl_port;
      }
      else
      {
          printf("Binding to port number %u is OK \n", cl_port);
          break;
      }
  }
  if ( cl_port > MAXDATAPORT )
  {
      printf("Unable to handle client  ( could not find free port ).. :( \n");
      fflush(stdout);
      cl_port = 0;
  }

  return cl_port;
}

int
TransmitTFTPFile(char * filename, int server_sock, struct sockaddr_in * client_out_sock, int client_length)
{
  printf("TransmitTFTPFile ( Opening local file for read ) called\n");
  printf("TransmitTFTPFile to port %u \n", ntohs(client_out_sock->sin_port));
  FILE *filetotransmit;
  filetotransmit = fopen(filename, "rb");
  unsigned int retransmit_attempts = 0;
  if ( filetotransmit != NULL )
  {
      //FILE CAN BE OPENED , CHECK FILE SIZE
      unsigned int filesize = 0, filepos = 0, dataread = 0, datatrans = 0;
      int datarecv = 0;
      fseek(filetotransmit, 0, SEEK_END);
      filesize = ftell(filetotransmit);
      rewind(filetotransmit);

      printf("Requested file ( %s ) for transmission has %u bytes size!\n", filename, filesize);
      if ( (filesize % 512 == 0) && (filesize != 0) )
      {
          printf("File size is a multiple of 512 should append a zero data package \n");
      }
      //FILE SIZE OK
      // MAKE ACK TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      struct ACK_TFTP_PACKET ackpacket;
      //          2 bytes   2 bytes
      // ACK    | opcode | block #
      //            A         B
      /* A part */ ackpacket.Op1 = 0;
      ackpacket.Op2 = 4;
      /* B part */ ackpacket.Block = 0;
      // MAKE ACK TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

      // MAKE DATA TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      struct DATA_TFTP_PACKET request;
      //          2 bytes   2 bytes    n bytes
      // DATA   | opcode | block # |  Data
      //            A         B         C
      /* A part */ request.Op1 = 0;
      request.Op2 = 3;
      /* B part */ request.Block = 0;
      /* C part */ request.data[0] = 0;
      // MAKE DATA TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      filepos = 0;
      datatrans = 0;

      struct sockaddr_in client_in_sock;

      while ((filepos < filesize) || ((retransmit_attempts != 0) && (retransmit_attempts < MAX_FAILED_RETRIES))
             || ((filepos >= filesize) && (dataread == 512)))
      {
          //ORIAKI PERIPTWSI POU TO TELEYTAIO MINIMA EINAI 512 Byte
          if ( (filepos >= filesize) && (dataread == 512) )
          {
              printf("Last message is 512 characters so Recepient won`t be able to stop \n");
              printf("Sending a zero data \n");
              dataread = 0;
              retransmit_attempts = 1;
              request.data[0] = 0;
              ++request.Block;
          }
          //ORIAKI PERIPTWSI POU TO TELEYTAIO MINIMA EINAI 512 Byte

          if ( retransmit_attempts == 0 )
          { //AN DEN KANOUME RENTRASMIT TOTE DIAVAZOUME KAINOURGIO BLOCK APO TO ARXEIO
              //READ DATA APO TO TOPIKO ARXEIO
              dataread = fread(request.data, 1, 512, filetotransmit);
              if ( dataread != 512 )
              {
                  if ( ferror(filetotransmit) )
                  {
                      printf("Error while reading file %s \n", filename);
                      fclose(filetotransmit);
                      return 1;
                  }
              }
              //SEND DATA STO ALLO MELLOS TOU SESSION
              ++request.Block;
              filepos += dataread;
          } // AN DEN KANOUME RENTRASMIT TOTE DIAVAZOUME KAINOURGIO BLOCK APO TO ARXEIO 


          printf("Sending data %u \n", dataread + 4);
          fflush(stdout);
          datatrans = sendto(server_sock, (const char*) & request, dataread + 4, 0, (struct sockaddr *) & client_out_sock, client_length);
          if ( datatrans < 0 )
          {
              printf("Error while sending file %s ", filename);
              printerror(errno);
              fclose(filetotransmit);
              return 1;
          }


          //RECEIVE ACKNOWLEDGMENT!
          printf("Waiting to receive acknowledgement\n");
          fflush(stdout);
          datarecv = recvfrom(server_sock, (char*) & ackpacket, 4, 0, (struct sockaddr *) & client_in_sock, &client_length);
          if ( datarecv < 0 )
          {
              printf("Error while receiving acknowledgement for file %s \n", filename);
              printerror(errno);
              ++retransmit_attempts;
              return 1;
          }
          else
          {
              printf("Received acknowledgement ( size %u ) for block %u , currently @ %u \n", datarecv, ackpacket.Block, request.Block);
              if ( (ackpacket.Op1 != 0) || (ackpacket.Op2 != 4) )
              {
                  printf("Incorrent acknowledgment magic numbers ( %u %u instead of 0 4 )  \n", ackpacket.Op1, ackpacket.Op2);
              }
              else if ( ackpacket.Block != request.Block )
              {
                  printf("\n\nOut of sync acknowledge Should send error ( todo ).!\n\n");
              }
              retransmit_attempts = 0;
          }
          //RECEIVE ACKNOWLEDGMENT!

          printf("Data read on loop is %u ", dataread);
      }
      fclose(filetotransmit);
  }
  else
  {
      printf("Could not open file %s \n", filename);
      return 1;
  }
  printf("TransmitTFTPFile has finished\n");
  return 0;
}

int
ReceiveTFTPFile(char * filename, int server_sock, struct sockaddr_in * client_out_sock, int client_length)
{
  printf("ReceiveTFTPFile  ( Opening local file for write )  called\n");
  printf("ReceiveTFTPFile from port %u \n", ntohs(client_out_sock->sin_port));
  FILE *filetotransmit;
  filetotransmit = fopen(filename, "wb");
  if ( filetotransmit != NULL )
  {
      //FILE CAN BE OPENED , CHECK FILE SIZE
      unsigned int filesize = 0, filepos = 0, datawrite = 0, datatrans = 0, reachedend = 0;
      int datarecv = 0;
      // MAKE ACK TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      struct ACK_TFTP_PACKET ackpacket;
      //          2 bytes   2 bytes
      // ACK    | opcode | block #
      //            A         B
      /* A part */ ackpacket.Op1 = 0;
      ackpacket.Op2 = 4;
      /* B part */ ackpacket.Block = 0;
      // MAKE ACK TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

      // MAKE DATA TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      struct DATA_TFTP_PACKET request;
      //          2 bytes   2 bytes    n bytes
      // DATA   | opcode | block # |  Data
      //            A         B         C
      /* A part */ request.Op1 = 0;
      request.Op2 = 3;
      /* B part */ request.Block = 0;
      /* C part */ request.data[0] = 0;
      // MAKE DATA TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

      filepos = 0;
      datatrans = 0;
      reachedend = 0;
      struct sockaddr_in client_in_sock;

      while (reachedend == 0)
      {
          //RECEIVE DATA
          printf("Waiting to receive data\n");
          fflush(stdout);
          datarecv = recvfrom(server_sock, (char*) & request, sizeof (request), 0, (struct sockaddr *) & client_in_sock, &client_length);

          if ( datarecv < 0 )
          {
              printf("Error while receiving file %s \n", filename);
              printerror(errno);
              fclose(filetotransmit);
              return 1;
          }
          else
          {
              printf("Received %u bytes from socket ", datarecv);
              request.data[datarecv - 4] = 0;
          }

          if ( datarecv<sizeof (request) )
          {
              reachedend = 1;
              printf("This should be the last packet \n");
          }

          //READ DATA
          if ( datarecv - 4 == 0 )
          {
              printf("Will not write to file zero .. \n");
          }
          else
          {
              datawrite = fwrite(request.data, 1, datarecv - 4, filetotransmit);
              /*printf("Data2Write(%s)\n",request.data); fflush(stdout);*/
          }

          if ( datawrite != 512 )
          {
              if ( ferror(filetotransmit) )
              {
                  printf("Error while writing file %s \n", filename);
                  fclose(filetotransmit);
                  return 1;
              }
          }

          //SEND ACKNOWLEDGMENT
          ++ackpacket.Block;

          printf("Sending acknowledgement %u \n", ackpacket.Block);
          fflush(stdout);
          datatrans = sendto(server_sock, (const char*) & ackpacket, 4, 0, (struct sockaddr *) & client_out_sock, client_length);
          if ( datatrans < 0 )
          {
              printf("Error while sending acknowledgment %s ", filename);
              fclose(filetotransmit);
              return 1;
          }
      }
      fclose(filetotransmit);
  }
  else
  {
      printf("Could not open file %s \n", filename);
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

int
HandleClient(unsigned char * filename, int froml, struct sockaddr_in fromsock, int operation)
{
  printf("HandleClient\n");
  int clsock, length, n;
  struct sockaddr_in server;

  clsock = socket(AF_INET, SOCK_DGRAM, 0);
  if ( clsock < 0 )
  {
      printf("Unable to handle new client ( could not create socket ).. :( \n");
      fflush(stdout);
      exit(0);
  }
  //SET TIMEOUT FOR OPERTATIONS
  struct timeval timeout_time = { 0 };
  timeout_time.tv_sec = 20;
  int i = setsockopt(clsock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *) & timeout_time, sizeof ( struct timeval));
  if ( i != 0 )
  {
      printf("Error setting receive Timeout for serving socket \n");
      printerror(errno);
  }
  i = setsockopt(clsock, SOL_SOCKET, SO_SNDTIMEO, (struct timeval *) & timeout_time, sizeof ( struct timeval));
  if ( i != 0 )
  {
      printf("Error setting send Timeout for serving socket \n");
      printerror(errno);
  }
  //SET TIMEOUT FOR OPERTATIONS
  length = sizeof (server);
  bzero(&server, length);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;

  unsigned int cl_port = MINDATAPORT;
  // BIND CODE GIA NA LAMVANOUME TA MINIMATA APO TON SERVER
  cl_port = FindFreePortInRange(clsock, &server);
  if ( (cl_port == 0) || (ntohs(server.sin_port) == 0) )
  {
      printf("Server  will be unable to receive messages , so it will now quit ( %u , %u ) \n", cl_port, ntohs(server.sin_port));
      exit(0);
  }
  // BIND CODE GIA NA LAMVANOUME TA MINIMATA APO TON SERVER
  if ( operation == 2 )
  {
      // IF client sends WRQ we reply with a ACK 0 message!
      struct ACK_TFTP_PACKET ackpacket;
      // MAKE TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      //        2 bytes   2 bytes
      // ACK  | opcode |  block #
      //          A         B
      // A part
      ackpacket.Op1 = 0;
      ackpacket.Op2 = 4;
      // B part
      ackpacket.Block = 0;
      // MAKE TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      n == sendto(clsock, (const char*) & ackpacket, 4, 0, (struct sockaddr *) & fromsock, froml);
  }

  if ( operation == 2 ) // WRQ
  {
      ReceiveTFTPFile(filename, clsock, &fromsock, froml);
  }
  else if ( operation == 1 ) // RRQ
  {
      TransmitTFTPFile(filename, clsock, &fromsock, froml);
  }

  fflush(stdout);
  shutdown(clsock, 2);
  write(1, "Exiting thread..!\n", 18);
  exit(0);
}

int
TFTPServer(unsigned int port)
{
  printf("TFTPServer\n");
  int sock, length, fromlen, n;
  struct sockaddr_in server;
  struct sockaddr_in from;

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if ( sock < 0 )
      error("Opening socket");

  length = sizeof (server);
  bzero(&server, length);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);

  if ( bind(sock, (struct sockaddr *) & server, length) < 0 )
      error("binding master port for atftp!");
  fromlen = sizeof (struct sockaddr_in);
  char filename[512];

  unsigned int fork_res, packeterror = 0;
  while (1)
  {
      struct TFTP_PACKET request = { 0 };
      printf("\n Waiting for a tftp client \n");
      n = recvfrom(sock, (char*) & request, sizeof (request), 0, (struct sockaddr *) & from, &fromlen);
      if ( n < 0 ) error("recvfrom");
      packeterror = 0;
      // DISASSEMBLE TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      //             2 bytes              1 byte         1byte
      // RRQ/WRQ   | opcode |  filename  |  0  |  Mode  |  0
      //               A          B         C       D      E
      // A part
      if ( (request.Op1 != 0) || ((request.Op2 != 2) && (request.Op2 != 1)) )
      {
          packeterror = 1;
      }
      // B part
      //write(1, request.data, n - 2);
      strcpy(filename, request.data);
      unsigned int fnm_end = strlen(filename);
      if ( fnm_end == 0 )
      {
          packeterror = 1;
      }
      // DISASSEMBLE TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      if ( packeterror == 0 )
      {
          fork_res = fork();
          if ( fork_res < 0 )
          {
              printf("Could not fork server , client will fail\n");
              //TODO ADD ERROR PACKET
          }
          else if ( fork_res == 0 )
          {
              int f_fromlen = fromlen;
              struct sockaddr_in f_from = from;
              printf("New UDP server fork to serve file %s , operation type %u \n", filename, request.Op2);
              fflush(stdout);
              HandleClient(filename, f_fromlen, f_from, request.Op2);
          }
          else
          {
              /* Server loop */
          }
      }
      else
      {
          printf("TFTP Server master thread - Incoming Request Denied..\n");
          fflush(stdout);
          write(1, request.data, n - 2);
      }
  }
  return;
}

//     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//                      CLIENT  PART 
//
//     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// AN OPERATION = 1 read | An OPERATION = 2 write

int
TFTPClient(char * server_ip, unsigned int port, char * filename, int operation)
{
  if ( (operation != WRITE) && (operation != READ) )
  {
      printf("Wrong Client Operation \n");
      return 1;
  }
  if ( strlen(filename) <= 0 )
  {
      printf("Wrong filename \n");
      return 1;
  }
  int sock, length, n;
  struct sockaddr_in server;
  struct hostent *hp;
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if ( sock < 0 ) error("Could not open socket for TFTP connection");

  struct timeval timeout_time = { 0 };
  timeout_time.tv_sec = 20;
  int i = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *) & timeout_time, sizeof ( struct timeval));
  if ( i != 0 )
  {
      printf("Error setting receive Timeout for client socket \n");
      printerror(errno);
  }
  i = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (struct timeval *) & timeout_time, sizeof ( struct timeval));
  if ( i != 0 )
  {
      printf("Error setting send Timeout for client socket \n");
      printerror(errno);
  }

  //SERVER EINAI TO SOCKADDR_IN POU PERIEXEI TIN DIEYTHINSI TOU MAIN TFTP SERVER ( PORT 69 PX ) 
  //FROM EINAI TO SOCKADDR_IN POU PERIEXEI TIN DIEYTHINSI TOU CLIENT TFTP BIND GIA NA LAMVANOUME MINIMATA 
  //TO EINAI TO SOCKADDR_IN POU PERIEXEI TIN DIEYTHINSI TOU DATA PORT POU ANOIGEI O TFTP SERVER ( PORT 30000 PX )

  // INITIALIZATION TOU SERVER SOCKADDR_IN
  server.sin_family = AF_INET;
  hp = gethostbyname(server_ip);
  if ( hp == 0 ) error("Unknown host for TFTP connection ");
  bcopy((char *) hp->h_addr, (char *) & server.sin_addr, hp->h_length);

  server.sin_port = htons(port);
  length = sizeof (struct sockaddr_in);



  // BIND CODE GIA NA LAMVANOUME TA MINIMATA APO TON SERVER
  struct sockaddr_in from, to;

  // BIND CODE GIA NA LAMVANOUME TA MINIMATA APO TON SERVER
  bzero(&from, length);
  from.sin_family = AF_INET;
  from.sin_addr.s_addr = INADDR_ANY;
  unsigned int cl_port = cl_port = FindFreePortInRange(sock, &from);
  if ( (cl_port == 0) || (ntohs(from.sin_port) == 0) )
  {
      printf("Client will be unable to receive messages , so it will now quit ( %u , %u ) \n", cl_port, ntohs(from.sin_port));
      exit(0);
  }
  // BIND CODE GIA NA LAMVANOUME TA MINIMATA APO TON SERVER



  // TO CODE GIA NA LAMVANOUME TA MINIMATA APO TON SERVER
  bzero(&to, length);
  to.sin_family = AF_INET; //from.sin_addr.s_addr=INADDR_ANY;
  hp = gethostbyname(server_ip);
  if ( hp == 0 ) error("Unknown host for TFTP connection ");
  bcopy((char *) hp->h_addr, (char *) & to.sin_addr, hp->h_length);
  // TO CODE GIA NA LAMVANOUME TA MINIMATA APO TON SERVER



  // MAKE TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  struct TFTP_PACKET request;
  //             2 bytes              1 byte         1byte
  // RRQ/WRQ   | opcode |  filename  |  0  |  Mode  |  0
  //               A          B         C       D      E
  // A part
  request.Op1 = 0;
  request.Op2 = operation;
  // B part
  strcpy(request.data, filename);
  unsigned int fnm_end = strlen(filename);
  // C part
  request.data[fnm_end++] = 0;
  // D part
  // Mode forced octet
  request.data[fnm_end++] = 'o';
  request.data[fnm_end++] = 'c';
  request.data[fnm_end++] = 't';
  request.data[fnm_end++] = 'e';
  request.data[fnm_end++] = 't';
  // E part
  request.data[fnm_end++] = 0;
  // MAKE TFTP PACKET! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

  //STELNOUME TO PRWTO PAKETO PROS TO PORT 69 TOU SERVER
  n = sendto(sock, (const char*) & request, fnm_end + 2, 0, (struct sockaddr *) & server, length);
  if ( n < 0 ) error("Sending initial TFTP packet");
  //Mexri edw exoume steilei i RRQ , i WRQ to opoio einai sigouro..
  // Vazoume from anti server giati o server einai i socket pros to port 69 , to from einai to 2o port
  if ( operation == READ ) // READ OPERATION
  {
      ReceiveTFTPFile(filename, sock, &to, length);
  }
  else if ( operation == WRITE ) // WRITE OPERATION
  {
      ReceiveNullACK(sock, &to, length); // Fortwnoume sto to , pou theloume na stelnoume to minima mas!
      TransmitTFTPFile(filename, sock, &to, length);
  }
  printf("Stopping TFTP client..\n");
  shutdown(sock, 2);
  return (0);
}


