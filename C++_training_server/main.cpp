#include<iostream>
#include<WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#include <vector>
#include<stdio.h>
#include<string.h>

using namespace std;

class client {
public:
	string name;
	SOCKET socket;
};

class game {
public:
	client* client_ingame[2] = { NULL };
	string roomnum;//�����
	int ready = 0;//ս���Ƿ�׼����ս���ڴ���������ң�����Ϊ1������Ϊ0
	int state = 0;//ս���Ƿ�ʼ����ʼΪ1��δ��ʼΪ0

	//������ս��
	game(client cli1, string rmnum) {
		client_ingame[0] = &cli1;
		roomnum = rmnum;
	}
};

vector<game> games;

void player_quit(SOCKET client_socket) {
	char buffer[1024];
	//����ս��
	for (int games_indx = 0; games_indx < games.size(); games_indx++) {
		//����ս���ڿͻ���
		for (auto client : games[games_indx].client_ingame) {
			//����ҵ��ÿͻ���
			if (client->socket == client_socket) {
				//����ս���ڸÿͻ���Ϊ��
				client == NULL;
				//��ÿͻ��˶���
				closesocket(client_socket);
				//����ս���������
				//��ս����û�����
				if (games[games_indx].client_ingame[0] == NULL && games[games_indx].client_ingame[1] == NULL) {
					//ɾ��ս��
					games.erase(games.begin() + games_indx);
				}//��ս���������
				else {
					//��ǰ�����0��λ
					if (games[games_indx].client_ingame[0] != NULL) {
						//�����ҷ������2�˳���Ϣ
						send(games[games_indx].client_ingame[0]->socket, "player 2 quit", sizeof("player 2 quit"), 0);
						//����ս��״̬Ϊδ׼��
						games[games_indx].ready = 0;
					}//��ǰ�����1��λ
					else if (games[games_indx].client_ingame[1] != NULL) {
						//�����ҷ������1�˳���Ϣ
						send(games[games_indx].client_ingame[1]->socket, "player 1 quit", sizeof("player 1 quit"), 0);
						//�����������Ϊ0��λ
						games[games_indx].client_ingame[0] = games[games_indx].client_ingame[1];
						games[games_indx].client_ingame[1] = NULL;
						//����ս��״̬Ϊδ׼��
						games[games_indx].ready = 0;
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

	//��ȡ�ͻ�������ǳ�������������뷿���
	client client_login;
	client_login.socket = client_socket;
	char buffer[1024];
	if (recv(client_socket, buffer, 1024, 0) < 0) {
		cerr << "no name received";
		closesocket(client_socket);
		return -1;
	}
	printf(buffer);
	client_login.name = buffer;
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
				games[games_indx].client_ingame[1] = &client_login;
				//����������ˣ����Կ�ʼ��Ϸ,�����˷���ready��Ϣ
				send(games[games_indx].client_ingame[0]->socket, "ready", sizeof("ready"), 0);
				send(games[games_indx].client_ingame[1]->socket, "ready", sizeof("ready"), 0);
				games[games_indx].ready = 1;
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
	games.emplace_back(client_login, num);

ready:
	//�ر�����
	//closesocket(client_socket);

	return 0;
}

int main(){
	//windows�������繦��
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	/**///����socket�׽��� Э���ַ��IPV4 TCPЭ�� ������ʽ0
	SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == listen_socket) {
		printf("create lisn_socket failed!!! errcode: %d\n", GetLastError());
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

	//�ȴ��ͻ�������
	while(1){
		SOCKET client_socket = accept(listen_socket, NULL, NULL);  //�ȴ����Ӳ����ؿͻ���socket�������ǰ������ִ��
		if (INVALID_SOCKET == client_socket)continue;

		SOCKET* sockfd = (SOCKET*)malloc(sizeof(SOCKET));
		*sockfd = client_socket;

		//�����߳�
		CreateThread(NULL, 0, thread_func, sockfd, 0, NULL);
	}
	return 0;
}