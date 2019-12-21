#include <fmt/core.h>
#include <cstdlib>
#include <ignore.h>
#include "manager.h"
#include "slotMap.h"

int main() {
	/*
	const char *key_file_key = "SSL_KEY";
	const char *key_file_name = getenv(key_file_key);
	const char *cert_file_key = "SSL_CERT";
	const char *cert_file_name = getenv(cert_file_key);
	if (!(cert_file_name && key_file_name)) {
		fmt::print("Couldn't find cert_file and key_file. Please ensure "
			 "environment variables, {} and {} are set.\n", cert_file_key, key_file_key);
		return 1;
	}
	*/

	SlotMap managers;

	// uWS::App({
		// .key_file_name = key_file_name,
		// .cert_file_name = cert_file_name
	// }).ws<UserData>("/joinGame/:game", {
	uWS::App().ws<UserData>("/joinGame/:game", {
		.open = [&managers](WebSocket *ws, uWS::HttpRequest *req) {
			auto *data = static_cast<UserData *>(ws->getUserData());
			data->socket = ws;
			SlotMap::Key key(std::stoi(std::string(req->getParameter(0))));
			auto manager = managers[key];
			if (!manager) {
				ws->end(100);
				return;
			}
			Manager &m = manager.value();
			data->playerId = m.addClient(ws);
			data->manager = &m;
		},
		.message = [](WebSocket *ws, std::string_view message, uWS::OpCode opCode) {
			if (opCode != uWS::OpCode::BINARY) {
				return;
			}

			auto *data = static_cast<UserData *>(ws->getUserData());
			data->manager->handleMessage(data->playerId, message);
		},
		.close = [](WebSocket *ws, int code, std::string_view message) {
			ignoreUnused(message);
			auto *data = static_cast<UserData *>(ws->getUserData());
			data->manager->onDisconnect(data->playerId, code);
		}
	}).ws<UserData>("/createGame", {
		.open = [&managers](WebSocket *ws, uWS::HttpRequest *req) {
			ignoreUnused(req);
			auto *data = static_cast<UserData *>(ws->getUserData());
			data->socket = ws;
			SlotMap::Key key = managers.getSlot();
			auto manager = managers[key];
			if (!manager) {
				ws->end(100);
				return;
			}
			Manager &m = manager.value();
			data->playerId = m.addClient(ws);
			data->manager = &m;
		},
		.message = [](WebSocket *ws, std::string_view message, uWS::OpCode opCode) {
			if (opCode != uWS::OpCode::BINARY) {
				return;
			}

			auto *data = static_cast<UserData *>(ws->getUserData());
			data->manager->handleMessage(data->playerId, message);
		},
		.close = [](WebSocket *ws, int code, std::string_view message) {
			ignoreUnused(message);
			auto *data = static_cast<UserData *>(ws->getUserData());
			data->manager->onDisconnect(data->playerId, code);
		}
	}).listen(4545, [](auto *listenSocket) {
        if (listenSocket) {
            std::cout << "Listening for connections..." << std::endl;
        }
    }).run();
	return 0;
}
