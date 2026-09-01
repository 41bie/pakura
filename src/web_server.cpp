#include "web_server.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>

#include "secrets.h"

namespace {

WebServer server(80);
bool accessPointEnabled = false;
bool filesystemReady = false;

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
