#include "ChattingServer.h"
#include "Configuration.h"
#include <conio.h>
#include <time.h>

int main() {
	ChattingServer chatserver(
		CHATTING_SERVER_CONNECT_TO_REDIS,
		CHATTING_SERVER_IP, CHATTING_SERVER_PORT, CHATTING_SERVER_MAX_OF_CONNECTIONS,
		MONT_SERVER_PROTOCOL_CODE, CHATTING_SERVER_PROTOCOL_CODE, CHATTING_SERVER_PACKET_KEY,
		CHATTING_SERVER_RECV_BUFFERING_MODE,
		CHATTING_SERVER_MAX_OF_CONNECTIONS,
		CHATTING_SERVER_IOCP_CONCURRENT_THREAD, CHATTING_SERVER_IOCP_WORKER_THREAD,
		CHATTING_SERVER_TLS_MEM_POOL_UNIT_CNT, CHATTING_SERVER_TLS_MEM_POOL_UNIT_CAPACITY,
		CHATTING_SERVER_SERIAL_BUFFER_SIZE,
		CHATTING_SERVER_SESSION_RECV_BUFF_SIZE,
		CHATTING_SERVER_CALC_TPS_THREAD
	);

	if (!chatserver.Start()) {
		return 0;
	}

	char ctr;
	clock_t ct = 0;
	while (true) {
		if (_kbhit()) {
			ctr = _getch();
			if (ctr == 'q' || ctr == 'Q') {
				break;
			}
		}


		chatserver.PrintServerInfoOnConsole();
		Sleep(1000);
	}

	chatserver.Stop();
}