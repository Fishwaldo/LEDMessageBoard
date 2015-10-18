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

// vim:sw=4 tw=120 et


#define MODE_CONFIG_INT             10

#ifdef __GNUC__
# define UNUSED(d) d __attribute__ ((unused))
#else
# define UNUSED(d) d
#endif

unsigned int regular_count = 0;
unsigned int debug_regular = 0;


#ifdef WIN32
typedef int socklen_t;

int winsock_init()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	// Start up sockets
	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		// Tell the user that we could not find a usable WinSock DLL.
		return 0;
	}

	/*
	 * Confirm that the WinSock DLL supports 2.2
	 * Note that if the DLL supports versions greater than 2.2 in addition to
	 * 2.2, it will still return 2.2 in wVersion since that is the version we
	 * requested.
	 * */
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		// Tell the user that we could not find a usable WinSock DLL.
		WSACleanup();
		return 0;
	}
	return 1;
}
#endif

int cmd_save(struct cli_def *cli, const char *command, char *argv[], int argc)
{
	struct LMBCTX *lmbctx = (struct LMBCTX *)cli_get_context(cli);
	boost::unique_lock<boost::mutex> scoped_lock(lmbctx->io_mutex);
	lmbctx->save("/etc/LMBd.conf");
	cli_print(cli, "Saved Config");
	BOOST_LOG_TRIVIAL(info) << "Saved Config From Terminal";
	return CLI_OK;
}

int cmd_show_ports(struct cli_def *cli, const char *command, char *argv[], int argc)
{
	int i;
	cli_print(cli, "called %s with \"%s\"", __func__, command);
	cli_print(cli, "%d arguments:", argc);
	for (i = 0; i < argc; i++)
		cli_print(cli, "        %s", argv[i]);

	std::vector<serial::PortInfo> pi;
	pi = serial::list_ports();
	std::vector<serial::PortInfo>::iterator pii;
	for (pii = pi.begin(); pii != pi.end(); pii++) {
		cli_print(cli, "Port: %s Description: %s", pii->port.c_str(), pii->description.c_str());
	}

	return CLI_OK;
}

int cmd_show_port(struct cli_def *cli, const char *command, char *argv[], int argc)
{

	struct LMBCTX *lmbctx = (struct LMBCTX *)cli_get_context(cli);
	boost::unique_lock<boost::mutex> scoped_lock(lmbctx->io_mutex);

	cli_print(cli, "Port: %s", lmbctx->port.c_str());
	return CLI_OK;
}

int cmd_show_messages(struct cli_def *cli, const char *command, char *argv[], int argc)
{

	struct LMBCTX *lmbctx = (struct LMBCTX *)cli_get_context(cli);
	boost::unique_lock<boost::mutex> scoped_lock(lmbctx->io_mutex);
	for (unsigned int i = 1; i <= lmbctx->messages; i++) {
		if (lmbctx->msgdisplay[i-1] == 1) {
			cli_print(cli, "Message %d: %s", i, lmbctx->displayedmsgs[i].c_str());
		}
	}
	return CLI_OK;
}

int cmd_show_startupmsgs(struct cli_def *cli, const char *command, char *argv[], int argc)
{

	struct LMBCTX *lmbctx = (struct LMBCTX *)cli_get_context(cli);
	boost::unique_lock<boost::mutex> scoped_lock(lmbctx->io_mutex);
	for (unsigned int i = 1; i <= lmbctx->messages; i++) {
		if (lmbctx->startupmsg[i].length() > 0) {
			cli_print(cli, "Startup Message %d: %s", i, lmbctx->startupmsg[i].c_str());
		}
	}
	return CLI_OK;
}


int cmd_set(struct cli_def *cli, const char *command, char *argv[], int argc)
{
	return CLI_OK;
}

int cmd_set_port(struct cli_def *cli, UNUSED(const char *command), char *argv[],
		int argc)
{
	if (strcmp(argv[0], "?") == 0)
	{
		cli_print(cli, "Specify a value");
		return CLI_OK;
	}

	if (!argv[0] && !&argv[0])
	{
		cli_print(cli, "Specify a serial port to open");
		return CLI_OK;
	}
	std::vector<serial::PortInfo> pi;
	pi = serial::list_ports();
	std::vector<serial::PortInfo>::iterator pii;
	for (pii = pi.begin(); pii != pi.end(); pii++) {
		if (!pii->port.compare(argv[0])) {
			cli_print(cli, "Setting Serial Port to %s", pii->port.c_str());
			struct LMBCTX *lmbctx = (struct LMBCTX *)cli_get_context(cli);
			boost::unique_lock<boost::mutex> scoped_lock(lmbctx->io_mutex);

			lmbctx->port = pii->port;
			BOOST_LOG_TRIVIAL(info) << "Set a New Serial Port from Terminal: " << lmbctx->port;
			return CLI_OK;
		}
	}
	cli_print(cli, "No Serial Port Matching %s was found", argv[1]);
	return CLI_OK;
}

int cmd_set_startupmsg(struct cli_def *cli, UNUSED(const char *command), char *argv[],
		int argc)
{
	if (argc < 2) {
		cli_print(cli, "usage: set startupmsg <pos> <message>");
		return CLI_OK;
	}
	struct LMBCTX *lmbctx = (struct LMBCTX *)cli_get_context(cli);
	boost::unique_lock<boost::mutex> scoped_lock(lmbctx->io_mutex);

	if (atoi(argv[0]) > lmbctx->messages) {
		cli_print(cli, "Position out of Range");
		return CLI_OK;
	}
	lmbctx->startupmsg[atoi(argv[0])] = std::string(argv[1]);

	return CLI_OK;
}



int cmd_message(struct cli_def *cli, UNUSED(const char *command), UNUSED(char *argv[]), UNUSED(int argc)) {
	if (argc < 2) {
		cli_print(cli, "usage: message <pos> <message>");
		return CLI_OK;
	}
	struct LMBCTX *lmbctx = (struct LMBCTX *)cli_get_context(cli);
	boost::unique_lock<boost::mutex> scoped_lock(lmbctx->io_mutex);

	lmbctx->driver->Init(lmbctx);
	lmbctx->driver->setMessage(atoi(argv[0]), argv[1]);
	lmbctx->driver->Fini();
	return CLI_OK;
}

int cmd_clear(struct cli_def *cli, UNUSED(const char *command), UNUSED(char *argv[]), UNUSED(int argc)) {
	if (argc < 1) {
		cli_print(cli, "usage: clear <pos>");
		return CLI_OK;
	}
	struct LMBCTX *lmbctx = (struct LMBCTX *)cli_get_context(cli);
	boost::unique_lock<boost::mutex> scoped_lock(lmbctx->io_mutex);

	lmbctx->driver->Init(lmbctx);
	lmbctx->driver->setMessage(atoi(argv[0]), "");
	lmbctx->driver->Fini();
	return CLI_OK;
}


int cmd_context(struct cli_def *cli, UNUSED(const char *command), UNUSED(char *argv[]), UNUSED(int argc))
{
	//	struct my_context *myctx = (struct my_context *)cli_get_context(cli);
	//cli_print(cli, "User context has a value of %d and message saying %s", myctx->value, myctx->message);
	return CLI_OK;
}

int check_auth(const char *username, const char *password)
{
	return CLI_OK;
#if 0
	if (strcasecmp(username, "fred") != 0)
		return CLI_ERROR;
	if (strcasecmp(password, "nerk") != 0)
		return CLI_ERROR;
	return CLI_OK;
#endif
}



int idle_timeout(struct cli_def *cli)
{
	cli_print(cli, "Custom idle timeout");
	return CLI_QUIT;
}





int Startcliloop(LMBCTX *lmbctx, int sockfd) {
	struct cli_command *c;
	struct cli_def *cli;


	cli = cli_init();

	cli_set_banner(cli, "Led Message Board");
	cli_set_hostname(cli, "LMB");
	cli_telnet_protocol(cli, 1);
	{
		boost::unique_lock<boost::mutex> scoped_lock(lmbctx->io_mutex);
		cli_allow_user(cli, lmbctx->username.c_str(), lmbctx->password.c_str());
		cli_allow_enable(cli, lmbctx->enablepass.c_str());
	}
	cli_set_idle_timeout_callback(cli, 60, idle_timeout); // 60 second idle timeout
	c = cli_register_command(cli, NULL, "set", cmd_set, PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);

	cli_register_command(cli, c, "port", cmd_set_port, PRIVILEGE_PRIVILEGED, MODE_EXEC, "Set Serial Port");
	cli_register_command(cli, c, "startupmsg", cmd_set_startupmsg, PRIVILEGE_PRIVILEGED, MODE_EXEC, "Set Serial Port");

	cli_register_command(cli, NULL, "save", cmd_save, PRIVILEGE_PRIVILEGED, MODE_EXEC, "Save Config");

	c = cli_register_command(cli, NULL, "show", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);

	cli_register_command(cli, c, "ports", cmd_show_ports, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Show available Serial Ports");
	cli_register_command(cli, c, "port", cmd_show_port, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Show Configured Serial Port");
	cli_register_command(cli, c, "messages", cmd_show_messages, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Show Current Messages");
	cli_register_command(cli, c, "startupmsg", cmd_show_startupmsgs, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Show Current Messages");

	cli_register_command(cli, NULL, "message", cmd_message, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Set a Message to be displayed");
	cli_register_command(cli, NULL, "clear", cmd_clear, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Clear a Message");


	cli_set_context(cli, (void*)lmbctx);
	BOOST_LOG_TRIVIAL(info) << "accepted new CLI Connection";
	cli_loop(cli, sockfd);
	BOOST_LOG_TRIVIAL(info) << "closed CLI Connection";
	cli_done(cli);
	return 0;
}




