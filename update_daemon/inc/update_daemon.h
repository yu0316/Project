/*
 * update_daemon.h
 *
 *  Created on: 2017年4月5日
 *      Author: ubuntu
 */

#ifndef _UPDATE_DAEMON_H_
#define _UPDATE_DAEMON_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <termios.h>

#include "common.h"

#include <list>

using namespace std;

extern bool g_boIsExit;


#if HAS_CREOSS
#define INTERFACE_NAME	"eth0"
#else
#define INTERFACE_NAME	"wlp4s0"//"ens33"
#endif

#define UDP_SERVER_PORT	(('Y'<< 8) | 'A')


typedef struct _tagStNetInterfaceConfig
{
	bool boIsDHCP;
	char c8IPV4[IPV4_ADDR_LENGTH];
	char c8Mask[IPV4_ADDR_LENGTH];
	char c8Gateway[IPV4_ADDR_LENGTH];
	char c8DNS[IPV4_ADDR_LENGTH];
	char c8ReserveDNS[IPV4_ADDR_LENGTH];

	char c8MACAddr[MAC_ADDR_LENGTH];
}StNetInterfaceConfig;

typedef struct _tagStNetIfConfigInner
{
	StNetInterfaceConfig stConfig;
	int s32DHCPPid;
}StNetIfConfigInner;


enum
{
	_UDP_Cmd_GetEthInfo = _MCS_Cmd_UpdateDaemon,
	_UDP_Cmd_SetEthInfo,


	_TCP_Cmd_Updata = _MCS_Cmd_UpdateDaemon + 0x100,

};


void *ThreadUDP(void *pArg);

#endif /* UARTDAEMON_UART_DAEMON_H_ */