#ifndef ArduinoMinecraftMonitor_H
#define ArduinoMinecraftMonitor_H

#include <Arduino.h>
#include "IPAddress.h"
#include "Client.h"
#include "Stream.h"

class ArduinoMinecraftMonitor {
 
  public:
    ArduinoMinecraftMonitor(IPAddress ip, uint16_t port);
    bool getStats();
    void debugEnabled(bool setdebug);

    //Minecraft Data
    String getMOTD();
    String getGameType();
    String getGameID();
    String getMCVersion();
    String getPlugins();
    String getMCMap();
    int getOnlinePlayers();
    int getMaxPlayers();
    int getHostPort();
    String getHostIP();
    String getPlayers();

  private:
    //Internal Functions
    bool waitTimeout();
    void resetTimeout();
    void debug(String text);
    String runHandshake();
    void getServerStats(String token);
    void receiveUDP(byte type);
    void interpretStatusPacket(char* receiveData, int packetSize);
    int readUntilNull(String* data, char* receiveData, int counter, int increment);
    void dumpPacket(char* packet, int packetSize);
    //Internal Varibles
    bool debugenabled = false;
    EthernetUDP udpPacket;
    IPAddress minecraftServerIP; //Address of Minecraft Server
    uint16_t minecraftQueryPort;
    const long timeout = 5000;
	unsigned long previousMillis = 0;
	bool errorState = false;
    //Minecraft Data
    String motd = "<unknown>";
    String gametype = "<unknown>";
    String gameID = "<unknown>";
    String mcversion = "<unknown>";
    String plugins = "<unknown>";
    String mcmap = "<unknown>";
    int onlinePlayers = -1;
    int maxPlayers = -1;
    int hostport = -1;
    String hostIP = "<unknown>";
    String players = "<unknown>";
};

#endif

