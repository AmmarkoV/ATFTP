YATFTP
======
Yet another tftp! 

A server-client implementation of trivial ftp server/client 

	------------------------------------------------------------------
	YATFTP 0.20 - get source @ git://github.com/AmmarkoV/ATFTP.git
	------------------------------------------------------------------

	Usage for TFTP client : 
		yatftp -r -f filename -a address -p port :	 read filename from address @ port 
		yatftp -w -f filename -a address -p port :	 write filename to address @ port 

	Usage for TFTP server : 
		yatftp -s [-p port]  :	 begin tftp server binded @ port (default #port 69)

		Common Options : 
		-l :	 log output to file tftp.log
		-q :	 silent mode, don't print any output
		-v :	 use verbose output
		-d :	 use debug output


