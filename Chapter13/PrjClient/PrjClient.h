#pragma once

#ifndef PRJCLIENT_H
#define PRJCLIENT_H

#include "resource.h"

#include <CommCtrl.h>
#include <commdlg.h>

#include <WinSock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <fstream>

#include "ConvTxt.h"

#define ENABLE_SOCKET
#include "CMyChatWnd.h"

using namespace std;

using namespace CMyChatWnd_Library;
using namespace CMyChatMsgMan_Library;
using namespace ConvTxt_Library;

#endif		// PRJCLIENT_H
