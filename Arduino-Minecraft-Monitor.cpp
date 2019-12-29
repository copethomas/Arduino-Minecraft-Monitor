#include "Arduino-Minecraft-Monitor.h"

ArduinoMinecraftMonitor::ArduinoMinecraftMonitor(IPAddress ip, uint16_t port) {
  this->minecraftServerIP = ip;
  this->minecraftQueryPort = port;
}

void ArduinoMinecraftMonitor::resetTimeout() {
  this->previousMillis = millis();
  this->errorState = false;
}

bool ArduinoMinecraftMonitor::waitTimeout() {
  unsigned long currentMillis = millis();
  if (currentMillis - this->previousMillis >= this->timeout) {
    this->previousMillis = millis();
    //Error: Connection Timeout
    this->errorState = true;
    return false;
  } else {
    this->errorState = false;
    return true;
  }
}

bool ArduinoMinecraftMonitor::getStats() {
  //Getting Minecraft Status...
  String token = runHandshake();
  if (errorState) {
    //Error in Handshake - return false
    return false;
  }
  getServerStats(token);
  if (errorState) {
    //Error in Getting Stats - return false
    return false;
  }
  //Finished Getting Status
  return true;
}

String ArduinoMinecraftMonitor::runHandshake() {
  //Executing Minecraft Handshake...
  //Sending Handshake Packet - 0xFE, 0xFD, 0x09, 0x04, 0x05, 0x06, 0x07
  udpPacket.begin(minecraftQueryPort);
  udpPacket.beginPacket(minecraftServerIP, minecraftQueryPort);
  char handshare[] = {0xFE, 0xFD, 0x09, 0x04, 0x05, 0x06, 0x07 };
  udpPacket.write(handshare, 7);
  udpPacket.endPacket();
  char* receiveData;
  String returnData;
  int packetSize = 0;
  //Packet Sent, Waiting for reply...
  resetTimeout();
  while (waitTimeout()) {
    packetSize = udpPacket.parsePacket();
    if (packetSize) {
      //Server Replied
      receiveData = new char[packetSize];
      udpPacket.read(receiveData, packetSize);
      if (receiveData[0] != 0x09) {
        //Handshake - Packet Error
        delete receiveData;
        errorState = true;
        return "";
      }
      for (int i = 5; i < packetSize; i++ ) {
        if (receiveData[i] == '\0') {
          break;
        } else {
          returnData += receiveData[i];
        }
      }
      break;
    }
  }
  if (errorState) {
    //Handshake - Timeout Error
    return "";
  }
  delete receiveData;
  return returnData;
}

void ArduinoMinecraftMonitor::getServerStats(String token) {
  //Retrieving Server Status...
  udpPacket.beginPacket(minecraftServerIP, minecraftQueryPort);
  char handshare[] = {0xFE, 0xFD, 0x00, 0x04, 0x05, 0x06, 0x07 };
  //Sending Handshake Packet - 0xFE, 0xFD, 0x09, 0x04, 0x05, 0x06, 0x07
  udpPacket.write(handshare, 7);
  const int32_t number = strtol(token.c_str(), NULL, 10);
  unsigned char bytes[4];
  bytes[0] = (number >> 24) & 0xFF;
  bytes[1] = (number >> 16) & 0xFF;
  bytes[2] = (number >> 8) & 0xFF;
  bytes[3] = number & 0xFF;
  //Plus Token
  udpPacket.write(bytes, 4);
  char padding[] = { 0x00, 0x00, 0x00, 0x00};
  //Plus Padding - 0x00, 0x00, 0x00, 0x00
  udpPacket.write(padding, 4);
  udpPacket.endPacket();
  //Packet Sent, Waiting for reply...
  char* receiveData;
  int packetSize = 0;
  resetTimeout();
  while (waitTimeout()) {
    packetSize = udpPacket.parsePacket();
    if (packetSize) {
      //Server Replied!
      receiveData = new char[packetSize];
      udpPacket.read(receiveData, packetSize);
      if (receiveData[0] != 0x00) {
        //Handshake - Packet Error
        delete receiveData;
        errorState = true;
        return;
      }
      break;
    }
  }
  if (errorState) {
    //GetStats - Timeout Error
    return;
  }
  interpretStatusPacket(receiveData, packetSize);
}

int ArduinoMinecraftMonitor::readUntilNull(String* data, char* receiveData, int counter, int increment) {
  *data = "";
  counter += increment;
  while (receiveData[counter] != '\0') {
    *data += receiveData[counter];
    counter++;
  }
  return counter;
}

void ArduinoMinecraftMonitor::interpretStatusPacket(char* receiveData, int packetSize) {
  //Interpret Status Packet...
  int i = 0;
  i = readUntilNull(&motd, receiveData, i, 25);
  i = readUntilNull(&gametype, receiveData, i, 10);
  i = readUntilNull(&gameID, receiveData, i, 9);
  i = readUntilNull(&mcversion, receiveData, i, 9);
  i = readUntilNull(&plugins, receiveData, i, 9);
  i = readUntilNull(&mcmap, receiveData, i, 5);
  String tmpStr = "";
  i = readUntilNull(&tmpStr, receiveData, i, 12);
  onlinePlayers = strtol(tmpStr.c_str(), NULL, 10);
  i = readUntilNull(&tmpStr, receiveData, i, 12);
  maxPlayers = strtol(tmpStr.c_str(), NULL, 10);
  i = readUntilNull(&tmpStr, receiveData, i, 10);
  hostport = strtol(tmpStr.c_str(), NULL, 10);
  i = readUntilNull(&hostIP, receiveData, i, 8);
  players = "";
  i += 12;
  while (i < (packetSize - 1)) {
    if (receiveData[i] == '\0') {
      players += ",";
    } else {
      players += receiveData[i];
    }
    i++;
  }
  players.remove(players.length() - 1);
  delete receiveData;
}

String ArduinoMinecraftMonitor::getMOTD() {
  return motd;
}
String ArduinoMinecraftMonitor::getGameType() {
  return gametype;
}
String ArduinoMinecraftMonitor::getGameID() {
  return gameID;
}
String ArduinoMinecraftMonitor::getMCVersion() {
  return mcversion;
}
String ArduinoMinecraftMonitor::getPlugins() {
  return plugins;
}
String ArduinoMinecraftMonitor::getMCMap() {
  return mcmap;
}
int ArduinoMinecraftMonitor::getOnlinePlayers() {
  return onlinePlayers;
}
int ArduinoMinecraftMonitor::getMaxPlayers() {
  return maxPlayers;
}
int ArduinoMinecraftMonitor::getHostPort() {
  return hostport;
}
String ArduinoMinecraftMonitor::getHostIP() {
  return hostIP;
}
String ArduinoMinecraftMonitor::getPlayers() {
  return players;
}