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
	string roomnum;//房间号
	int ready = 0;//战局是否准备（战局内存在两名玩家），是为1，不是为0
	int state = 0;//战局是否开始，开始为1，未开始为0

	//创建新战局
	game(client cli1, string rmnum) {
		client_ingame[0] = &cli1;
		roomnum = rmnum;
	}
};

vector<game> games;

void player_quit(SOCKET client_socket) {
	char buffer[1024];
	//遍历战局
	for (int games_indx = 0; games_indx < games.size(); games_indx++) {
		//遍历战局内客户端
		for (auto client : games[games_indx].client_ingame) {
			//如果找到该客户端
			if (client->socket == client_socket) {
				//设置战局内该客户端为空
				client == NULL;
				//与该客户端断联
				closesocket(client_socket);
				//检查该战局内玩家数
				//若战局内没有玩家
				if (games[games_indx].client_ingame[0] == NULL && games[games_indx].client_ingame[1] == NULL) {
					//删除战局
					games.erase(games.begin() + games_indx);
				}//若战局内有玩家
				else {
					//当前玩家在0号位
					if (games[games_indx].client_ingame[0] != NULL) {
						//向该玩家发送玩家2退出消息
						send(games[games_indx].client_ingame[0]->socket, "player 2 quit", sizeof("player 2 quit"), 0);
						//设置战局状态为未准备
						games[games_indx].ready = 0;
					}//当前玩家在1号位
					else if (games[games_indx].client_ingame[1] != NULL) {
						//向该玩家发送玩家1退出消息
						send(games[games_indx].client_ingame[1]->socket, "player 1 quit", sizeof("player 1 quit"), 0);
						//将该玩家设置为0号位
						games[games_indx].client_ingame[0] = games[games_indx].client_ingame[1];
						games[games_indx].client_ingame[1] = NULL;
						//设置战局状态为未准备
						games[games_indx].ready = 0;
					}
				}
			}
		}
	}
}

//线程函数
DWORD WINAPI thread_func(LPVOID lpThreadParameter) {
	printf("start!!!\n");
	//设置socket
	SOCKET client_socket = *(SOCKET*)lpThreadParameter;
	free(lpThreadParameter);

	//获取客户端玩家昵称与所创建或加入房间号
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
		//若该房间存在
		if (num == games[games_indx].roomnum) {
			//房间有1人，加入
			if (games[games_indx].client_ingame[1] == NULL) {
				cout << "加入房间 " << games[games_indx].roomnum << endl;
				games[games_indx].client_ingame[1] = &client_login;
				//房间加入两人，可以开始游戏,向两人发送ready信息
				send(games[games_indx].client_ingame[0]->socket, "ready", sizeof("ready"), 0);
				send(games[games_indx].client_ingame[1]->socket, "ready", sizeof("ready"), 0);
				games[games_indx].ready = 1;
				goto ready;
			}//房间已满重新输入房间号
			else{
			    cout << "房间已满" << endl;
				send(client_socket, "at full", sizeof("at full"), 0);
				MessageBox(NULL, L"房间已满,请输入其他房间号", L"无法加入", MB_OK | MB_ICONINFORMATION);
				goto roomnuminput;
			}
		}
	}
	//房间不存在
	cout << "房间已创建" << endl;
	send(client_socket, "create", sizeof("create"), 0);
	games.emplace_back(client_login, num);

ready:
	//关闭连接
	//closesocket(client_socket);

	return 0;
}

int main(){
	//windows启用网络功能
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	/**///创建socket套接字 协议地址簇IPV4 TCP协议 保护方式0
	SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == listen_socket) {
		printf("create lisn_socket failed!!! errcode: %d\n", GetLastError());
		return -1;
	}

	//为socket绑定端口号
	struct sockaddr_in local = { 0 };
	local.sin_family = AF_INET;                       //协议地址簇
	local.sin_port = htons(8080);                     //端口号
	local.sin_addr.s_addr = inet_addr("0.0.0.0");     //IP地址 全零地址，全部接受

	if (-1 == bind(listen_socket, (struct sockaddr*)&local, sizeof(local))) {
		printf("bind socket failed!!! errcode: %d\n", GetLastError());
		return -1;
	}

	//为socket开启监听属性
	if (-1 == listen(listen_socket, 10)) {
		printf("start listen failed!!! errcode: %d\n", GetLastError());
		return -1;
	}

	//等待客户端连接
	while(1){
		SOCKET client_socket = accept(listen_socket, NULL, NULL);  //等待连接并返回客户端socket，在完成前不继续执行
		if (INVALID_SOCKET == client_socket)continue;

		SOCKET* sockfd = (SOCKET*)malloc(sizeof(SOCKET));
		*sockfd = client_socket;

		//创建线程
		CreateThread(NULL, 0, thread_func, sockfd, 0, NULL);
	}
	return 0;
}