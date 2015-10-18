/* LED Message Board - lmbd.cpp
 ** Copyright (c) 2014 Justin Hammond
 **
 **  This program is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 2 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program; if not, write to the Free Software
 **  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 **  USA
 **
 ** LED Message Board SVN Identification:
 ** $Rev$
 */

/** @file lmbd.cpp
 *  @brief
 */

#include <stdio.h>
#include <sys/types.h>
#include <iostream>
#include <iomanip>
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include <signal.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "libcli/libcli.h"
#include "serial/serial.h"
#include "lmbd.hpp"
#include "Driver-DX.hpp"
#include "FileMonitor.hpp"


#define CLITEST_PORT                8000




int main()
{

	int s, x;
	struct sockaddr_in addr;
	int on = 1;

#ifndef WIN32
	signal(SIGCHLD, SIG_IGN);
#endif
#ifdef WIN32
	if (!winsock_init()) {
		printf("Error initialising winsock\n");
		return 1;
	}
#endif

	boost::log::core::get()->set_filter
			(
					boost::log::trivial::severity >= boost::log::trivial::info
			);


	struct LMBCTX *lmbctx = new LMBCTX;

	lmbctx->driver = new Driver_DX();
	lmbctx->sp = NULL;
	lmbctx->port = "/dev/ttyUSB0";
	lmbctx->debug_level = 0;
	lmbctx->username = "Admin";
	lmbctx->password = "password";
	lmbctx->enablepass = "enable";
	lmbctx->max_cli = 5;
	lmbctx->monitorpath = "/tmp/test/";
	lmbctx->messages = 6;
	lmbctx->msgdisplay.resize(6);
	std::vector<std::string> displayedmsgs;

	try {
		lmbctx->load("/etc/LMBd.conf");
	} catch (...) {
		BOOST_LOG_TRIVIAL(warning) << "Could Not Load Config File";
	}

	lmbctx->driver->Init(lmbctx);
	lmbctx->driver->StartUp();
	lmbctx->driver->Fini();
	for (unsigned int i = 1; i <= lmbctx->messages; i++) {
		if (lmbctx->startupmsg[i].length() > 0) {
			lmbctx->driver->Init(lmbctx);
			lmbctx->driver->setMessage(i, lmbctx->startupmsg[i]);
			lmbctx->driver->Fini();

		}
	}


	FileMonitor fm;
	fm.init(lmbctx);



	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Socket Error";
		return 1;
	}
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(CLITEST_PORT);
	if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0)
	{
		BOOST_LOG_TRIVIAL(error) << "bind Error";
		return 1;
	}

	if (listen(s, 50) < 0)
	{
		BOOST_LOG_TRIVIAL(error) << "listen Error";
		return 1;
	}

	BOOST_LOG_TRIVIAL(info) << "Listening on port " << CLITEST_PORT;
	while ((x = accept(s, NULL, 0)))
	{
		if (lmbctx->Clients.size() < lmbctx->max_cli) {
			lmbctx->Clients.push_back(boost::make_shared<boost::thread>(Startcliloop, boost::ref(lmbctx), x));
		} else {
			BOOST_LOG_TRIVIAL(warning) << "Dropping Client Connection as we are at our Max";
			close(x);
		}
	}

	delete lmbctx;
	return 0;
}
