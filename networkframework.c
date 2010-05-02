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

/* ********************** *
 * GENERIC FUNCTIONS PART *
 * ********************** */

/* check the verbosity level */
inline int
trivial_msg()
{
	return verbosity > 2;
}

inline int
debug_msg()
{
	return verbosity > 1;
}

	inline int
error_msg()
{
	return verbosity > 0;
}

/* in case of error, print error message and exit */
	void
error(char *msg)
{
	if ( !error_msg() )
	{
		exit(EXIT_SUCCESS);
	}
	perror(strcat("ERROR: ", msg));
	exit(EXIT_SUCCESS);
}

/* clear the errno var, to update it later
 * we do not care about all errors */
	inline void
clrerrno()
{
	errno = 0;
}

/* print error message according to errno */
	void
printerror(int errnum)
{
	if ( !error_msg() )
		return;
	if ( errnum == 0 )
		fprintf(outstrm, "No error \n");
	else if ( errnum == EBADF )
		fprintf(outstrm, "The argument is an invalid descriptor\n");
	else if ( errnum == ECONNREFUSED )
		fprintf(outstrm, "A remote host refused to allow the network connection\n");
	else if ( errnum == ENOTCONN )
		fprintf(outstrm, "The socket is associated with a connection oriented protocol and is not connected\n");
	else if ( errnum == ENOTSOCK )
		fprintf(outstrm, "The argument does not refer to a socket\n");
	else if ( errnum == EAGAIN )
		fprintf(outstrm, "Timeout expired / The socket is marked non-blocking and the op would block \n");
	else if ( errnum == EINTR )
		fprintf(outstrm, "Receive interrupted by signal\n");
	else if ( errnum == EFAULT )
		fprintf(outstrm, "The receive buffer is outside the process address space\n");
	else if ( errnum == EINVAL )
		fprintf(outstrm, "Invalid argument passed\n");
	else if ( errnum == ENOPROTOOPT )
		fprintf(outstrm, "Option not supported by the protocol\n");
	else if ( errnum == EDOM )
		fprintf(outstrm, "Too big values to fit\n");
	else if ( errnum == 98 )
		fprintf(outstrm, "Address already in use!\n");
	else if ( errnum == EAFNOSUPPORT )
		fprintf(outstrm, "Address family not supported by protocol in use!\n");
	else
		fprintf(outstrm, "Unknown error %i ", errnum);
	fflush(stdout);
}

/* check if $filename exists and can be read,
 * if it can be read, then it can be sent */
	int
fcheck(const char* filename)
{
	return access(filename, F_OK) & access(filename, R_OK);
}

	int
OpCodeValidTFTP(unsigned char op1, unsigned char op2)
{
	if ( op1 != 0 )
	{
		fprintf(outstrm, "Wrong TFTP magic number! %u \n", op1);
		fflush(stdout);
		return EXIT_FAILURE;
	}
	else
	{
		switch (op2)
		{
			case 1:
				if ( debug_msg() ) fprintf(outstrm, "RRQ - Read Request\n");
				break;
			case 2:
				if ( debug_msg() ) fprintf(outstrm, "WRQ - Write Request\n");
				break;
			case 3:
				if ( debug_msg() ) fprintf(outstrm, "Data Message - Should not be accepted here\n");
				break;
			case 4:
				if ( debug_msg() ) fprintf(outstrm, "ACK - Data Acknowledge , Should not be accepted here\n");
				break;
			case 5:
				if ( debug_msg() ) fprintf(outstrm, "ERROR - Error Request\n");
				break;
			default:
				return EXIT_FAILURE;
				break;
		}
	}
	return EXIT_SUCCESS;
}

	int
ReceiveNullACK(int server_sock, struct sockaddr_in * client_sock, int client_length)
{
	clrerrno();
	/* MAKE ACK TFTP PACKET! */
	struct ACK_TFTP_PACKET ackpacket;
	/*          2 bytes  2 bytes
	 * ACK    | opcode | block #
	 *            A         B
	 */
	/* A part */
	ackpacket.Op1 = 0;
	ackpacket.Op2 = 4;
	/* B part */
	ackpacket.Block = 0;
	/* MAKE ACK TFTP PACKET! */
	int datarecv, retransmit_attempts = 1;
	struct sockaddr_in recv_tmp;
	while ((retransmit_attempts != 0) && (retransmit_attempts < MAX_FAILED_RETRIES))
	{
		if ( debug_msg() )
		{
			fprintf(outstrm, "Waiting to receive null acknowledgement\n");
		}
		fflush(stdout);
		/* RECEIVE ACKNOWLEDGMENT! */
		datarecv = recvfrom(server_sock, (char*) & ackpacket, 4, 0,
				(struct sockaddr *) & recv_tmp, & client_length);
		if ( datarecv < 0 )
		{
			if ( error_msg() )
				fprintf(outstrm, "Error while receiving null acknowledgement \n");
			printerror(errno);
			++retransmit_attempts;
			return EXIT_FAILURE;
		}
		else
		{
			if ( debug_msg() )
				fprintf(outstrm, "Received acknowledgement for block %u ( waiting for 0 ) \n",
						ackpacket.Block);
			if ( (ackpacket.Op1 != 0) || (ackpacket.Op2 != 4) )
			{
				fprintf(outstrm, "\nWrong Packet %u %u ( instead of 0 4 )!\n",
						ackpacket.Op1, ackpacket.Op2);
				++retransmit_attempts;
			}
			else if ( ackpacket.Block != 0 )
			{
				fprintf(outstrm, "\n\nOut of sync acknowledge.!\n\n");
				++retransmit_attempts;
			}
			else
			{
				fprintf(outstrm, "\nNull Acknowledge received!!\n");
				retransmit_attempts = 0;
				break;
			}
		}
	}
	if ( debug_msg() )
	{
		fprintf(outstrm, "Stopping null acknowledgement wait..\n");
	}
	if ( ntohs(recv_tmp.sin_port) == 0 )
	{
		fprintf(outstrm, "Null acknowledgement returned null port , failed..\n");
		return EXIT_FAILURE;
	}
	else
	{
		if ( debug_msg() )
			fprintf(outstrm, "Null acknowledgment revealed %s port %u ,family %i\n",
					inet_ntoa(recv_tmp.sin_addr), ntohs(recv_tmp.sin_port), recv_tmp.sin_family);
		*client_sock = recv_tmp;
	}
	return EXIT_SUCCESS;
}

	int
TransmitError(char * message, unsigned short errorcode, int sock, struct sockaddr_in* peer)
{
	if ( debug_msg() )
		fprintf(outstrm, "\n Transmitting error message `%s` ( code %i ) to %s:%u \n",
				message, errorcode, inet_ntoa(peer->sin_addr), ntohs(peer->sin_port));
	/* ERROR TFTP PACKET ASSEMBLY! */
	struct ERROR_TFTP_PACKET error;
	/*          2 bytes    2 bytes         1byte
	 * ERROR   | opcode | errorcode | MSG |  0
	 *             A          B        C     D
	 */
	/* A part */
	error.Op1 = 0;
	error.Op2 = 5;
	/* B part */
	error.ErrorCode = errorcode;
	/* C part */
	strcpy(error.data, message);
	/* D part */
	error.data[strlen(message)] = 0;
	/* ERROR TFTP PACKET ASSEMBLY COMPLETE! */

	/* convert to network byte order */
	error.ErrorCode = htons(error.ErrorCode);
	int n = sendto(sock, (const char*) & error, strlen(message) + 4, 0,
			(struct sockaddr *) & peer, sizeof (struct sockaddr_in));
	/* convert back to out byte order */
	error.ErrorCode = ntohs(error.ErrorCode);
	if ( n < 0 )
	{
		if ( error_msg() )
			fprintf(outstrm, "Failed to transmit error message! , no retries \n ");
		printerror(errno);
		return EXIT_FAILURE;
	}
	else if ( n < (int) strlen(message) + 4 )
	{
		if ( error_msg() )
			fprintf(outstrm, "Failed to transmit complete error message! , no retries \n ");
		return EXIT_FAILURE;
	}
	else if ( debug_msg() )
	{
		fprintf(outstrm, "Tried my best to forward error message! , no retries \n ");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

	int
FindFreePortInRange(int thesock, struct sockaddr_in* server)
{
	clrerrno();
	unsigned int cl_port = MINDATAPORT;
	int bindres = 0, length = sizeof (struct sockaddr_in);
	while (cl_port <= MAXDATAPORT)
	{
		server->sin_port = htons(cl_port);
		bindres = bind(thesock, (struct sockaddr *) server, length);
		if ( bindres < 0 )
		{
			/* we don't need to log every bind step */
			// fprintf(outstrm, "Binding port number %u is not available \n", cl_port);
			++cl_port;
		}
		else
		{
			if ( debug_msg() )
				fprintf(outstrm, "Binding to port number %u is OK \n", cl_port);
			break;
		}
	}
	if ( cl_port > MAXDATAPORT )
	{
		fprintf(outstrm, "Unable to handle client  ( could not find free port ).. :( \n");
		fflush(stdout);
		cl_port = 0;
	}
	/* ignore bind errors */
	clrerrno();
	return cl_port;
}

	int
TransmitTFTPFile(const char * filename, int server_sock, struct sockaddr_in * client_pout_sock, int client_length)
{
	clrerrno();
	struct sockaddr_in client_send_sockaddr;
	client_send_sockaddr = *client_pout_sock;
	if ( debug_msg() )
		fprintf(outstrm, "TransmitTFTPFile ( Opening local file for read ) called\n");
	if ( debug_msg() )
		fprintf(outstrm, "TransmitTFTPFile to %s port %u\n",
				inet_ntoa(client_send_sockaddr.sin_addr),
				ntohs(client_send_sockaddr.sin_port));
	FILE *filetotransmit;
	filetotransmit = fopen(filename, "rb");
	unsigned int retransmit_attempts = 0;
	if ( filetotransmit != NULL )
	{
		/* FILE CAN BE OPENED , CHECK FILE SIZE */
		unsigned int filesize = 0, filepos = 0;
		int datatrans = 0, datarecv = 0, dataread = 0;
		fseek(filetotransmit, 0, SEEK_END);
		filesize = ftell(filetotransmit);
		rewind(filetotransmit);

		if ( debug_msg() )
			fprintf(outstrm, "Requested file ( %s ) for transmission has %u bytes size!\n",
					filename, filesize);
		if ( (filesize % 512 == 0) && (filesize != 0) )
			if ( debug_msg() )
				fprintf(outstrm, "File size is a multiple of 512 should append a zero data package \n");
		/* FILE SIZE OK */
		/* MAKE ACK TFTP PACKET! */
		struct ACK_TFTP_PACKET ackpacket;
		/*          2 bytes  2 bytes
		 * ACK    | opcode | block #
		 *            A        B
		 */
		/* A part */
		ackpacket.Op1 = 0;
		ackpacket.Op2 = 4;
		/* B part */
		ackpacket.Block = 0;
		/* MAKE ACK TFTP PACKET! */

		/* MAKE DATA TFTP PACKET! */
		struct DATA_TFTP_PACKET request;
		/*          2 bytes  2 bytes   n bytes
		 * DATA   | opcode | block # | Data
		 *            A         B        C
		 */
		/* A part */
		request.Op1 = 0;
		request.Op2 = 3;
		/* B part */
		request.Block = 0;
		/* C part */
		request.data[0] = 0;
		/* MAKE DATA TFTP PACKET! */
		filepos = 0;
		datatrans = 0;

		struct sockaddr_in client_in_sock;

		while ((filepos < filesize)
				|| ((retransmit_attempts != 0) && (retransmit_attempts < MAX_FAILED_RETRIES))
				|| ((filepos >= filesize) && (dataread == 512)))
		{
			//ORIAKI PERIPTWSI POU TO TELEYTAIO MINIMA EINAI 512 Byte
			if ( (filepos >= filesize) && (dataread == 512) )
			{
				if ( debug_msg() )
					fprintf(outstrm, "Last message is 512 characters so Recipient "
							"won`t be able to stop. Sending a zero data \n");
				dataread = 0;
				retransmit_attempts = 1;
				request.data[0] = 0;
				++request.Block;
			}
			//ORIAKI PERIPTWSI POU TO TELEYTAIO MINIMA EINAI 512 Byte

			if ( retransmit_attempts == 0 )
			{
				//AN DEN KANOUME RENTRASMIT TOTE DIAVAZOUME KAINOURGIO BLOCK APO TO ARXEIO
				//READ DATA APO TO TOPIKO ARXEIO
				dataread = fread(request.data, 1, 512, filetotransmit);
				if ( dataread != 512 && ferror(filetotransmit) )
				{
					fprintf(outstrm, "Error while reading file %s \n", filename);
					fclose(filetotransmit);
					return EXIT_FAILURE;
				}
				//SEND DATA STO ALLO MELLOS TOU SESSION
				++request.Block;
				filepos += dataread;
			}

			if ( trivial_msg() )
				fprintf(outstrm, "Sending data %u , block %u \n", dataread + 4, request.Block);
			fflush(stdout);
			/* convert to network byte order */
			request.Block = htons(request.Block);
			datatrans = sendto(server_sock, (const char*) & request, dataread + 4, 0,
					(struct sockaddr *) & client_send_sockaddr, client_length);
			/* convert back to out byte order */
			request.Block = ntohs(request.Block);
			if ( datatrans < 0 )
			{
				if ( error_msg() )
					fprintf(outstrm, "Error while sending file %s ( conn family %i  , AF_INET = %i)",
							filename, client_send_sockaddr.sin_family, AF_INET);
				printerror(errno);
				fclose(filetotransmit);
				return EXIT_FAILURE;
			}

			//RECEIVE ACKNOWLEDGMENT!
			if ( trivial_msg() )
				fprintf(outstrm, "Waiting to receive acknowledgement\n");
			fflush(stdout);
			datarecv = recvfrom(server_sock, (char*) & ackpacket, 4, 0,
					(struct sockaddr *) & client_in_sock, &client_length);
			/* convert back to out byte order */
			ackpacket.Block = ntohs(ackpacket.Block);
			if ( datarecv < 0 )
			{
				if ( error_msg() )
					fprintf(outstrm, "Error while receiving acknowledgement for file %s \n", filename);
				printerror(errno);
				++retransmit_attempts;
				return EXIT_FAILURE;
			}
			else
			{
				if ( trivial_msg() )
					fprintf(outstrm, "Received ACK ( size %u ) for block %u , currently @ %u \n",
							datarecv, ackpacket.Block, request.Block);
				if ( (ackpacket.Op1 != 0) || (ackpacket.Op2 != 4) )
				{
					if ( error_msg() )
						fprintf(outstrm, "Incorrect acknowledgment magic numbers ( %u %u instead of 0 4 )  \n",
								ackpacket.Op1, ackpacket.Op2);
				}
				else if ( ackpacket.Block != request.Block )
				{
					if ( error_msg() )
						fprintf(outstrm, "\n\nOut of sync acknowledge Should send error ( TODO ).!\n\n");
				}
				retransmit_attempts = 0;
			}
			/* RECEIVE ACKNOWLEDGMENT! */
			if ( trivial_msg() )
				fprintf(outstrm, "Data from file read on this loop is %u ", dataread);
		}
		fclose(filetotransmit);
	}
	else
	{
		if ( error_msg() )
			fprintf(outstrm, "Could not open file %s \n", filename);
		return EXIT_FAILURE;
	}
	if ( debug_msg() )
		fprintf(outstrm, "TransmitTFTPFile has finished\n");
	return EXIT_SUCCESS;
}

	int
ReceiveTFTPFile(const char * filename, int server_sock, struct sockaddr_in * client_pout_sock, int client_length)
{
	clrerrno();
	struct sockaddr_in client_send_sockaddr;
	client_send_sockaddr = *client_pout_sock;
	if ( debug_msg() )
		fprintf(outstrm, "ReceiveTFTPFile  ( Opening local file for write )  called\n");
	if ( debug_msg() )
		fprintf(outstrm, "ReceiveTFTPFile from %s port %u\n",
				inet_ntoa(client_send_sockaddr.sin_addr),
				ntohs(client_send_sockaddr.sin_port));
	if ( (debug_msg()) && (ntohs(client_send_sockaddr.sin_port) == 0) )
		fprintf(outstrm, "In case you are running the client and you see 0.0.0.0 "
				"and port 0 thats normal , we haven't got a packet yet!");

	FILE *filetotransmit;
	filetotransmit = fopen(filename, "wb");
	if ( filetotransmit != NULL )
	{
		/* FILE CAN BE OPENED , CHECK FILE SIZE */
		unsigned int filepos = 0;
		int datarecv = 0, datawrite = 0, datatrans = 0, reachedend = 0;
		/* MAKE ACK TFTP PACKET! */
		struct ACK_TFTP_PACKET ackpacket;
		/*          2 bytes  2 bytes
		 * ACK    | opcode | block #
		 *            A         B
		 */
		/* A part */
		ackpacket.Op1 = 0;
		ackpacket.Op2 = 4;
		/* B part */
		ackpacket.Block = 0;
		/* MAKE ACK TFTP PACKET! */

		/* MAKE DATA TFTP PACKET! */
		struct DATA_TFTP_PACKET request;
		/*          2 bytes   2 bytes  n bytes
		 * DATA   | opcode | block # | Data
		 *            A         B        C
		 */
		/* A part */
		request.Op1 = 0;
		request.Op2 = 3;
		/* B part */
		request.Block = 0;
		/* C part */
		request.data[0] = 0;
		/* MAKE DATA TFTP PACKET! */
		filepos = 0;
		datatrans = 0;
		reachedend = 0;

		while (reachedend == 0)
		{
			/* RECEIVE DATA */
			if ( trivial_msg() )
				fprintf(outstrm, "Waiting to receive data\n");
			fflush(stdout);
			// VAZW CLIENT_OUT_SOCK ETSI WSTE NA PERNOUME KATEYTHEIAN TIN PEER ADDRESS K NA MIN YPARXEI PROB
			datarecv = recvfrom(server_sock, (char*) & request, sizeof (request), 0,
					(struct sockaddr *) & client_send_sockaddr, &client_length);
			/* convert back to out byte order */
			request.Block = ntohs(request.Block);

			if ( datarecv < 0 )
			{
				if ( error_msg() )
					fprintf(outstrm, "Error while receiving file %s \n", filename);
				printerror(errno);
				fclose(filetotransmit);
				return EXIT_FAILURE;
			}
			else
			{
				if ( trivial_msg() )
					fprintf(outstrm, "Received %u bytes from socket ", datarecv);
				request.data[datarecv - 4] = 0;
			}

			if ( datarecv < (int) sizeof (request) )
			{
				reachedend = 1;
				if ( debug_msg() )
					fprintf(outstrm, "This should be the last packet \n");
			}

			/* READ DATA */
			if ( datarecv - 4 == 0 )
			{
				if ( error_msg() )
					fprintf(outstrm, "Will not write to file zero .. \n");
			}
			else
			{
				datawrite = fwrite(request.data, 1, datarecv - 4, filetotransmit);
				// fprintf(outstrm, "Data2Write(%s)\n",request.data);
				// fflush(stdout);
			}

			if ( datawrite != 512 && ferror(filetotransmit) )
			{
				fprintf(outstrm, "Error while writing file %s \n", filename);
				fclose(filetotransmit);
				return EXIT_FAILURE;
			}
			/* SEND ACKNOWLEDGMENT */
			++ackpacket.Block;
			if ( trivial_msg() )
				fprintf(outstrm, "Sending acknowledgement %u \n", ackpacket.Block);
			fflush(stdout);
			/* convert to network byte order */
			ackpacket.Block = htons(ackpacket.Block);
			datatrans = sendto(server_sock, (const char*) & ackpacket, 4, 0,
					(struct sockaddr *) & client_send_sockaddr, client_length);
			/* convert back to out byte order */
			ackpacket.Block = ntohs(ackpacket.Block);
			if ( datatrans < 0 )
			{
				if ( error_msg() )
					fprintf(outstrm, "Error while sending acknowledgment %s ", filename);
				fclose(filetotransmit);
				return EXIT_FAILURE;
			}
		}
		fclose(filetotransmit);
	}
	else
	{
		if ( error_msg() )
			fprintf(outstrm, "Could not open file %s \n", filename);
		return EXIT_FAILURE;
	}
	if ( debug_msg() )
		fprintf(outstrm, "ReceiveTFTPFile has finished\n");
	return EXIT_SUCCESS;
}

/* *********** *
 * SERVER PART *
 * *********** */
	int
HandleClient(unsigned char * filename, int froml, struct sockaddr_in fromsock, int operation)
{
	if ( debug_msg() )
		fprintf(outstrm, "HandleClient from address %s port %u\n",
				inet_ntoa(fromsock.sin_addr), ntohs(fromsock.sin_port));
	int clsock, length, n;
	struct sockaddr_in server;

	clsock = socket(AF_INET, SOCK_DGRAM, 0);
	if ( clsock < 0 )
	{
		if ( error_msg() )
			fprintf(outstrm, "Unable to handle new client ( could not create socket ).. :( \n");
		fflush(stdout);
		exit(EXIT_SUCCESS);
	}
	/* SET TIMEOUT FOR OPERATIONS */
	struct timeval timeout_time = { ZERO, MAX_WAIT };
	int reslt = setsockopt(clsock, SOL_SOCKET, SO_RCVTIMEO,
			(struct timeval *) & timeout_time,
			sizeof ( struct timeval));
	if ( reslt != 0 )
	{
		if ( error_msg() )
			fprintf(outstrm, "Error setting receive Timeout for serving socket \n");
		printerror(errno);
	}
	reslt = setsockopt(clsock, SOL_SOCKET, SO_SNDTIMEO,
			(struct timeval *) & timeout_time,
			sizeof ( struct timeval));
	if ( reslt != 0 )
	{
		if ( error_msg() )
			fprintf(outstrm, "Error setting send Timeout for serving socket \n");
		printerror(errno);
	}
	/* SET TIMEOUT FOR OPERATIONS */
	length = sizeof (struct sockaddr_in); //= sizeof (server);
	bzero(&server, length);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;

	unsigned int cl_port = MINDATAPORT;
	/* BIND CODE GIA NA LAMVANOUME TA MINIMATA APO TON SERVER */
	cl_port = FindFreePortInRange(clsock, &server);
	if ( (cl_port == 0) || (ntohs(server.sin_port) == 0) )
	{
		if ( error_msg() )
			fprintf(outstrm, "Server  will be unable to receive messages , so it will now quit ( %u , %u ) \n",
					cl_port, ntohs(server.sin_port));
		exit(EXIT_SUCCESS);
	}
	/* BIND CODE GIA NA LAMVANOUME TA MINIMATA APO TON SERVER */
	if ( operation == WRITE )
	{
		// IF client sends WRQ we reply with a ACK 0 message!
		struct ACK_TFTP_PACKET ackpacket;
		/* MAKE TFTP PACKET! */
		/*        2 bytes  2 bytes
		 * ACK  | opcode | block #
		 *          A        B
		 */
		/* A part */
		ackpacket.Op1 = 0;
		ackpacket.Op2 = 4;
		/* B part */
		ackpacket.Block = 0;
		/* MAKE TFTP PACKET! */
		froml = sizeof (struct sockaddr_in); // <- KAKO DN KANEI :P
		clrerrno();
		if ( debug_msg() )
			fprintf(outstrm, "Trying to send null ACK to address %s port %u\n",
					inet_ntoa(fromsock.sin_addr), ntohs(fromsock.sin_port));
		n = sendto(clsock, (const char*) & ackpacket, 4, 0, (struct sockaddr *) & fromsock, froml);
		if ( n < 0 )
		{
			if ( error_msg() )
				fprintf(outstrm, "Could not send initial null acknowledge to reveal my port.. , failed \n");
			printerror(errno);
			exit(EXIT_SUCCESS);
		}
		else
		{
			if ( debug_msg() )
				fprintf(outstrm, "I just sent initial null acknowledgment to port %u \n",
						ntohs(fromsock.sin_port));
		}
	}
	if ( operation == WRITE )
	{
		ReceiveTFTPFile(filename, clsock, &fromsock, froml);
	}
	else if ( operation == READ )
	{
		TransmitTFTPFile(filename, clsock, &fromsock, froml);
	}

	fflush(stdout);
	//shutdown(clsock, 2);
	if ( debug_msg() )
		fprintf(outstrm, "Stopping handle client thread\n");
	exit(EXIT_SUCCESS);
}

	int
TFTPServer(unsigned int port)
{
	if ( debug_msg() )
		fprintf(outstrm, "TFTPServer\n");
	int sock, length, fromlen, n;
	struct sockaddr_in server, from;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if ( sock < 0 )
		error("Opening socket\n");

	length = sizeof (server);
	bzero(&server, length);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	if ( bind(sock, (struct sockaddr *) & server, length) < 0 )
	{
		error("binding master port for yatftp!\n");
	}
	fromlen = sizeof (struct sockaddr_in);
	char filename[512];
	int fork_res, packeterror = 0;
	while (1)
	{
		struct TFTP_PACKET request;
		if ( debug_msg() )
			fprintf(outstrm, "\n Waiting for a tftp client \n");
		n = recvfrom(sock, (char*) & request, sizeof (request), 0, (struct sockaddr *) & from, &fromlen);
		if ( n < 0 )
			error("recvfrom\n");
		/* DISASSEMBLE TFTP PACKET! */
		/*           2 bytes             1 byte         1byte
		 * RRQ/WRQ | opcode | filename  |  0  |  Mode  |  0
		 *             A         B         C       D      E
		 */
		/* A part */
		if ( (request.Op1 != 0) || ((request.Op2 != 2) && (request.Op2 != 1)) )
		{
			packeterror = 1;
		}
		/* B part */
		//write(1, request.data, n - 2);
		strcpy(filename, request.data);
		unsigned int fnm_end = strlen(filename);
		/* CHECK FOR INCORRECT FILENAMES! */
		if ( !fcheck(filename) )
		{
			packeterror = 1;
			fprintf(outstrm, "Cannot access file %s, failing packet \n", filename);
			TransmitError("Cannot access file", 2, sock, &from);
		}
		else if ( fnm_end == 0 )
		{
			packeterror = 1;
			if ( error_msg() )
				fprintf(outstrm, "Null filename (%s) , failing packet \n", filename);
			TransmitError("Null filename", 3, sock, &from);
		}
		/* DISASSEMBLE TFTP PACKET! */
		if ( !packeterror )
		{
			fork_res = fork();
			if ( fork_res < 0 )
			{
				if ( error_msg() )
					fprintf(outstrm, "Could not fork server , client will fail\n");
				TransmitError("Cannot fork accept connection", 1, sock, &from);
			}
			else if ( fork_res == 0 )
			{
				int f_fromlen = fromlen;
				struct sockaddr_in f_from = from;
				if ( debug_msg() )
					fprintf(outstrm, "New UDP server fork to serve file %s , operation type %u \n",
							filename, request.Op2);
				fflush(stdout);
				/* check if root */
				if ( getuid() == ROOT_ID )
				{
					setuid(USER_ID);
					fprintf(outstrm, "Switched from root(uid=%d) to normal user(uid=%d)\n",
							ROOT_ID, getuid());
				}
				HandleClient(filename, f_fromlen, f_from, request.Op2);
			}
			else
			{
				/* Server loop */
			}
		}
		else
		{
			fprintf(outstrm, "TFTP Server master thread - Incoming Request Denied..\n");
			fflush(stdout);
			write(1, request.data, n - 2);
		}
	}
	return EXIT_SUCCESS;
}

/* *********** *
 * CLIENT PART *
 * *********** */
	int
TFTPClient(char * server_ip, unsigned port, const char * filename, const int operation)
{
	if ( (operation != WRITE) && (operation != READ) )
	{
		if ( error_msg() )
			fprintf(outstrm, "Wrong Client Operation \n");
		return EXIT_FAILURE;
	}
	if ( strlen(filename) <= 0 )
	{
		if ( error_msg() )
			fprintf(outstrm, "Wrong filename \n");
		return EXIT_FAILURE;
	}
	int sock, length, n;
	struct sockaddr_in server;
	struct hostent *hp;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if ( sock < 0 )
	{
		if ( error_msg() )
			fprintf(outstrm, "Error creating socket for TFTP connection , failed.. \n");
		printerror(errno);
		exit(EXIT_SUCCESS);
	}
	struct timeval timeout_time = { 0, MAX_WAIT };
	int reslt = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
			(struct timeval *) & timeout_time,
			sizeof ( struct timeval));
	if ( reslt != 0 )
	{
		if ( error_msg() )
			fprintf(outstrm, "Error setting receive Timeout for client socket \n");
		printerror(errno);
	}
	reslt = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
			(struct timeval *) & timeout_time,
			sizeof ( struct timeval));
	if ( reslt != 0 )
	{
		if ( error_msg() )
			fprintf(outstrm, "Error setting send Timeout for client socket \n");
		printerror(errno);
	}

	/* SERVER EINAI TO SOCKADDR_IN POU PERIEXEI TIN DIEYTHINSI TOU MAIN TFTP SERVER ( PORT 69 PX )
	 * FROM EINAI TO SOCKADDR_IN POU PERIEXEI TIN DIEYTHINSI TOU CLIENT TFTP BIND GIA NA LAMVANOUME MINIMATA
	 * TO EINAI TO SOCKADDR_IN POU PERIEXEI TIN DIEYTHINSI TOU DATA PORT POU ANOIGEI O TFTP SERVER ( PORT 30000 PX )
	 */

	/* INITIALIZATION TOU SERVER SOCKADDR_IN */
	server.sin_family = AF_INET;
	hp = gethostbyname(server_ip);
	if ( hp == 0 )
	{
		error("Unknown host for TFTP connection\n");
	}
	bcopy((char *) hp->h_addr, (char *) & server.sin_addr, hp->h_length);

	server.sin_port = htons(port);
	length = sizeof (struct sockaddr_in);

	/* needed SOCKADDR structures */
	struct sockaddr_in from, to;
	/* BIND CODE GIA NA LAMVANOUME TA MINIMATA APO TON SERVER */
	bzero(&from, length);
	from.sin_family = AF_INET;
	from.sin_addr.s_addr = INADDR_ANY;

	unsigned cl_port = 0;
	cl_port = FindFreePortInRange(sock, &from);
	if ( (cl_port == 0) || (ntohs(from.sin_port) == 0) )
	{
		if ( error_msg() )
			fprintf(outstrm, "Client will be unable to receive messages , so it will now quit ( %u , %u ) \n",
					cl_port, ntohs(from.sin_port));
		exit(EXIT_SUCCESS);
	}
	/* BIND CODE GIA NA LAMVANOUME TA MINIMATA APO TON SERVER */

	/* TO CODE GIA NA STELNOUME TA MINIMATA APO TON SERVER */
	bzero(&to, length);
	to.sin_family = AF_INET;
	//to.sin_addr.s_addr=INADDR_ANY;
	//hp = gethostbyname(server_ip);
	//if ( hp == 0 ) error("Unknown host for TFTP connection ");
	//bcopy((char *) hp->h_addr, (char *) & to.sin_addr, hp->h_length);

	/* MAKE TFTP PACKET! */
	struct TFTP_PACKET request;
	/*           2 bytes            1 byte          1byte
	 * RRQ/WRQ | opcode | filename |  0   |  Mode  |  0
	 *             A         B        C        D      E
	/* A part */
	request.Op1 = 0;
	request.Op2 = operation;
	/* B part */
	strcpy(request.data, filename);
	unsigned int fnm_end = strlen(filename);
	/* C part */
	request.data[fnm_end++] = 0;
	/* D part */
	// Mode forced octet
	request.data[fnm_end++] = 'o';
	request.data[fnm_end++] = 'c';
	request.data[fnm_end++] = 't';
	request.data[fnm_end++] = 'e';
	request.data[fnm_end++] = 't';
	/* E part */
	request.data[fnm_end] = 0;
	/* MAKE TFTP PACKET! */

	/* STELNOUME TO PRWTO PAKETO PROS TO PORT 69 TOU SERVER */
	n = sendto(sock, (const char*) & request, fnm_end + 2, 0, (struct sockaddr *) & server, length);
	if ( n < 0 )
	{
		if ( error_msg() )
			fprintf(outstrm, "Error sending initial TFTP packet , failed.. \n");
		printerror(errno);
		exit(EXIT_SUCCESS);
	}
	/* Mexri edw exoume steilei i RRQ , i WRQ to opoio einai sigouro.
	 * Vazoume from anti server giati o server einai i socket pros to port 69 ,
	 * to from einai to 2o port */
	if ( operation == READ )
	{
		ReceiveTFTPFile(filename, sock, &to, length);
	}
	else if ( operation == WRITE )
	{
		if ( ReceiveNullACK(sock, &to, length) )
		{
			if ( error_msg() )
				fprintf(outstrm, "Cannot receive zero acknowledge to start file transmission , failing..");
		}
		else
		{
			TransmitTFTPFile(filename, sock, &to, length);
		}
	}
	if ( debug_msg() )
		fprintf(outstrm, "Stopping TFTP client..\n");
	//shutdown(sock, 2);
	return EXIT_SUCCESS;
}
