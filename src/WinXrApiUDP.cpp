/*
 * Copyright (C) 2024-2026 WinlatorXR
 *
 * This file is part of WinlatorXR.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "WinXrApiUDP.h"
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <cstring>
#include <io.h>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <sstream>
#include <vector>
#include <charconv>
#include <cmath>
#include <cstdint>

WinXrApiUDP::WinXrApiUDP()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// Bound here rather than on the reader thread, so GetRetData can read it before that thread runs
	udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udpSocket != INVALID_SOCKET)
	{
		sockaddr_in serverAddr{};
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		serverAddr.sin_port = htons(udpPort);

		if (bind(udpSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
		{
			closesocket(udpSocket);
			udpSocket = INVALID_SOCKET;
		}
	}

	//Logger::log << "[WinXrUDP] Starting UDP receiver thread..." << std::endl;
	udpReadThread = std::thread(&WinXrApiUDP::ReceiveData, this);
	udpReadThread.detach();
}

void WinXrApiUDP::ReceiveData()
{
	readerStarted = true;
	if (udpSocket == INVALID_SOCKET)
		return;

	sockaddr_in clientAddr{};

	while (true)
	{
		try
		{
			char buffer[16384];
			int addrLen = sizeof(clientAddr);
			int bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
			if (bytesReceived <= 0)
			{
				// Socket closed by KillReceiver: stop instead of spinning
				int error = WSAGetLastError();
				if (error == WSAENOTSOCK || error == WSANOTINITIALISED)
					break;
				continue;
			}
			StorePacket(buffer, bytesReceived);
		}
		catch (const std::exception& e)
		{
			//Logger::log << "[WinXrUDP] Error receiving UDP data: " << e.what() << std::endl;
		}
	}
}

// A host that was asked for XrAPI 0.7 starts its tracking packet with these bytes and sends the
// values as little-endian floats and ints; anything else is the space-separated text of 0.6
static const char kBinaryMagic[4] = { 'W', 'X', 'R', 7 };
static const int kMaxPoses = 256;

namespace {

// Reads the text packet a token at a time. Like the stream it replaces, one bad token leaves
// every later value at zero; unlike it, it does not depend on the locale the game has set.
struct TextReader {
	const char* p;
	const char* end;
	bool ok = true;

	bool Token(const char*& b, const char*& e)
	{
		while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == '\0')) p++;
		if (!ok || p >= end) {
			ok = false;
			return false;
		}
		b = p;
		while (p < end && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '\0') p++;
		e = p;
		return true;
	}

	float Float()
	{
		const char *b, *e;
		float value = 0;
		if (!Token(b, e)) return 0;
		if (b < e && *b == '+') b++;
		if (std::from_chars(b, e, value).ec != std::errc() || !std::isfinite(value)) {
			ok = false;
			return 0;
		}
		return value;
	}

	int Int()
	{
		const char *b, *e;
		int value = 0;
		if (!Token(b, e)) return 0;
		if (b < e && *b == '+') b++;
		if (std::from_chars(b, e, value).ec != std::errc()) {
			ok = false;
			return 0;
		}
		return value;
	}
};

struct BinaryReader {
	const char* p;
	const char* end;

	template <typename T> T Read()
	{
		T value{};
		if (end - p >= (ptrdiff_t)sizeof(T)) {
			memcpy(&value, p, sizeof(T));
			p += sizeof(T);
		} else {
			p = end;
		}
		return value;
	}
};

void ParseText(const char* buffer, int length, WxrHostState& out)
{
	TextReader in{ buffer, buffer + length };
	const char *b, *e;
	in.Token(b, e); // client name

	for (float& f : out.floats) f = in.Float();
	out.frameId = in.Int();
	out.recenterId = in.Int();

	// One T or F per button; anything else in the token is skipped
	int button = 0;
	if (in.Token(b, e)) {
		for (; b < e && button < 19; b++) {
			if (*b == 'T' || *b == 'F') out.buttons[button++] = *b == 'T';
		}
	}

	int count = in.Int();
	if (count > kMaxPoses) count = kMaxPoses;
	for (int i = 0; i < count; i++) {
		WxrHostPose pose;
		pose.space = in.Int();
		pose.baseSpace = in.Int();
		for (float& f : pose.pose) f = in.Float();
		for (float& f : pose.velocity) f = in.Float();
		out.poses.push_back(pose);
	}
}

void ParseBinary(const char* buffer, int length, WxrHostState& out)
{
	BinaryReader in{ buffer + sizeof(kBinaryMagic), buffer + length };
	for (float& f : out.floats) f = in.Read<float>();
	out.frameId = in.Read<int32_t>();
	out.recenterId = in.Read<int32_t>();
	uint32_t buttonBits = in.Read<uint32_t>();
	for (int i = 0; i < 19; i++) out.buttons[i] = (buttonBits >> i) & 1;

	int count = in.Read<int32_t>();
	if (count > kMaxPoses) count = kMaxPoses;
	for (int i = 0; i < count; i++) {
		WxrHostPose pose;
		pose.space = in.Read<int32_t>();
		pose.baseSpace = in.Read<int32_t>();
		for (float& f : pose.pose) f = in.Read<float>();
		for (float& f : pose.velocity) f = in.Read<float>();
		out.poses.push_back(pose);
	}
}

} // namespace

void WinXrApiUDP::StorePacket(const char* buffer, int bytesReceived)
{
	// The reader thread and an inline read can both end up here; scratch keeps its pose buffer between packets
	std::lock_guard<std::mutex> parseLock(parseMtx);
	std::vector<WxrHostPose> poses = std::move(scratch.poses);
	poses.clear();
	scratch = WxrHostState{};
	scratch.poses = std::move(poses);

	if (bytesReceived >= (int)sizeof(kBinaryMagic) && memcmp(buffer, kBinaryMagic, sizeof(kBinaryMagic)) == 0)
		ParseBinary(buffer, bytesReceived, scratch);
	else
		ParseText(buffer, bytesReceived, scratch);

	if (LastOpenXRFrameID == scratch.frameId)
		return;

	{
		std::lock_guard<std::mutex> lock(mtx);
		std::swap(state, scratch);
		hasState = true;
	}

	cv.notify_all();
}

// Reads one packet on the calling thread, waiting up to timeoutMs for it
void WinXrApiUDP::ReceiveOnce(int timeoutMs)
{
	if (udpSocket == INVALID_SOCKET)
	{
		Sleep(timeoutMs);
		return;
	}

	fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET((SOCKET)udpSocket, &readSet);
	timeval timeout{ timeoutMs / 1000, (timeoutMs % 1000) * 1000 };
	if (select(0, &readSet, nullptr, nullptr, &timeout) <= 0)
		return;

	char buffer[16384];
	sockaddr_in clientAddr{};
	int addrLen = sizeof(clientAddr);
	int bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
	if (bytesReceived > 0)
		StorePacket(buffer, bytesReceived);
}

void WinXrApiUDP::SendData(std::string sendData)
{
	try
	{
		struct sockaddr_in targetAddress;
		if (udpSendSocket == INVALID_SOCKET)
			udpSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (udpSendSocket == INVALID_SOCKET) {
			//Logger::log << "[WinXrUDP] Error sending UDP data: socket creation failed" << std::endl;
			return;
		}

		targetAddress.sin_family = AF_INET;
		targetAddress.sin_port = htons(udpSendPort);
		inet_pton(AF_INET, "127.0.0.1", &targetAddress.sin_addr);

		int result = sendto(udpSendSocket, sendData.c_str(), sendData.length(), 0, (struct sockaddr*)&targetAddress, sizeof(targetAddress));
		if (result == SOCKET_ERROR) {
			//Logger::log << "[WinXrUDP] sendto failed with error " << WSAGetLastError() << std::endl;
		}

		//WSACleanup();
	}
	catch (const std::exception& e)
	{
		//Logger::log << "[WinXrUDP] Error sending UDP data: " << e.what() << std::endl;
	}
}

void WinXrApiUDP::KillReceiver()
{
	//Logger::log << "[WinXrUDP] Shutting down UDP receiver..." << std::endl;

	try
	{
		udpReadThread.~thread();
		udpReadThread = std::thread();
		closesocket(udpSocket);
		WSACleanup();
	}
	catch (const std::exception& e)
	{
		//Logger::log << "[WinXrUDP] Error killing UDP receiver: " << e.what() << std::endl;
	}
}

// Copies the latest host packet into out, waiting for the first one
bool WinXrApiUDP::GetState(WxrHostState& out) {
	std::unique_lock<std::mutex> lock(mtx);
	// Without the socket nothing can arrive (another process already holds the port), so this
	// returns false and the caller fails the call instead of the game hanging in it
	if (!hasState && !HasSocket())
		return false;
	// The host only streams after it has seen a state message, and the one sent at startup can arrive before it listens
	while (!hasState) {
		if (readerStarted) {
			if (cv.wait_for(lock, std::chrono::milliseconds(250), [this] { return hasState; }))
				break;
		} else {
			// Set up from inside a DllMain (Max Payne 2 VR), the loader lock keeps the reader thread
			// from starting until this call returns, so waiting for it would never end
			lock.unlock();
			ReceiveOnce(250);
			lock.lock();
			if (hasState)
				break;
		}
		lock.unlock();
		SendData("0 0 2 0");
		lock.lock();
	}
	out = state;
	return true;
}

WinXrApiUDP::~WinXrApiUDP()
{
	KillReceiver();
}
