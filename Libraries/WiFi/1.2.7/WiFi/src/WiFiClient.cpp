/*
  WiFiClient.cpp - Library for Arduino Wifi shield.
  Copyright (c) 2011-2014 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

extern "C" {
  #include "utility/wl_definitions.h"
  #include "utility/wl_types.h"
  #include "string.h"
  #include "utility/debug.h"
}

#include "WiFi.h"
#include "WiFiClient.h"
#include "WiFiServer.h"
#include "utility/server_drv.h"


uint16_t WiFiClient::_srcport = 1024;

WiFiClient::WiFiClient() : _sock(MAX_SOCK_NUM) {
}

WiFiClient::WiFiClient(uint8_t sock) : _sock(sock) {
}

int WiFiClient::connect(const char* host, uint16_t port) {
	IPAddress remote_addr;
	if (WiFi.hostByName(host, remote_addr))
	{
		return connect(remote_addr, port);
	}
	return 0;
}

int WiFiClient::connect(IPAddress ip, uint16_t port) {
    _sock = getFirstSocket();
    if (_sock != NO_SOCKET_AVAIL)
    {
    	ServerDrv::startClient(uint32_t(ip), port, _sock);
    	WiFiClass::_state[_sock] = _sock;

    	unsigned long start = millis();

    	// wait 4 second for the connection to close
    	while (!connected() && millis() - start < 10000)
    		delay(1);

    	if (!connected())
       	{
    		return 0;
    	}
    }else{
    	// Serial.println("No Socket available");
    	return 0;
    }
    return 1;
}

size_t WiFiClient::write(uint8_t b) {
	  return write(&b, 1);
}

size_t WiFiClient::write(const uint8_t *buf, size_t size) {
  if (_sock >= MAX_SOCK_NUM)
  {
	  setWriteError();
	  return 0;
  }
  if (size==0)
  {
	  setWriteError();
      return 0;
  }


  if (!ServerDrv::sendData(_sock, buf, size))
  {
	  setWriteError();
      return 0;
  }
  if (!ServerDrv::checkDataSent(_sock))
  {
	  setWriteError();
      return 0;
  }

  return size;
}

int WiFiClient::available() {
  if (_sock != 255)
  {
      return ServerDrv::availData(_sock);
  }
   
  return 0;
}

int WiFiClient::read() {
  uint8_t b;
  if (!available())
    return -1;

  ServerDrv::getData(_sock, &b);
  return b;
}


int WiFiClient::read(uint8_t* buf, size_t size) {
  // sizeof(size_t) is architecture dependent
  // but we need a 16 bit data type here
  uint16_t _size = size;
  if (!ServerDrv::getDataBuf(_sock, buf, &_size))
      return -1;
  return 0;
}

int WiFiClient::peek() {
	  uint8_t b;
	  if (!available())
	    return -1;

	  ServerDrv::getData(_sock, &b, 1);
	  return b;
}

void WiFiClient::flush() {
  // TODO: a real check to ensure transmission has been completed
}

void WiFiClient::stop() {

  if (_sock == 255)
    return;

  ServerDrv::stopClient(_sock);
  WiFiClass::_state[_sock] = NA_STATE;

  int count = 0;
  // wait maximum 5 secs for the connection to close
  while (status() != CLOSED && ++count < 50)
    delay(100);

  _sock = 255;
}

uint8_t WiFiClient::connected() {

  if (_sock == 255) {
    return 0;
  } else {
    uint8_t s = status();

    return !(s == LISTEN || s == CLOSED || s == FIN_WAIT_1 ||
    		s == FIN_WAIT_2 || s == TIME_WAIT ||
    		s == SYN_SENT || s== SYN_RCVD ||
    		(s == CLOSE_WAIT));
  }
}

uint8_t WiFiClient::status() {
    if (_sock == 255) {
    return CLOSED;
  } else {
    return ServerDrv::getClientState(_sock);
  }
}

WiFiClient::operator bool() {
  return _sock != 255;
}

// Private Methods
uint8_t WiFiClient::getFirstSocket()
{
    for (int i = 0; i < MAX_SOCK_NUM; i++) {
      if (WiFiClass::_state[i] == NA_STATE)
      {
          return i;
      }
    }
    return SOCK_NOT_AVAIL;
}

