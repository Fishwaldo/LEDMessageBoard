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
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/dynamic_bitset.hpp>
#include "serial/serial.h"
#include "libcli/libcli.h"

class iDriver;


struct LMBCTX {
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
		void load(const std::string &filename)
		{
		    // Create empty property tree object
			boost::property_tree::ptree tree;

		    // Parse the XML into the property tree.
			boost::property_tree::read_json(filename, tree);

		    // Use the throwing version of get to find the debug filename.
		    // If the path cannot be resolved, an exception is thrown.
		    port= tree.get<std::string>("serial.port");

		    // Use the default-value version of get to find the debug level.
		    // Note that the default value is used to deduce the target type.
		    debug_level = tree.get("debug.level", 0);
		    username = tree.get<std::string>("username");
		    password = tree.get<std::string>("password");
		    enablepass = tree.get<std::string>("enablepass");
		    monitorpath = tree.get<std::string>("monitorpath");
		    for (unsigned int i = 1; i <= this->messages; i++) {
		    	std::stringstream ss;
		    	ss << "startupmessage." << std::dec << i;
		    	this->startupmsg[i] = tree.get<std::string>(ss.str());
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
		    tree.put("debug.level", debug_level);
		    tree.put("username", username);
		    tree.put("password", password);
		    tree.put("enablepass", enablepass);
		    tree.put("monitorpath", monitorpath);

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
