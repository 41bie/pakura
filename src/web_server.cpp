#include "web_server.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <SD.h>
#include <WebServer.h>
#include <WiFi.h>

#include "secrets.h"

namespace {

WebServer server(80);
bool accessPointEnabled = false;
bool filesystemReady = false;
const char* ssidFilePath = "/data/ssid.csv";

struct NetworkRecord {
	int id;
	String ssid;
	String bssid;
	String rssi;
	String channel;
	String security;
};

void serveFile(const char* path, const char* contentType) {
	File file = LittleFS.open(path, FILE_READ);

	if (!file) {
		server.send(404, "text/plain", "Not found");
		return;
	}

	server.streamFile(file, contentType);
	file.close();
}

void handleRoot() {
	serveFile("/web/index.html", "text/html");
}

void handleStylesheet() {
	serveFile("/web/style.css", "text/css");
}

void handleScript() {
	serveFile("/web/script.js", "application/javascript");
}

bool parseNetworkRecord(const String& line, NetworkRecord& record) {
	int idEnd = line.indexOf(',');
	if (idEnd < 0) {
		return false;
	}

	record.id = line.substring(0, idEnd).toInt();
	int securitySeparator = line.lastIndexOf(',');
	int channelSeparator = securitySeparator < 0
		? -1
		: line.lastIndexOf(',', securitySeparator - 1);
	int rssiSeparator = channelSeparator < 0
		? -1
		: line.lastIndexOf(',', channelSeparator - 1);
	int bssidSeparator = rssiSeparator < 0
		? -1
		: line.lastIndexOf(',', rssiSeparator - 1);

	if (bssidSeparator <= idEnd) {
		record.ssid = line.substring(idEnd + 1);
		record.bssid = "";
		record.rssi = "";
		record.channel = "";
		record.security = "";
		return record.ssid.length() > 0;
	}

	record.ssid = line.substring(idEnd + 1, bssidSeparator);
	record.bssid = line.substring(bssidSeparator + 1, rssiSeparator);
	record.rssi = line.substring(rssiSeparator + 1, channelSeparator);
	record.channel = line.substring(channelSeparator + 1, securitySeparator);
	record.security = line.substring(securitySeparator + 1);
	return record.ssid.length() > 0;
}

String jsonEscape(const String& value) {
	String escaped;
	for (unsigned int index = 0; index < value.length(); index++) {
		char character = value[index];
		if (character == '\\' || character == '"') {
			escaped += '\\';
		}
		if (character == '\n' || character == '\r') {
			escaped += ' ';
		}
		else {
			escaped += character;
		}
	}
	return escaped;
}

void appendJsonString(String& output, const char* key, const String& value) {
	output += "\"";
	output += key;
	output += "\":\"";
	output += jsonEscape(value);
	output += "\"";
}

void appendNetworkJson(String& output, const NetworkRecord& record, bool includeSSID) {
	output += "{";
	output += "\"id\":";
	output += record.id;
	output += ",";
	if (includeSSID) {
		appendJsonString(output, "ssid", record.ssid);
	}
	else {
		appendJsonString(output, "prefix", record.ssid.substring(0, 3));
	}
	output += ",";
	appendJsonString(output, "bssid", record.bssid.length() > 0 ? record.bssid : "UNKNOWN");
	output += ",";
	appendJsonString(output, "rssi", record.rssi.length() > 0 ? record.rssi : "UNKNOWN");
	output += ",";
	appendJsonString(output, "channel", record.channel.length() > 0 ? record.channel : "UNKNOWN");
	output += ",";
	appendJsonString(output, "security", record.security.length() > 0 ? record.security : "UNKNOWN");
	output += "}";
}

bool countNetworkRecords(int& recordCount) {
	File file = SD.open(ssidFilePath, FILE_READ);
	if (!file) {
		recordCount = 0;
		return false;
	}

	recordCount = 0;
	while (file.available()) {
		String line = file.readStringUntil('\n');
		line.trim();
		NetworkRecord record;
		if (parseNetworkRecord(line, record)) {
			recordCount++;
		}
	}
	file.close();
	return true;
}

void handleNetworkPage() {
	int recordCount = 0;
	countNetworkRecords(recordCount);

	const int pageSize = 10;
	int page = server.hasArg("page") ? server.arg("page").toInt() : 0;
	if (page < 0) {
		page = 0;
	}
	int pageCount = (recordCount + pageSize - 1) / pageSize;
	if (pageCount > 0 && page >= pageCount) {
		page = pageCount - 1;
	}

	int firstRecord = recordCount - ((page + 1) * pageSize);
	if (firstRecord < 0) {
		firstRecord = 0;
	}
	int lastRecord = recordCount - (page * pageSize);
	if (lastRecord > recordCount) {
		lastRecord = recordCount;
	}

	String response = "{\"page\":";
	response += page;
	response += ",\"pageCount\":";
	response += pageCount;
	response += ",\"records\":[";
	bool first = true;
	File file = SD.open(ssidFilePath, FILE_READ);
	if (file) {
		NetworkRecord records[10];
		int pageRecordCount = 0;
		int recordIndex = 0;
		while (file.available() && recordIndex < lastRecord) {
			String line = file.readStringUntil('\n');
			line.trim();
			NetworkRecord record;
			if (!parseNetworkRecord(line, record)) {
				continue;
			}
			if (recordIndex >= firstRecord) {
				records[pageRecordCount++] = record;
			}
			recordIndex++;
		}
		file.close();
		for (int index = pageRecordCount - 1; index >= 0; index--) {
			if (!first) {
				response += ",";
			}
			appendNetworkJson(response, records[index], false);
			first = false;
		}
	}
	response += "]}";
	server.send(200, "application/json", response);
}

void handleNetworkDetail() {
	if (!server.hasArg("id")) {
		server.send(400, "application/json", "{\"error\":\"Missing id\"}");
		return;
	}

	int requestedId = server.arg("id").toInt();
	File file = SD.open(ssidFilePath, FILE_READ);
	if (file) {
		while (file.available()) {
			String line = file.readStringUntil('\n');
			line.trim();
			NetworkRecord record;
			if (parseNetworkRecord(line, record) && record.id == requestedId) {
				String response;
				appendNetworkJson(response, record, true);
				file.close();
				server.send(200, "application/json", response);
				return;
			}
		}
		file.close();
	}

	server.send(404, "application/json", "{\"error\":\"Record not found\"}");
}

void handleNotFound() {
	server.send(404, "text/plain", "Not found");
}

}

void beginWebServer() {
	if (!LittleFS.begin(true)) {
		Serial.println("LittleFS FAILED");
		return;
	}

	filesystemReady = true;

	server.on("/", HTTP_GET, handleRoot);
	server.on("/style.css", HTTP_GET, handleStylesheet);
	server.on("/script.js", HTTP_GET, handleScript);
	server.on("/api/ssids", HTTP_GET, handleNetworkPage);
	server.on("/api/ssid", HTTP_GET, handleNetworkDetail);
	server.onNotFound(handleNotFound);
	Serial.println("Web server ready");
}

bool enablePakuraAccessPoint() {
	if (!filesystemReady || accessPointEnabled) {
		return filesystemReady;
	}

	if (!WiFi.softAP("Pakura", PAKURA_AP_PASSWORD)) {
		Serial.println("Pakura AP FAILED");
		return false;
	}

	server.begin();
	accessPointEnabled = true;

	Serial.print("Pakura AP: ");
	Serial.println(WiFi.softAPSSID());
	Serial.print("Pakura web server: http://");
	Serial.println(WiFi.softAPIP());
	return true;
}

void disablePakuraAccessPoint() {
	if (!accessPointEnabled) {
		return;
	}

	server.stop();
	WiFi.softAPdisconnect(true);
	accessPointEnabled = false;
	Serial.println("Pakura AP disabled");
}

bool isPakuraAccessPointEnabled() {
	return accessPointEnabled;
}

void handleWebServer() {
	server.handleClient();
}
