#include "Arduino-Minecraft-Monitor.h"

ArduinoMinecraftMonitor::ArduinoMinecraftMonitor(IPAddress serverIP, unsigned int queryPort) {
  minecraftIP = serverIP;
  minecraftPort = queryPort;
  udpPacket.begin(queryPort);
}

void ArduinoMinecraftMonitor::debugEnabled(bool setdebug) {
  debugenabled = setdebug;
  if (setdebug) {
    debug("ArduinoMinecraftMonitor - Debug");
  }
}

void ArduinoMinecraftMonitor::resetTimeout() {
  previousMillis = millis();
  errorState = false;
}

bool ArduinoMinecraftMonitor::waitTimeout() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= timeout) {
    previousMillis = millis();
    debug("Error: Connection Timeout");
    errorState = true;
    return false;
  } else {
    errorState = false;
    return true;
  }
}

void ArduinoMinecraftMonitor::debug(String text) {
  if (debugenabled) {
    Serial.print("[Debug] ");
    Serial.println(text);
  }
}

void ArduinoMinecraftMonitor::dumpPacket(char* packet, int packetSize) {
  if (debugenabled) {
    Serial.print("[Debug] Packet Dump: ");
    for (int i = 0; i < packetSize; i++) {
      Serial.print(packet[i], HEX);
      Serial.print(" ");
    }
    Serial.println("");
  }
}

ArduinoMinecraftMonitor& ArduinoMinecraftMonitor::setClient(Client& client){
    this->_client = &client;
    return *this;
}

bool ArduinoMinecraftMonitor::getStats() {
  debug("Getting Minecraft Status...");
  String token = runHandshake();
  if (errorState) {
    debug("Error in Handshake - return false");
    return false;
  }
  getServerStats(token);
  if (errorState) {
    debug("Error in Getting Stats - return false");
    return false;
  }
  debug("Finished Getting Status.");
  return true;
}

String ArduinoMinecraftMonitor::runHandshake() {
  debug("Executing Minecraft Handshake...");
  debug("Sending Handshake Packet - 0xFE, 0xFD, 0x09, 0x04, 0x05, 0x06, 0x07");
  udpPacket.beginPacket(minecraftIP, minecraftPort);
  char handshare[] = {0xFE, 0xFD, 0x09, 0x04, 0x05, 0x06, 0x07 };
  udpPacket.write(handshare, 7);
  udpPacket.endPacket();
  char* receiveData;
  String returnData;
  int packetSize = 0;
  debug("Packet Sent, Waiting for reply...");
  resetTimeout();
  while (waitTimeout()) {
    packetSize = udpPacket.parsePacket();
    if (packetSize) {
      debug("Server Replied!");
      receiveData = new char[packetSize];
      udpPacket.read(receiveData, packetSize);
      if (receiveData[0] != 0x09) {
        dumpPacket(receiveData, packetSize);
        debug("Handshake - Packet Error");
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
    debug("Handshake - Timeout Error");
    return "";
  }
  dumpPacket(receiveData, packetSize);
  delete receiveData;
  return returnData;
}

void ArduinoMinecraftMonitor::getServerStats(String token) {
  debug("Retrieving Server Status...");
  udpPacket.beginPacket(minecraftIP, minecraftPort);
  char handshare[] = {0xFE, 0xFD, 0x00, 0x04, 0x05, 0x06, 0x07 };
  debug("Sending Handshake Packet - 0xFE, 0xFD, 0x09, 0x04, 0x05, 0x06, 0x07");
  udpPacket.write(handshare, 7);
  const int32_t number = strtol(token.c_str(), NULL, 10);
  unsigned char bytes[4];
  bytes[0] = (number >> 24) & 0xFF;
  bytes[1] = (number >> 16) & 0xFF;
  bytes[2] = (number >> 8) & 0xFF;
  bytes[3] = number & 0xFF;
  debug("Plus Token " + token);
  udpPacket.write(bytes, 4);
  char padding[] = { 0x00, 0x00, 0x00, 0x00};
  debug("Plus Padding - 0x00, 0x00, 0x00, 0x00");
  udpPacket.write(padding, 4);
  udpPacket.endPacket();
  debug("Packet Sent, Waiting for reply...");
  char* receiveData;
  int packetSize = 0;
  resetTimeout();
  while (waitTimeout()) {
    packetSize = udpPacket.parsePacket();
    if (packetSize) {
      debug("Server Replied!");
      receiveData = new char[packetSize];
      udpPacket.read(receiveData, packetSize);
      if (receiveData[0] != 0x00) {
        debug("Handshake - Packet Error");
        dumpPacket(receiveData, packetSize);
        delete receiveData;
        errorState = true;
        return;
      }
      break;
    }
  }
  if (errorState) {
    debug("GetStats - Timeout Error");
    return;
  }
  dumpPacket(receiveData, packetSize);
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
  debug("Interpret Status Packet...");
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