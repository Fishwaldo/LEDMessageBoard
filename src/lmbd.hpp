/* LED Message Board - lmbd.hpp
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

/** @file lmbd.hpp
 *  @brief
 */



#ifndef SRC_LMBD_HPP_
#define SRC_LMBD_HPP_
#include <boost/make_shared.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/filesystem.hpp>
#include "serial/serial.h"
#include "libcli/libcli.h"
#include "LoggerCpp/LoggerCpp.h"

class iDriver;


namespace patch
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}



#define LMB_LOG_DEBUG() logger->debug()
#define LMB_LOG_INFO() logger->info()
#define LMB_LOG_WARN() logger->warning()
#define LMB_LOG_ERROR() logger->error()


extern LMB::Log::Logger *logger;


struct LMBCTX {
		LMB::Log::Config::Vector configList;
		serial::Serial *sp;
		boost::mutex io_mutex;
		std::string port;
		int debug_level;
		std::string username;
		std::string password;
		std::string enablepass;
		struct cli_def *cli;
		iDriver *driver;
		std::string monitorpath;
		unsigned int messages;
		boost::dynamic_bitset<> msgdisplay;
		std::map<int, std::string> displayedmsgs;
		std::map<int, std::string> startupmsg;
		std::vector<boost::shared_ptr<boost::thread> > Clients;
		unsigned int max_cli;
		bool consolelog;
		std::string logpath;
		size_t maxlogfilesize;
		void load(const std::string &filename)
		{
		    // Create empty property tree object
			boost::property_tree::ptree tree;

		    // Parse the XML into the property tree.
			boost::property_tree::read_json(filename, tree);

		    // Use the throwing version of get to find the debug filename.
		    // If the path cannot be resolved, an exception is thrown.
			port= tree.get<std::string>("serial.port", "/dev/ttyUSB0");

		    // Use the default-value version of get to find the debug level.
		    // Note that the default value is used to deduce the target type.
		    username = tree.get<std::string>("username", "admin");
		    password = tree.get<std::string>("password", "password");
		    enablepass = tree.get<std::string>("enablepass", "password");
		    boost::filesystem::path monpath(tree.get<std::string>("monitorpath", "/var/spool/LMBd"));
	    	if (boost::filesystem::exists(monpath) && boost::filesystem::is_directory(monpath)) {
			    monitorpath = tree.get<std::string>("monitorpath", "/var/spool/LMBd");
	    	} else {
	    		throw std::runtime_error("monitorpath does not exist, or isn't a directory");
	    	}
		    if (tree.get<bool>("ConsoleLogging", false) == true) {
		    	LMB::Log::Config::addOutput(configList, "OutputConsole");
		    	consolelog = true;
		    }

		    if (tree.get<std::string>("LogPath", "/var/log/LMBd/").length() > 0) {
		    	boost::filesystem::path dir(tree.get<std::string>("LogPath", "/var/log/LMBd/"));
		    	if (boost::filesystem::exists(dir) && boost::filesystem::is_directory(dir)) {
		    		logpath = tree.get<std::string>("LogPath", "/var/log/LMBd/");
		    		logpath += "/LMBd.Log";
		    	} else {
		    		throw std::runtime_error("LogPath does not exist, or isn't a directory");
		    	}
		    }

			LMB::Log::Config::addOutput(configList, "OutputFile");
			LMB::Log::Config::setOption(configList, "filename", logpath.c_str());
			std::string oldlogpath(logpath);
			oldlogpath += ".old";
			LMB::Log::Config::setOption(configList, "filename_old", oldlogpath.c_str());
			LMB::Log::Config::setOption(configList, "max_startup_size",  "0");
			maxlogfilesize = tree.get<size_t>("MaxLogFileSize", 1024*1024);

			LMB::Log::Config::setOption(configList, "max_size", patch::to_string(maxlogfilesize).c_str());

		    logger->setLevel(LMB::Log::Log::toLevel(tree.get<std::string>("ConsoleLogLevel", "Warning").c_str()));

		    for (unsigned int i = 1; i <= this->messages; i++) {
		    	std::stringstream ss;
		    	ss << "startupmessage." << std::dec << i;
		    	this->startupmsg[i] = tree.get<std::string>(ss.str(), "");
		    }



		}
		void save(const std::string &filename)
		{
		    // Create an empty property tree object.
			boost::property_tree::ptree tree;

		    // Put the simple values into the tree. The integer is automatically
		    // converted to a string. Note that the "debug" node is automatically
		    // created if it doesn't exist.
		    tree.put("serial.port", port);
		    tree.put("username", username);
		    tree.put("password", password);
		    tree.put("enablepass", enablepass);
		    tree.put("monitorpath", monitorpath);
		    tree.put("ConsoleLogging", consolelog);
		    tree.put("ConsoleLogLevel", LMB::Log::Log::toString(logger->getLevel()));
		    tree.put("LogPath", logpath);
		    tree.put("MaxLogFileSize", maxlogfilesize);
		    for (unsigned int i = 1; i <= this->messages; i++) {
		    	std::stringstream ss;
		    	ss << "startupmessage." << std::dec << i;
		    	tree.put(ss.str(), this->startupmsg[i]);
		    }



		    // Write property tree to XML file
		    boost::property_tree::write_json(filename, tree);
		}
};



int Startcliloop(LMBCTX *lmbctx, int sockfd);






#endif /* SRC_LMBD_HPP_ */
