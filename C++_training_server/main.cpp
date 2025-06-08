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
	bool ready = 0;//玩家是否准备，是为1，不是为0
	bool win = 0, lose = 0;//是否胜利或失败

	~client() {
		player_quit(socket);
	}
};

class game {
public:
	client* client_ingame[2] = { NULL };
	string roomnum;//房间号

	//创建新战局
	game(client* cli1, string rmnum) {
		client_ingame[0] = cli1;
		roomnum = rmnum;
	}
};

vector<game> games;

//断开连接并将战局内存在的另一个人设置为player1，如果没有删除战局
void player_quit(SOCKET client_socket) {
	cout << client_socket << " quit" << endl;
	char buffer[1024] = {0};
	//遍历战局
	for (int games_indx = 0; games_indx < games.size(); games_indx++) {
		//遍历战局内客户端
		for (int i = 0; i < 2;i++) {
			//如果找到该客户端
			if (games[games_indx].client_ingame[i]!=NULL && games[games_indx].client_ingame[i]->socket == client_socket) {
				//设置战局内该客户端为空
				games[games_indx].client_ingame[i] = NULL;
				//与该客户端断联
				closesocket(client_socket);
				//检查该战局内玩家数
				//若战局内没有玩家
				if (games[games_indx].client_ingame[0] == NULL && games[games_indx].client_ingame[1] == NULL) {
					//删除战局
					games.erase(games.begin() + games_indx);
					break;
				}//若战局内有玩家
				else {
					//当前玩家在0号位
					if (games[games_indx].client_ingame[0] != NULL) {
						//向该玩家发送玩家2退出消息
						send(games[games_indx].client_ingame[0]->socket, "player 2 quit", sizeof("player 2 quit"), 0);
						break;
					}//当前玩家在1号位
					else if (games[games_indx].client_ingame[1] != NULL) {
						//向该玩家发送玩家1退出消息
						send(games[games_indx].client_ingame[1]->socket, "player 1 quit", sizeof("player 1 quit"), 0);
						//将该玩家设置为0号位
						games[games_indx].client_ingame[0] = games[games_indx].client_ingame[1];
						games[games_indx].client_ingame[1] = NULL;
						break;
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

	u_long mode = 0; // 设置为阻塞模式 非阻塞为1，阻塞为0
	if (ioctlsocket(client_socket, FIONBIO, &mode) == SOCKET_ERROR) {
		std::cerr << "ioctlsocket failed: " << WSAGetLastError() << std::endl;
		closesocket(client_socket);
		WSACleanup();
		return -1;
	}

	//获取客户端玩家昵称与所创建或加入房间号
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
		//若该房间存在
		if (num == games[games_indx].roomnum) {
			//房间有1人，加入
			if (games[games_indx].client_ingame[1] == NULL) {
				cout << "加入房间 " << games[games_indx].roomnum << endl;
				send(client_socket, "ready", sizeof("ready"), 0);
				games[games_indx].client_ingame[1] = &client_login;
				send(games[games_indx].client_ingame[1]->socket, games[games_indx].client_ingame[0]->name, sizeof(games[games_indx].client_ingame[0]->name), 0);
				send(games[games_indx].client_ingame[0]->socket, games[games_indx].client_ingame[1]->name, sizeof(games[games_indx].client_ingame[1]->name), 0);
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
	games.emplace_back(&client_login, num);

ready:
	mode = 1; // 设置为非阻塞模式 非阻塞为1，阻塞为0
	if (ioctlsocket(client_socket, FIONBIO, &mode) == SOCKET_ERROR) {
		std::cerr << "ioctlsocket failed: " << WSAGetLastError() << std::endl;
		closesocket(client_socket);
		WSACleanup();
		return -1;
	}
	while (1) {
		//没数据返回-1，断开连接返回0，有数据返回数据大小
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
	//windows启用网络功能
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	/**///创建socket套接字 协议地址簇IPV4 TCP协议 保护方式非阻塞
	SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == listen_socket) {
		printf("create lisn_socket failed!!! errcode: %d\n", GetLastError());
		return -1;
	}

	// 设置监听套接字为非阻塞模式
	u_long mode = 1; // 非阻塞模式
	if (ioctlsocket(listen_socket, FIONBIO, &mode) == SOCKET_ERROR) {
		std::cerr << "ioctlsocket failed: " << WSAGetLastError() << std::endl;
		closesocket(listen_socket);
		WSACleanup();
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

	char buffer[1024] = { 0 };
	//等待客户端连接
	while(1){
		SOCKET client_socket = accept(listen_socket, NULL, NULL);  //等待连接并返回客户端socket，非阻塞执行
		if (INVALID_SOCKET != client_socket){
		    SOCKET* sockfd = (SOCKET*)malloc(sizeof(SOCKET));
		    *sockfd = client_socket;
		    
		    //创建线程
		    CreateThread(NULL, 0, thread_func, sockfd, 0, NULL);
        }
		//遍历战局
		for (auto game : games) {
			//双方都准备，发送开始游戏消息
			if (game.client_ingame[0] != NULL && game.client_ingame[1] != NULL && game.client_ingame[0]->ready && game.client_ingame[1]->ready) {
				send(game.client_ingame[0]->socket, "start", sizeof("start"), 0);
				send(game.client_ingame[1]->socket, "start", sizeof("start"), 0);
				game.client_ingame[0]->ready = 0;
				game.client_ingame[1]->ready = 0;
			}
			//如果零号玩家认输或一号玩家胜利,向双方发送对应消息
			if (game.client_ingame[0] != NULL && game.client_ingame[1] != NULL && (game.client_ingame[0]->lose || game.client_ingame[1]->win)) {
				send(game.client_ingame[0]->socket, "lose", sizeof("lose"), 0);
				game.client_ingame[0]->lose = 0;
				send(game.client_ingame[1]->socket, "win", sizeof("win"), 0);
				game.client_ingame[1]->win = 0;

			}//如果零号玩家胜利或一号玩家认输,向双方发送对应消息
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