/* LED Message Board - FileMonitor.cpp
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

/** @file FileMonitor.cpp
 *  @brief
 */
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>
#include "FileMonitor.hpp"
#include "Driver.hpp"

Inotify inotify(IN_CLOSE_WRITE);


void MonitorFiles(LMBCTX *lmbctx) {
	while(true) {
		FileSystemEvent event = inotify.getNextEvent();
		LMB_LOG_DEBUG()  << "Event wd(" << event.wd << ") " << event.getMaskString() << " for " << event.path << " was triggered!";
		boost::filesystem::ifstream ifs;
		ifs.open(event.path, std::ios::in);
		std::stringstream ss;
		ss << ifs.rdbuf();
		std::string message(ss.str());
		message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
//		int messageid = boost::lexical_cast<int>(event.path.generic_string().at(event.path.generic_string().length()-1));
		int messageid = event.wd;
		LMB_LOG_INFO() << "Message in Slot " << messageid << " was " << message;
		{
			boost::unique_lock<boost::mutex> scoped_lock(lmbctx->io_mutex);
			lmbctx->driver->Init(lmbctx);
			lmbctx->driver->setMessage(messageid, message);
			lmbctx->driver->Fini();
		}
	}
}



bool FileMonitor::init(LMBCTX *lmbctx) {

	boost::unique_lock<boost::mutex> scoped_lock(lmbctx->io_mutex);
	boost::filesystem::path monpath(lmbctx->monitorpath);
	if(fs::exists(monpath)) {
		if(fs::is_directory(monpath)) {
			for (fs::directory_iterator end_dir_it, it(monpath); it!=end_dir_it; ++it) {
				fs::remove_all(it->path());
			}

			for (unsigned int i = 1; i <= lmbctx->messages; i++) {
				char filename[255];
				snprintf(filename, 255, "message-%d", i);
				boost::filesystem::path messagefile = monpath;
				messagefile /= filename;
				LMB_LOG_INFO() << "Creating Message File: " << messagefile;
				std::ofstream f{messagefile.c_str(), std::ios::app};
				f.close();
				inotify.watchFile(messagefile);
			}


		} else {
			LMB_LOG_ERROR() << "Monitor Path is not a directory";
			return false;
		}
	} else {
		LMB_LOG_ERROR() << "Monitor Path does not exist";
		return false;
	}

	/* Now Start up our Monitoring Thread */
	boost::thread(MonitorFiles, boost::ref(lmbctx));
	return true;
}
