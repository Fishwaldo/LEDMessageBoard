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
#include <cstdio>
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

#include <boost/program_options.hpp>
#include "libcli/libcli.h"
#include "serial/serial.h"
#include "lmbd.hpp"
#include "Driver-DX.hpp"
#include "FileMonitor.hpp"



#define CLITEST_PORT                8000

LMB::Log::Logger *logger = NULL;


int main (int ac, char **av)
{
	int s, x;
	bool fork = true;
	struct sockaddr_in addr;
	int on = 1;
	boost::program_options::options_description desc("Allowed Options");

	desc.add_options()
		("help", "Command Line Options")
		("config", boost::program_options::value<std::string>(), "config file")
		("debug", boost::program_options::value<int>(), "Debug Level")
	;

#ifndef WIN32
	signal(SIGCHLD, SIG_IGN);
#endif
#ifdef WIN32
	if (!winsock_init()) {
		printf("Error initialising winsock\n");
		return 1;
	}
#endif


	boost::program_options::variables_map vm;
	boost::program_options::store(boost::program_options::parse_command_line(ac, av, desc), vm);
	boost::program_options::notify(vm);
	if (vm.count("help")) {
		std::cout << desc << std::endl;
		return 1;
	}

	boost::filesystem::path cfgpath;
	if (vm.count("config")) {
		cfgpath = vm["config"].as<std::string>();
	} else {
		cfgpath = "/etc/LMBd.conf";
	}
	if (!boost::filesystem::exists(cfgpath)) {
		std::cerr << "Can't find Config File at path " << cfgpath << std::endl;
		return -1;
	}

	if (vm.count("debug")) {
		fork = false;
		LMB::Log::Manager::setDefaultLevel(LMB::Log::Log::eDebug);
	} else
		LMB::Log::Manager::setDefaultLevel(LMB::Log::Log::eInfo);


	logger = new LMB::Log::Logger("LMBd");

	struct LMBCTX *lmbctx = new LMBCTX;

	lmbctx->driver = new Driver_DX();
	lmbctx->sp = NULL;
	lmbctx->port = "/dev/ttyUSB0";
	lmbctx->debug_level = 0;
	lmbctx->username = "Admin";
	lmbctx->password = "password";
	lmbctx->enablepass = "enable";
	lmbctx->max_cli = 5;
	lmbctx->monitorpath = "/var/spool/LMBd/";
	lmbctx->messages = 6;
	lmbctx->msgdisplay.resize(6);
	lmbctx->logpath = "/var/log/LMBd";
	lmbctx->maxlogfilesize = 1024*1024;
	std::vector<std::string> displayedmsgs;

	try {
		lmbctx->load(cfgpath.native());
	} catch (std::exception& e) {
		std::cerr << "Could Not Load Config File " << e.what() << std::endl;
		exit(-1);
	}

	LMB::Log::Manager::configure(lmbctx->configList);


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
		LMB_LOG_ERROR() << "Socket Error";
		return 1;
	}
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(CLITEST_PORT);
	if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0)
	{
		LMB_LOG_ERROR() << "bind Error";
		return 1;
	}

	if (listen(s, 50) < 0)
	{
		LMB_LOG_ERROR() << "listen Error";
		return 1;
	}

	LMB_LOG_INFO() << "Listening on port " << CLITEST_PORT;
	while ((x = accept(s, NULL, 0)))
	{
		if (lmbctx->Clients.size() < lmbctx->max_cli) {
			lmbctx->Clients.push_back(boost::make_shared<boost::thread>(Startcliloop, boost::ref(lmbctx), x));
		} else {
			LMB_LOG_WARN() << "Dropping Client Connection as we are at our Max";
			close(x);
		}
	}

	delete lmbctx;
	return 0;
}
