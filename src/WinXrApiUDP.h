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

#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <Winsock2.h>
#include <iostream>
#include <thread>
#include <cstring>
#include <io.h>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <atomic>
#include <vector>

// One pair of spaces the host located
struct WxrHostPose {
	int space = 0;
	int baseSpace = 0;
	float pose[7] = {};     // position xyz, orientation xyzw; an all-zero orientation means not located
	float velocity[7] = {}; // XrSpaceVelocityFlags, linear xyz, angular xyz
};

// The host's tracking packet, read once when it arrives instead of on every xrWaitFrame
struct WxrHostState {
	float floats[12] = {}; // sticks LX LY RX RY, L trigger, L grip, R trigger, R grip, IPD, FOV h, FOV v, refresh rate
	int frameId = 0;
	int recenterId = 0;
	bool buttons[19] = {};
	std::vector<WxrHostPose> poses;
};

class WinXrApiUDP
{
public:
	WinXrApiUDP();
	void Init();
	void ReceiveData();
	void KillReceiver();
	void SendData(std::string sendData);
	void StorePacket(const char* buffer, int bytesReceived);
	void ReceiveOnce(int timeoutMs);

	bool GetState(WxrHostState& out);
	bool HasSocket() const { return udpSocket != (int)INVALID_SOCKET; }
	~WinXrApiUDP();

	int LastOpenXRFrameID = -1;

private:
	int udpPort = 7872;
	int udpSocket;
	int udpSendPort = 7278;
	int udpSendSocket = (int)INVALID_SOCKET;
	std::thread udpReadThread;
	WxrHostState state;
	bool hasState = false;
	WxrHostState scratch;
	std::mutex parseMtx;
	std::atomic<bool> readerStarted{ false };
	std::mutex mtx;
	std::condition_variable cv;
};

