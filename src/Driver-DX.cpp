/* LED Message Board - Driver-DX.cpp
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

/** @file Driver-DX.cpp
 *  @brief
 */
#include <iostream>
#include "Driver-DX.hpp"


void doChkSum(uint8_t *data, uint8_t length) {
	uint8_t csum = 0;
	int i;
	for (i = 1; i < length; i++) {
		csum += data[i];
	}
	LMB_LOG_DEBUG() << "csum " << std::hex << (int)csum << " old " << std::hex << (int)data[i];
	data[i] = csum;
}
void insertMessage(uint8_t part, uint8_t *data, std::string message) {
	uint8_t startpos = 4;
	uint8_t size = 60;
	uint8_t messagestart = 0;
	LMB_LOG_DEBUG() << "message length: " << std::dec << message.length();
	if (part == 1) {
		startpos = 8;
		data[7] = message.length();
	} else if (part == 2) {
		messagestart = 60;
		size = 65;
	} else if (part == 3) {
		messagestart = 124;
		size = 65;
	} else if (part == 4) {
		messagestart = 188;
		size = 65;
	}

	if ((message.length() - messagestart) < size)
		size = (message.length() - messagestart);

	LMB_LOG_DEBUG() <<  "size " << (int)message.length() << std::dec << " StartPos: " << (int)startpos << " size " << (int)size << " messagestart " << (int)messagestart ;


	if (size > message.length())
		return;
	if (messagestart > message.length())
		return;
	for (int i = 0; i < size; i++) {
		data[startpos++] = message.at(i + messagestart);
	}

}

void printMessage(uint8_t *data, size_t length) {
	std::stringstream ss;
	LMB_LOG_DEBUG() <<  "Raw Message: ";
	for (unsigned int i = 0; i < length; ++i) {
		ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int)data[i] << " ";
		if ((i+1) % 16 == 0) {
			LMB_LOG_DEBUG() <<  ss.str();
			ss.str("");
		}
	}
	if (ss.gcount() > 0)
		LMB_LOG_DEBUG() <<  ss.str();
}

void setMessageNumber(uint8_t *data, uint8_t pos, bool firstmsg) {
	data[2] = (uint8_t)5+pos;
}



bool Driver_DX::Init(LMBCTX *lmbctx) {
	this->lmbctx = lmbctx;

	if (this->lmbctx->sp == NULL && this->lmbctx->port.length() != 0) {
		this->lmbctx->sp = new serial::Serial(this->lmbctx->port, 38400, serial::Timeout::simpleTimeout(10));
	} else if (this->lmbctx->sp == NULL){
		return false;
	}

	if (!this->lmbctx->sp->isOpen()) {
		this->lmbctx->sp->open();
	}
	this->lmbctx->sp->setFlowcontrol(serial::flowcontrol_software);
	this->lmbctx->sp->setRTS(true);
	this->lmbctx->sp->setDTR(true);
	/* construct the Message Header */
	LMB_LOG_DEBUG() << "Port Open: " << this->lmbctx->sp->isOpen();
	std::vector<uint8_t> data;
	uint8_t start[] = { 0x54 };
	this->lmbctx->sp->write(start, 1);
	this->lmbctx->sp->flush();
	std::string query = this->lmbctx->sp->read(1);
	if (query.length() > 0) {
		LMB_LOG_DEBUG() << "Reply to Inquiry: " << std::hex << query.at(0);
	} else {
		LMB_LOG_ERROR() << "No Reply to Inquiry";
		return false;
	}
	LMB_LOG_DEBUG() << "Send Out Start Sequence";

	this->lmbctx->sp->flush();
	start[0] = 0x41;
	this->lmbctx->sp->write(start, 1);
	start[0] = 0x42;
	this->lmbctx->sp->write(start, 1);
	start[0] = 0x43;
	this->lmbctx->sp->write(start, 1);
	start[0] = 0x44;
	this->lmbctx->sp->write(start, 1);
	start[0] = 0x45;
	this->lmbctx->sp->write(start, 1);
	this->lmbctx->sp->flush();
	usleep(150 * 1000);
	return true;

}


#include <boost/algorithm/string/replace.hpp>

bool Driver_DX::setMessage(int pos, std::string message) {

	if (this->lmbctx->messages < (unsigned int)pos) {
		LMB_LOG_WARN() << "Invalid Message Position " << pos;
		return false;
	}

	if (message.length() > 250) {
		LMB_LOG_WARN() << "Message is " << std::dec << message.length() << " chars long. Trimming to 250 Chars";
		message = message.substr(0, 250);
	}

	if (message.length() > 0) {


		this->lmbctx->displayedmsgs[pos] = message;
		this->lmbctx->msgdisplay[pos-1] = 1;

		//boost::replace_all(message, "~+", "\xc0\x00");
		boost::replace_all(message, "~H", "\xc0\x02");
		boost::replace_all(message, "~<", "\xc0\x04");
		boost::replace_all(message, "~>", "\xc0\x06");
		boost::replace_all(message, "~P", "\xc0\x08");
		boost::replace_all(message, "~x", "\xc0\x0A");
		boost::replace_all(message, "~S", "\xc0\x0C");
		boost::replace_all(message, "~O", "\xc0\x0E");
		boost::replace_all(message, "~o", "\xc0\x10");
		boost::replace_all(message, "~M", "\xc0\x12");
		boost::replace_all(message, "~Q", "\xc0\x14");
		boost::replace_all(message, "~n", "\xc0\x16");



		uint8_t header1[] =  { 0x02, 0x31, 0x06, 0x00, 0x35, 0x00, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 0x72};
		insertMessage(1, &header1[0], message);
		setMessageNumber(&header1[0], pos, true);
		doChkSum(&header1[0], 68);
		printMessage(header1, 70);

		uint8_t header2[] = { 0x02, 0x31, 0x06, 0x40, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 0x71};
		insertMessage(2, &header2[0], message);
		setMessageNumber(&header2[0], pos, false);
		doChkSum(&header2[0], 68);
		printMessage(header2, 69);

		uint8_t header3[] = { 0x02, 0x31, 0x06, 0x80, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 0xB7};
		insertMessage(3, &header3[0], message);
		setMessageNumber(&header3[0], pos, false);
		doChkSum(&header3[0], 68);
		printMessage(header3, 69);

		uint8_t header4[] = { 0x02, 0x31, 0x06, 0xC0, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 0xF7};
		insertMessage(4, &header4[0], message);
		setMessageNumber(&header4[0], pos, false);
		doChkSum(&header4[0], 68);
		printMessage(header4, 69);






		uint8_t *finalpck = (uint8_t*)malloc(281);
		finalpck[0] = 0x00;
		for (int i = 0; i < 69; i++)
			finalpck[1+i] = header1[i];
		for (int i = 0; i < 69; i++)
			finalpck[1+69+i] = header2[i];
		for (int i = 0; i < 69; i++)
			finalpck[1+69+69+i] = header3[i];
		for (int i = 0; i < 69; i++)
			finalpck[1+69+69+69+i] = header4[i];
		printMessage(finalpck, 277);
		LMB_LOG_DEBUG() << "sending packets: ";
		int sent = (int)this->lmbctx->sp->write(header1, 69);
		LMB_LOG_DEBUG() << "size: " << std::dec << sent;
		usleep(100 * 1000);
		sent = (int)this->lmbctx->sp->write(header2, 69);
		LMB_LOG_DEBUG() << "size: " << std::dec << sent;
		usleep(100 * 1000);
		sent = (int)this->lmbctx->sp->write(header3, 69);
		LMB_LOG_DEBUG() << "size: " << std::dec << sent;
		usleep(100 * 1000);
		sent =(int)this->lmbctx->sp->write(header4, 69);
		LMB_LOG_DEBUG() << "size: " << std::dec << sent;
		usleep(100 * 1000);
	} else {
		LMB_LOG_DEBUG() << "Clearing Message " << pos;
		this->lmbctx->displayedmsgs[pos] = "";
		this->lmbctx->msgdisplay[pos-1] = 0;
	}
	uint8_t header5[] = { 0x02, 0x33, (uint8_t)this->lmbctx->msgdisplay.to_ulong() };
	int sent = (int)this->lmbctx->sp->write(header5, 3);
	LMB_LOG_DEBUG() << "size: " << std::dec << sent;
	LMB_LOG_INFO() << "Set Message " << pos << " to " << this->lmbctx->displayedmsgs[pos];

	return true;
}

bool Driver_DX::Fini() {
	usleep(150 * 1000);
	lmbctx->sp->close();
	return true;
}

#include <boost/lexical_cast.hpp>

class uint_from_hex   // For use with boost::lexical_cast
{
		unsigned int value;
	public:
		operator unsigned int() const { return value; }
		friend std::istream& operator>>( std::istream& in, uint_from_hex& outValue )
		{
			in >> std::hex >> outValue.value;
		}
};

#define STOH(x) boost::lexical_cast<uint_from_hex>(x)

uint8_t InsertString(unsigned int value) {
	std::stringstream ss;
	ss << "0x" << std::setfill('0') << std::setw(2) << value;
	return (STOH(ss.str()) & 0xFF);
}


bool Driver_DX::StartUp() {
	std::stringstream ss;
	time_t t = time(NULL);
	tm* timePtr = localtime(&t);
/*	uint8_t setTime[10] = { 0x02, 0x34, InsertString(timePtr->tm_year -100), InsertString(timePtr->tm_mon +1), InsertString(timePtr->tm_mday), InsertString(timePtr->tm_hour), InsertString(timePtr->tm_min), InsertString(timePtr->tm_sec), InsertString(timePtr->tm_wday), 0x00 };
#	doChkSum(&setTime[0], 9);
#	int sent = (int)this->lmbctx->sp->write(setTime, 10);
	LMB_LOG_DEBUG() << "Set Clock: " << std::dec << sent;
*/
	return true;
}
