/* LED Message Board - Driver-DX.hpp
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

/** @file Driver-DX.hpp
 *  @brief
 */


#ifndef SRC_DRIVER_DX_HPP_
#define SRC_DRIVER_DX_HPP_

#include "Driver.hpp"

class Driver_DX : public iDriver {
public:
		Driver_DX() {};
		~Driver_DX() {};
		bool Init(LMBCTX *lmbctx);
		bool Fini();
		bool setMessage(int pos, std::string message);
		virtual bool StartUp();
private:
		LMBCTX *lmbctx;
};





#endif /* SRC_DRIVER_DX_HPP_ */
