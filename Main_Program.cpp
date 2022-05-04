#include <iostream>
#include <string>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include "func_input.h"
using namespace std;

//--------------------------------------------数据--------------------------------------------------
const int initial_page_1 = 1, initial_page_2 = 2, initial_page_3 = 3, initial_page_4 = 4;
const int game_page = -1, load_page = -2, save_page = -3, end_page = -4, start_page = 100, return_page = 200;

int turnID;
int currBotColor; // 机器人所执颜色（1为黑，-1为白，棋盘状态亦同）
int gridInfo[7][7] = { 0 }; // 先x后y，记录棋盘状态
int blackPieceCount = 2, whitePieceCount = 2;//计数
int startX, startY, resultX, resultY;

static int delta[24][2] = { { 1,1 },{ 0,1 },{ -1,1 },{ -1,0 },
{ -1,-1 },{ 0,-1 },{ 1,-1 },{ 1,0 },
{ 2,0 },{ 2,1 },{ 2,2 },{ 1,2 },
{ 0,2 },{ -1,2 },{ -2,2 },{ -2,1 },
{ -2,0 },{ -2,-1 },{ -2,-2 },{ -1,-2 },
{ 0,-2 },{ 1,-2 },{ 2,-2 },{ 2,-1 } };
double estimate[7][7] = {
{ 1,0.965051,0.849347,0.803681,0.779659,0.789668,1 },
{ 0.965051,0.87366,0.835621,0.743914,0.715635,0.719026,0.789668},
{ 0.849347,0.835621,0.788361,0.701912,0.677423,0.715635,0.779659},
{ 0.803681,0.743914,0.701912,0.70752,0.701912,0.743914,0.803681},
{ 0.779659,0.715635,0.677423,0.701912,0.788361,0.835621,0.849347},
{ 0.789668,0.719026,0.715635,0.743914,0.835621,0.87366,0.965051},
{ 1,0.789668,0.779659,0.803681,0.849347,0.965051,1} };

struct Node
{
	double a = -100000, b = 100000;
	int board[7][7];
};
struct possible
{
	int beginPos[1000][2], possiblePos[1000][2], posCount;
};
struct decision
{
	int x1, y1, x2, y2;
};

//----------------------------------------函数声明-----------------------------------------------------
void clear_srceen();//清屏
void word_title();//艺术字
int menu(int page);//目录界面
int menu_save();//存档界面
void save_file();//存档函数
int menu_load();//读档界面
void load_file();//读档函数
int menu_end(int x);//终止退出界面
int menu_game_paly();//初始进入游戏界面

decision AI();//AI决策交互
decision people();//人类方交互
bool judge(int qipan[7][7]);//判断游戏是否结束以及输赢
int winner(int qipan[7][7]);//判断获胜者
void chess_table_dynamic();//实时棋盘输出

bool inMap(int x, int y);//判断是否在地图内
bool ProcStep(int x0, int y0, int x1, int y1, int color, int(*p)[7]);//落子并计数
void ProcStep_temp(int x0, int y0, int x1, int y1, int color, int(*p)[7]);//模拟落子
possible find_possible(int(*p)[7], int color, int choice);//寻找可能落子集合
double value_evaluate(int(*temp)[7], int num);//估值函数
double alpha_beta(Node* child, int depth, int color_temp, int choice);//alpha_beta剪枝

//--------------------------------------AI与人机交互部分------------------------------------------------------
inline bool inMap(int x, int y)
{
	if (x < 0 || x > 6 || y < 0 || y > 6)
		return false;
	return true;
}

possible find_possible(int(*p)[7], int color, int choice)
{
	possible str;
	str.posCount = 0;
	int x0, y0, x1, y1, dir;
	if (choice == 1)
	{
		for (y0 = 0; y0 < 7; y0++)
			for (x0 = 0; x0 < 7; x0++)
			{
				if (p[x0][y0] != 0)
					continue;
				for (dir = 0; dir < 24; dir++)
				{
					x1 = x0 + delta[dir][0];
					y1 = y0 + delta[dir][1];
					if (!inMap(x1, y1))
						continue;
					if (p[x1][y1] != color)
						continue;
					str.beginPos[str.posCount][0] = x1;
					str.beginPos[str.posCount][1] = y1;
					str.possiblePos[str.posCount][0] = x0;
					str.possiblePos[str.posCount][1] = y0;
					str.posCount++;
				}
			}
	}
	else if (choice == 2)
	{
		for (y0 = 0; y0 < 7; y0++)
			for (x0 = 0; x0 < 7; x0++)
			{
				if (p[x0][y0] != color)
					continue;
				for (dir = 0; dir < 24; dir++)
				{
					x1 = x0 + delta[dir][0];
					y1 = y0 + delta[dir][1];
					if (!inMap(x1, y1))
						continue;
					if (p[x1][y1] != 0)
						continue;
					str.beginPos[str.posCount][0] = x0;
					str.beginPos[str.posCount][1] = y0;
					str.possiblePos[str.posCount][0] = x1;
					str.possiblePos[str.posCount][1] = y1;
					str.posCount++;
				}
			}
	}
	return str;
}

bool ProcStep(int x0, int y0, int x1, int y1, int color, int(*p)[7])
{
	if (color == 0)
		return false;
	if (x1 == -1) // 无路可走，跳过此回合
		return false;
	if (!inMap(x0, y0) || !inMap(x1, y1)) // 超出边界
		return false;
	if (p[x0][y0] != color)
		return false;
	int dx, dy, x, y, currCount = 0, dir;
	dx = abs((x0 - x1)), dy = abs((y0 - y1));
	if ((dx == 0 && dy == 0) || dx > 2 || dy > 2) // 保证不会移动到原来位置，而且移动始终在5×5区域内
		return false;
	if (p[x1][y1] != 0) // 保证移动到的位置为空
		return false;
	if (dx == 2 || dy == 2) // 如果走的是5×5的外围，则不是复制粘贴
		p[x0][y0] = 0;
	else
	{
		if (color == 1)
			blackPieceCount++;
		else
			whitePieceCount++;
	}

	p[x1][y1] = color;
	for (dir = 0; dir < 8; dir++) // 影响邻近8个位置
	{
		x = x1 + delta[dir][0];
		y = y1 + delta[dir][1];
		if (!inMap(x, y))
			continue;
		if (p[x][y] == -color)
		{
			currCount++;
			p[x][y] = color;
		}
	}
	if (currCount != 0)
	{
		if (color == 1)
		{
			blackPieceCount += currCount;
			whitePieceCount -= currCount;
		}
		else
		{
			whitePieceCount += currCount;
			blackPieceCount -= currCount;
		}
	}
	return true;
}

void ProcStep_temp(int x0, int y0, int x1, int y1, int color, int(*p)[7])
{
	int dx, dy, x, y, dir;
	dx = abs((x0 - x1)), dy = abs((y0 - y1));
	if (dx == 2 || dy == 2) // 如果走的是5×5的外围，则不是复制粘贴
		p[x0][y0] = 0;
	p[x1][y1] = color;
	for (dir = 0; dir < 8; dir++) // 影响邻近8个位置
	{
		x = x1 + delta[dir][0];
		y = y1 + delta[dir][1];
		if (!inMap(x, y))
			continue;
		if (p[x][y] == -color)
		{
			p[x][y] = color;
		}
	}
}

double value_evaluate(int(*temp)[7], int num)
{
	const int(*gridInfo_temp)[7] = temp;
	double value_temp = 0;
	for (int i = 0; i < 7; i++)
		for (int j = 0; j < 7; j++)
		{
			if (gridInfo_temp[j][i] == 0)
			{
				if (num == 3)
					value_temp += 1;
				else if (num == 4)
					value_temp -= 1;
				else
					continue;
			}
			if (gridInfo_temp[j][i] == -currBotColor)
			{
				if (num == 3 || num == 4)
					value_temp -= 1;
				else
				{
					value_temp -= estimate[j][i];
					
				}

			}
			else if (gridInfo_temp[j][i] == currBotColor)
			{
				if (num == 3 || num == 4)
					value_temp += 1;
				else
				{
					value_temp += estimate[j][i];
				}
			}
		}
	return value_temp;
}

double alpha_beta(Node* child, int depth, int color_temp, int choice)
{

	possible temp_possible = find_possible((*child).board, color_temp, choice);
	if (temp_possible.posCount == 0)
	{
		if (color_temp == currBotColor)
		{
			if (value_evaluate((*child).board, 4) > 0)
				return 100000;
			else
				return -100000;
		}
		else
		{
			if (value_evaluate((*child).board, 3) < 0)
				return -100000;
			else
				return 100000;
		}
	}
	if (depth == 0)
	{
		return value_evaluate((*child).board, 2);
	}
	else
	{
		if (color_temp == currBotColor)
		{
			double v = -100000;
			for (int i = 0; i < temp_possible.posCount; i++)
			{
				Node children = (*child);
				ProcStep_temp(temp_possible.beginPos[i][0], temp_possible.beginPos[i][1],
					temp_possible.possiblePos[i][0], temp_possible.possiblePos[i][1], currBotColor, children.board);
				v = max(alpha_beta(&children, depth - 1, -color_temp, choice), v);
				(*child).a = max((*child).a, v);
				if ((*child).a >= (*child).b)
					break;
			}
			return v;
		}
		else
		{
			double v = 100000;
			for (int i = 0; i < temp_possible.posCount; i++)
			{
				Node children = (*child);
				ProcStep_temp(temp_possible.beginPos[i][0], temp_possible.beginPos[i][1],
					temp_possible.possiblePos[i][0], temp_possible.possiblePos[i][1], -currBotColor, children.board);
				v = min(alpha_beta(&children, depth - 1, -color_temp, choice), v);
				(*child).b = min((*child).b, v);
				if ((*child).a >= (*child).b)
					break;
			}
			return v;
		}
	}
}

decision AI()
{
	int choice = 0;
	if (blackPieceCount + whitePieceCount >= 25)
		choice = 1;
	else
		choice = 2;

	// 找出合法落子点
	possible str1 = find_possible(gridInfo, currBotColor, choice);
	decision deci;
	int startX = -1, startY = -1, resultX = -1, resultY = -1;

	Node result;
	result.a = -100000;
	result.b = 100000;

	for (int i = 0; i < 7; i++)
	{
		for (int j = 0; j < 7; j++)
		{
			result.board[i][j] = gridInfo[i][j];
		}
	}

	double v = 100000;
	bool m = false;
	for (int i = 0; i < str1.posCount; i++)
	{
		Node children = result;
		ProcStep_temp(str1.beginPos[i][0], str1.beginPos[i][1],
			str1.possiblePos[i][0], str1.possiblePos[i][1], currBotColor, children.board);
		int num = 10;
		if (blackPieceCount + whitePieceCount >= 47)
			num = 7;
		else if (blackPieceCount + whitePieceCount >= 46)
			num = 5;
		else if (blackPieceCount + whitePieceCount >= 42)
			num = 4;
		else if (blackPieceCount + whitePieceCount >= 40)
			num = 3;
		else if (blackPieceCount + whitePieceCount >= 15)
			num = 2;
		else
			num = 3;
		v = max(alpha_beta(&children, num, currBotColor, choice), v);
		if (result.a < v)
		{
			result.a = v;
			startX = str1.beginPos[i][0];
			startY = str1.beginPos[i][1];
			resultX = str1.possiblePos[i][0];
			resultY = str1.possiblePos[i][1];
		}
	}
	if (startX == -1)
	{
		if (str1.posCount > 0)
		{
			srand(time(0));
			int choice = rand() % str1.posCount;
			startX = str1.beginPos[choice][0];
			startY = str1.beginPos[choice][1];
			resultX = str1.possiblePos[choice][0];
			resultY = str1.possiblePos[choice][1];
		}
	}
	deci.x1 = startX; deci.x2 = resultX; deci.y1 = startY; deci.y2 = resultY;
	return deci;
}

decision people()
{
	decision deci;
	cin >> deci.x1 >> deci.y1 >> deci.x2 >> deci.y2;
	return deci;
}

//-------------------------------------界面与功能部分---------------------------------------------------
void clear_srceen()
{
	system("cls");
}

void word_title()
{
	cout << '\t' << '\t'
		<< "  ██████████    ███████████     ██████████   ███         ███     ███         ███ " << endl
		<< '\t' << '\t'
		<< "  ███    ███     █████████      ███    ███    ███       ███       ███       ███" << endl
		<< '\t' << '\t'
		<< "  ███    ███        ███         ███    ███     ███     ███         ███     ███   " << endl
		<< '\t' << '\t'
		<< "  ███    ███        ███         ███    ███       ███  ███            ███  ███    " << endl
		<< '\t' << '\t'
		<< "████████████        ███       ████████████         █████              ██████     " << endl
		<< '\t' << '\t'
		<< "  ███    ███        ███         ███    ███       ███   ███           ███   ███    " << endl
		<< '\t' << '\t'
		<< "  ███    ███        ███         ███    ███     ████     ████       ███      ███  " << endl
		<< '\t' << '\t'
		<< "  ███    ██         ███         ███    ██     ████        ████   ████         ████ " << endl << endl;
	//艺术字体ATAXX    
}

void chess_table_dynamic()
{
	clear_srceen();
	word_title();
	if (currBotColor == 1)
	{
		cout << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
			<< "      SCORE" << endl
			<< '\t' << '\t' << '\t' << '\t' << '\t'
			<< "   YOU:   " << whitePieceCount << '\t' << "   COMPUTER:   " << blackPieceCount << endl;//比分栏
	}
	else
	{
		cout << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
			<< "      SCORE" << endl
			<< '\t' << '\t' << '\t' << '\t' << '\t'
			<< "   YOU:   " << blackPieceCount << '\t' << "   COMPUTER:   " << whitePieceCount << endl;//比分栏
	}
	cout << '\t' << '\t' << '\t' << '\t' << '\t'
		<< "      输入 -1 -1 -1 -1 中止游戏" << endl;
	cout << '\t' << '\t' << '\t' << '\t'
		<< "   ═════════════════════════════════════════════" << endl;//分隔符

	cout << '\t' << '\t' << '\t' << '\t' << '\t'
		<< "  y  0   1   2   3   4   5   6  " << endl;//列指标

	cout << '\t' << '\t' << '\t' << '\t' << '\t'
		<< "x ╔═══╦═══╦═══╦═══╦═══╦═══╦═══╗" << endl
		<< '\t' << '\t' << '\t' << '\t' << '\t'
		<< "0 ║";
	for (int i = 0; i < 7; i++)
	{
		if (gridInfo[0][i] == 1)
			cout << " ●║";
		else if (gridInfo[0][i] == -1)
			cout << " ○║";
		else
			cout << "   ║";
	}

	cout<< endl;
	for (int i = 1; i < 7; i++)
	{
		cout << '\t' << '\t' << '\t' << '\t' << '\t'
			<< "  ╠═══╬═══╬═══╬═══╬═══╬═══╬═══╣" << endl
			<< '\t' << '\t' << '\t' << '\t' << '\t'
			<< i << " ║" ;
		for (int j = 0; j < 7; j++)
		{
			if (gridInfo[i][j] == 1)
				cout << " ●║";
			else if (gridInfo[i][j] == -1)
				cout << " ○║";
			else
				cout << "   ║";
		}
		cout << endl;
	}
	cout << '\t' << '\t' << '\t' << '\t' << '\t'
		<< "  ╚═══╩═══╩═══╩═══╩═══╩═══╩═══╝" << endl;//基本棋盘表
}

int menu(int page)
{
	clear_srceen();
	word_title();
	if (page == initial_page_1)
	{
		cout << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
			<< "> 1.开始新游戏 <" << endl << endl;
	}
	else
	{
		cout << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
			<< "  1.开始新游戏  " << endl << endl;
	}
	if (page == initial_page_2)
	{
		cout << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
			<< "> 2.读档 <" << endl << endl;
	}
	else
	{
		cout << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
			<< "  2.读档  " << endl << endl;
	}
	if (page == initial_page_3)
	{
		cout << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
			<< "> 3.存档 <" << endl << endl;
	}
	else
	{
		cout << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
			<< "  3.存档  " << endl << endl;
	}
	if (page == initial_page_4)
	{
		cout << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
			<< "> 4.退出 <" << endl << endl;
	}
	else
	{
		cout << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
			<< "  4.退出  " << endl << endl;
	}
	cout << '\t' << '\t' << '\t'
		<< "请通过“↑←”或“→↓”选择选项" << '\t' << '\t' << "按“ENTER”进入" << endl;//界面内容
	return  catch_keyboard(page);
}

int menu_load()
{
	clear_srceen();
	word_title();
	load_file();
	return catch_keyboard(101);
}

void load_file()
{
	FILE* fp;
	fp = fopen("棋盘数据.dat", "rb");/*打开一个二进制文件只读*/
	fread(gridInfo, sizeof(int), sizeof(gridInfo), fp);
	fread(&currBotColor, sizeof(int), 1, fp);
	fread(&turnID, sizeof(int), 1, fp);
	fread(&blackPieceCount, sizeof(int), 1, fp);
	fread(&whitePieceCount, sizeof(int), 1, fp);
	fclose(fp);
	cout << endl << endl << '\t' << '\t' << '\t' << '\t' << '\t'
		<< "复盘完成，按任意键开始游戏" << endl;
}

int menu_save()
{
	clear_srceen();
	word_title();
	save_file();
	return catch_keyboard(201);
}

void save_file()
{
	FILE* fp;
	fp = fopen("棋盘数据.dat", "wb"); /*创建一个二进制文件只写*/
	fwrite(gridInfo, sizeof(int), sizeof(gridInfo), fp);
	fwrite(&currBotColor, sizeof(int), 1, fp);
	fwrite(&turnID, sizeof(int), 1, fp);
	fwrite(&blackPieceCount, sizeof(int), 1, fp);
	fwrite(&whitePieceCount, sizeof(int), 1, fp);
	fclose(fp);
	cout << endl << endl  << '\t' << '\t' << '\t' << '\t' << '\t'
		<< "存盘完成，按任意键返回主界面" << endl;
}

bool judge(int qipan[7][7])
{
	int flag1 = 0, flag2 = 0;
	for (int i = 0; i < 7; i++)
	{
		for (int j = 0; j < 7; j++)
		{
			if (qipan[i][j] == 1)
			{
				for (int dir = 0; dir < 24; dir++)
				{
					int x = i + delta[dir][0];
					int y = j + delta[dir][1];
					if (!inMap(x, y))
						continue;
					if (qipan[x][y] == 0)
						flag1++;
				}
			}
			if (qipan[i][j] == -1)
			{
				for (int dir = 0; dir < 24; dir++)
				{
					int x = i + delta[dir][0];
					int y = j + delta[dir][1];
					if (!inMap(x, y))
						continue;
					if (qipan[x][y] == 0)
						flag2++;
				}
			}
		}
	}
	if (flag1 == 0 || flag2 == 0)
		return true;
	else
		return false;
}

int winner(int qipan[7][7])
{
	int black = 0;
	int white = 0;
	for (int i = 0; i < 7; i++)
	{
		for (int j = 0; j < 7; j++)
		{
			if (qipan[i][j] == 1)
				black++;
			if (qipan[i][j] == -1)
				white++;
		}
	}
	if (black > white)
		return 1;
	else
		return -1;
}

int menu_end(int x)
{
	Sleep(500);
	clear_srceen();
	word_title();
	cout << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
	cout << "┏┛┻━━━━━━━━━━┛┻┓" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t';
	cout << "┃｜｜｜｜｜｜｜┃" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
	cout << "┃　　  ━　　　 ┃" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t';
	cout << "┃　 ┳┛    ┗┳   ┃" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
	cout << "┃　　　　　　　┃" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
	if (x == 1)
	{
		cout << "┃　 　 ┻　　 　┃" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t';
		cout << "┃　　　　　　　┃" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
		cout << "┗━━━┓　　　┏━━━┛" << endl << '\t' << '\t' << '\t' << '\t' << '\t'  << '\t';
		cout << "　　┃　你　┃　　" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
		cout << "　　┃　输　┃　　" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
		cout << "　　┃　了　┃　　" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
	}
	else
	{
		cout << "┃　　 　\//　 　　┃" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
		cout << "┃　　　　　　　┃" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
		cout << "┗━━━┓　　　┏━━━┛" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
		cout << "　　┃　你　┃　　" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
		cout << "　　┃　赢　┃　　" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
		cout << "　　┃　了　┃　　" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
	}
	cout << "　　┃　  　┃" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
	cout << "　　┃　　　┗━━━━━━━┓" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
	cout << "　　┃            　┣┓" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t';
	cout << "　　┃            　┃" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
	cout << "　　┗━━┓┓┏━━━━┳┓┏━━┛" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
	cout << "　　　 ┃┫┫　  ┃┫┫" << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' ;
	cout << "　     ┗┻┛　  ┗┻┛" << endl;
	cout << endl << endl << '\t' << '\t' << '\t' << '\t' << '\t'
		<< "按任意键返回主界面" << endl;
	return catch_keyboard(101);
}

int menu_game_paly()
{
	// 初始化棋盘
	memset(gridInfo, 0, sizeof(gridInfo));
	gridInfo[0][0] = gridInfo[6][6] = 1;  //|黑|白|
	gridInfo[6][0] = gridInfo[0][6] = -1; //|白|黑|
	turnID = 1;
	whitePieceCount = 2; blackPieceCount = 2;
	int choice = 1, ans = 1;
	while (1)
	{
		clear_srceen();
		word_title();
		choice = ans;
		if (choice == 1)
		{
			cout << endl << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
				<< "> 1.白方 <";
			cout << endl << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
				<< "  2.黑方  " << endl;
		}
		else 
		{
			cout << endl << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
				<< "  1.白方  ";
			cout << endl << endl << '\t' << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
				<< "> 2.黑方 <" << endl;
		}
		cout <<endl<< '\t' << '\t' << '\t'
			<< "请通过“↑←”或“→↓”选择选项" << '\t' << '\t' << "按“ENTER”进入" << endl;
		ans = catch_keyboard(choice+10);
		if (ans < 0)
		{
			if (choice == 1)
				return 1;
			else
				return -1;
		}
	}
}

//-----------------------------------MAIN函数-------------------------------------------------------------
int main()
{
	system("color 70");//设置背景颜色
loop:
	int page = initial_page_1;

	do
	{
		page = menu(page);
	} while (page > 0);

	if (page == game_page)
	{
		currBotColor=menu_game_paly();
		page = start_page;
	}
	else if (page == load_page)
		page = menu_load();
	else if (page == save_page)
		page = menu_save();
	else if (page == end_page)
		return 0;

	if (page == start_page)
	{
		while (1)
		{
			bool flag = false;
			if (currBotColor == turnID)
			{
				while (1&&!flag)
				{
					flag = false;
					chess_table_dynamic();
					decision tmp = AI();
					if (ProcStep(tmp.x1, tmp.y1, tmp.x2, tmp.y2, currBotColor, gridInfo))
						flag = true;
					Sleep(500);
					chess_table_dynamic();
					if (flag)
						turnID = -turnID;
					if (judge(gridInfo))
						break;
				}
			}
			if (currBotColor != turnID)
			{
				while (1 && !flag)
				{
					flag = false;
					chess_table_dynamic();
					decision tmp = people();
					if (tmp.x1 == -1 && tmp.x2 == -1 && tmp.y1 == -1 && tmp.y2 == -1)
						goto loop;
					if (ProcStep(tmp.x1, tmp.y1, tmp.x2, tmp.y2, -currBotColor, gridInfo))
						flag = true;
					chess_table_dynamic();
					if (flag)
						turnID = -turnID;
				}
			}
			if (judge(gridInfo))
				break;
		}
		if (currBotColor == winner(gridInfo))
			page = menu_end(1);
		else
			page = menu_end(0);
	}
	if (page == return_page)
		goto loop;
	return 0;

}
