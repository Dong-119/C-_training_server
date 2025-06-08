#include<iostream>
#include<WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#include <vector>
#include<stdio.h>
#include<string.h>

using namespace std;

void player_quit(SOCKET client_socket);

class client {
public:
	char name[1024] = { 0 };
	SOCKET socket = 0;
	bool ready = 0;//����Ƿ�׼������Ϊ1������Ϊ0
	bool win = 0, lose = 0;//�Ƿ�ʤ����ʧ��

	~client() {
		player_quit(socket);
	}
};

class game {
public:
	client* client_ingame[2] = { NULL };
	string roomnum;//�����

	//������ս��
	game(client* cli1, string rmnum) {
		client_ingame[0] = cli1;
		roomnum = rmnum;
	}
};

vector<game> games;

//�Ͽ����Ӳ���ս���ڴ��ڵ���һ��������Ϊplayer1�����û��ɾ��ս��
void player_quit(SOCKET client_socket) {
	cout << client_socket << " quit" << endl;
	char buffer[1024] = {0};
	//����ս��
	for (int games_indx = 0; games_indx < games.size(); games_indx++) {
		//����ս���ڿͻ���
		for (int i = 0; i < 2;i++) {
			//����ҵ��ÿͻ���
			if (games[games_indx].client_ingame[i]!=NULL && games[games_indx].client_ingame[i]->socket == client_socket) {
				//����ս���ڸÿͻ���Ϊ��
				games[games_indx].client_ingame[i] = NULL;
				//��ÿͻ��˶���
				closesocket(client_socket);
				//����ս���������
				//��ս����û�����
				if (games[games_indx].client_ingame[0] == NULL && games[games_indx].client_ingame[1] == NULL) {
					//ɾ��ս��
					games.erase(games.begin() + games_indx);
					break;
				}//��ս���������
				else {
					//��ǰ�����0��λ
					if (games[games_indx].client_ingame[0] != NULL) {
						//�����ҷ������2�˳���Ϣ
						send(games[games_indx].client_ingame[0]->socket, "player 2 quit", sizeof("player 2 quit"), 0);
						break;
					}//��ǰ�����1��λ
					else if (games[games_indx].client_ingame[1] != NULL) {
						//�����ҷ������1�˳���Ϣ
						send(games[games_indx].client_ingame[1]->socket, "player 1 quit", sizeof("player 1 quit"), 0);
						//�����������Ϊ0��λ
						games[games_indx].client_ingame[0] = games[games_indx].client_ingame[1];
						games[games_indx].client_ingame[1] = NULL;
						break;
					}
				}
			}
		}
	}
}

//�̺߳���
DWORD WINAPI thread_func(LPVOID lpThreadParameter) {
	printf("start!!!\n");
	//����socket
	SOCKET client_socket = *(SOCKET*)lpThreadParameter;
	free(lpThreadParameter);

	u_long mode = 0; // ����Ϊ����ģʽ ������Ϊ1������Ϊ0
	if (ioctlsocket(client_socket, FIONBIO, &mode) == SOCKET_ERROR) {
		std::cerr << "ioctlsocket failed: " << WSAGetLastError() << std::endl;
		closesocket(client_socket);
		WSACleanup();
		return -1;
	}

	//��ȡ�ͻ�������ǳ�������������뷿���
	client client_login;
	client_login.socket = client_socket;
	char buffer[1024] = { 0 };
	if (recv(client_socket, buffer, 1024, 0) < 0) {
		cerr << "no name received";
		closesocket(client_socket);
		return -1;
	}
	printf(buffer);
	strcpy(client_login.name, buffer);
roomnuminput:
	if (recv(client_socket, buffer, 1024, 0) < 0) {
		cerr << "no roomnumber received";
		closesocket(client_socket);
		return -1;
	}if (strcmp(buffer, "back") == 0) {
		player_quit(client_socket);
		return -1;
	}
	printf(buffer);

	string num = buffer;
	for (int games_indx = 0; games_indx < games.size(); games_indx++) {
		//���÷������
		if (num == games[games_indx].roomnum) {
			//������1�ˣ�����
			if (games[games_indx].client_ingame[1] == NULL) {
				cout << "���뷿�� " << games[games_indx].roomnum << endl;
				send(client_socket, "ready", sizeof("ready"), 0);
				games[games_indx].client_ingame[1] = &client_login;
				send(games[games_indx].client_ingame[1]->socket, games[games_indx].client_ingame[0]->name, sizeof(games[games_indx].client_ingame[0]->name), 0);
				send(games[games_indx].client_ingame[0]->socket, games[games_indx].client_ingame[1]->name, sizeof(games[games_indx].client_ingame[1]->name), 0);
				goto ready;
			}//���������������뷿���
			else{
			    cout << "��������" << endl;
				send(client_socket, "at full", sizeof("at full"), 0);
				MessageBox(NULL, L"��������,���������������", L"�޷�����", MB_OK | MB_ICONINFORMATION);
				goto roomnuminput;
			}
		}
	}
	//���䲻����
	cout << "�����Ѵ���" << endl;
	send(client_socket, "create", sizeof("create"), 0);
	games.emplace_back(&client_login, num);

ready:
	mode = 1; // ����Ϊ������ģʽ ������Ϊ1������Ϊ0
	if (ioctlsocket(client_socket, FIONBIO, &mode) == SOCKET_ERROR) {
		std::cerr << "ioctlsocket failed: " << WSAGetLastError() << std::endl;
		closesocket(client_socket);
		WSACleanup();
		return -1;
	}
	while (1) {
		//û���ݷ���-1���Ͽ����ӷ���0�������ݷ������ݴ�С
		if (recv(client_socket, buffer, 1024, 0) > 0) {
			if (strcmp(buffer, "back") == 0) {
				player_quit(client_socket);
				return 0;
			}
			else if(strcmp(buffer, "ready") == 0) {
				client_login.ready=!client_login.ready;
			}
			else if (strcmp(buffer, "win") == 0) {
				client_login.win = 1;
			}
			else if (strcmp(buffer, "give up") == 0) {
				client_login.lose = 1;
			}
		}
	}
	return 0;
}

int main(){
	//windows�������繦��
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	/**///����socket�׽��� Э���ַ��IPV4 TCPЭ�� ������ʽ������
	SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == listen_socket) {
		printf("create lisn_socket failed!!! errcode: %d\n", GetLastError());
		return -1;
	}

	// ���ü����׽���Ϊ������ģʽ
	u_long mode = 1; // ������ģʽ
	if (ioctlsocket(listen_socket, FIONBIO, &mode) == SOCKET_ERROR) {
		std::cerr << "ioctlsocket failed: " << WSAGetLastError() << std::endl;
		closesocket(listen_socket);
		WSACleanup();
		return -1;
	}

	//Ϊsocket�󶨶˿ں�
	struct sockaddr_in local = { 0 };
	local.sin_family = AF_INET;                       //Э���ַ��
	local.sin_port = htons(8080);                     //�˿ں�
	local.sin_addr.s_addr = inet_addr("0.0.0.0");     //IP��ַ ȫ���ַ��ȫ������

	if (-1 == bind(listen_socket, (struct sockaddr*)&local, sizeof(local))) {
		printf("bind socket failed!!! errcode: %d\n", GetLastError());
		return -1;
	}

	//Ϊsocket������������
	if (-1 == listen(listen_socket, 10)) {
		printf("start listen failed!!! errcode: %d\n", GetLastError());
		return -1;
	}

	char buffer[1024] = { 0 };
	//�ȴ��ͻ�������
	while(1){
		SOCKET client_socket = accept(listen_socket, NULL, NULL);  //�ȴ����Ӳ����ؿͻ���socket��������ִ��
		if (INVALID_SOCKET != client_socket){
		    SOCKET* sockfd = (SOCKET*)malloc(sizeof(SOCKET));
		    *sockfd = client_socket;
		    
		    //�����߳�
		    CreateThread(NULL, 0, thread_func, sockfd, 0, NULL);
        }
		//����ս��
		for (auto game : games) {
			//˫����׼�������Ϳ�ʼ��Ϸ��Ϣ
			if (game.client_ingame[0] != NULL && game.client_ingame[1] != NULL && game.client_ingame[0]->ready && game.client_ingame[1]->ready) {
				send(game.client_ingame[0]->socket, "start", sizeof("start"), 0);
				send(game.client_ingame[1]->socket, "start", sizeof("start"), 0);
				game.client_ingame[0]->ready = 0;
				game.client_ingame[1]->ready = 0;
			}
			//��������������һ�����ʤ��,��˫�����Ͷ�Ӧ��Ϣ
			if (game.client_ingame[0] != NULL && game.client_ingame[1] != NULL && (game.client_ingame[0]->lose || game.client_ingame[1]->win)) {
				send(game.client_ingame[0]->socket, "lose", sizeof("lose"), 0);
				game.client_ingame[0]->lose = 0;
				send(game.client_ingame[1]->socket, "win", sizeof("win"), 0);
				game.client_ingame[1]->win = 0;

			}//���������ʤ����һ���������,��˫�����Ͷ�Ӧ��Ϣ
			else if (game.client_ingame[0] != NULL && game.client_ingame[1] != NULL && (game.client_ingame[0]->win || game.client_ingame[1]->lose)) {
				send(game.client_ingame[1]->socket, "lose", sizeof("lose"), 0);
				game.client_ingame[0]->win = 0;
				send(game.client_ingame[0]->socket, "win", sizeof("win"), 0);
				game.client_ingame[1]->lose = 0;
			}
		}
	}
	return 0;
}