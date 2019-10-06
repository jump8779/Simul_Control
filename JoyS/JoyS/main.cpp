#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>

#define HAVE_STRUCT_TIMESPEC // for timespec error

#include<pthread.h>

#define BUF_SIZE 1024
#define SERVER_PORT 1024

#pragma comment(lib, "winmm.lib")  //joystick 
#pragma comment(lib, "Ws2_32.lib") // for WinSock2.h error

#pragma warning(disable:4996)

typedef struct Joy // Joystick position x,y,z
{
	int x;
	int y;
	int z;
}JOY;

struct socketStruct
{
	JOY joy;
};

bool Flag = false;
socketStruct Sendstruct;
SOCKET Client_listen;

// 조이스틱 연결 및 좌표값 불러오기
void* Joy(void* data) 
{
	for(;;)
	{
		const int MID_VALUE = (int)(USHRT_MAX / 2);

		JOYCAPS jc;
		JOYINFOEX lastJoyState;
		// JOYSTICKID1 0
		// JOYSTICKID2 1

		if (joyGetNumDevs() == 0) 
		{ // 연결된 장치 수
			printf("Please connect a joystick\n");
			Flag = false;
			while (joyGetNumDevs() == 0);
			system("cls");
		}
		// 조이스틱 호환성 검사
		if (joyGetDevCaps(JOYSTICKID1, &jc, sizeof(jc)) != JOYERR_NOERROR) 
		{
			printf("Please connect a compatible joystick\n");
			Flag = false;
			while (joyGetDevCaps(JOYSTICKID1, &jc, sizeof(jc)) != JOYERR_NOERROR);
			system("cls");
		}
		// 사용할 정보 범위 설정
		lastJoyState.dwSize = sizeof(lastJoyState);
		lastJoyState.dwFlags = JOY_RETURNALL | JOY_RETURNPOVCTS | JOY_RETURNCENTERED | JOY_USEDEADZONE;
		// joyGetPosEx : 조이스틱의 상태를 얻어오는 함수
		// return값 [JOYERR_NOERROR | JOYERR_PARMS | JOYERR_NOCANDO | JOYERR_UNPLUGGED]
		if (joyGetPosEx(JOYSTICKID1, &lastJoyState) != JOYERR_NOERROR)
		{
			printf("Please reconnect the joystick\n");
			Flag = false;
			while (joyGetPosEx(JOYSTICKID1, &lastJoyState) != JOYERR_NOERROR);
			system("cls");
		}

		Flag = true;
		Sendstruct.joy.x = (int)(lastJoyState.dwXpos);
		Sendstruct.joy.y = (int)(lastJoyState.dwYpos);
		Sendstruct.joy.z = (int)(lastJoyState.dwZpos);
	}
}
void* do_client(void* data)
{
	WSADATA wsaData;
	sockaddr_in local_addr;

	int len_addr;

	WSAStartup(MAKEWORD(2, 2), &wsaData);
	Client_listen = socket(PF_INET, SOCK_STREAM, 0); // TCP 소켓 생성

	if (Client_listen == INVALID_SOCKET)
	{
		printf("socket creat error.\n");
		exit(1);
	}

	memset(&local_addr, 0, sizeof(local_addr)); // local_addr 를 0으로 초기화
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	local_addr.sin_port = htons(SERVER_PORT);

	if (connect(Client_listen, (SOCKADDR*)& local_addr, sizeof(local_addr)) == SOCKET_ERROR)
	{
		printf("Socket Connection Error.\n");
		exit(1);
	}

	printf("Server connecting...\n");

	for(;;)
	{
		while (Flag == true) 
		{
			send(Client_listen, (char*)& Sendstruct, sizeof(Sendstruct), 0);

			printf("%d\t\t%d\t\t%d\n", Sendstruct.joy.x, Sendstruct.joy.y, Sendstruct.joy.z);
			Sleep(100);
		}
	}
}

void main() 
{
	int thr_id;
	pthread_t p_thread[2];
	int a = 1;

	//while (1)
	{
		thr_id = pthread_create(&p_thread[0], NULL, do_client, (void*)& a);
		thr_id = pthread_create(&p_thread[1], NULL, Joy, (void*)& a);

		pthread_join(p_thread[0], NULL);
		pthread_join(p_thread[1], NULL);
	}

	closesocket(Client_listen);
	WSACleanup();
}