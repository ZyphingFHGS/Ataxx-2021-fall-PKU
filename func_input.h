#include "windows.h"
#include "stdio.h"
#include <conio.h>
#include <windows.h>

HANDLE hIn;

inline int catch_keyboard(int a)
{
	hIn = GetStdHandle(STD_INPUT_HANDLE); // 获取标准输入设备句柄
	INPUT_RECORD keyRec;
	DWORD res;
	ReadConsoleInput(hIn, &keyRec, 1, &res);
	ReadConsoleInput(hIn, &keyRec, 1, &res);
	if (a == 101)
		return 100;
	else if (a == 201)
		return 200;
	if (keyRec.Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
		return a * (-1);
	else if (keyRec.Event.KeyEvent.wVirtualKeyCode == VK_UP || keyRec.Event.KeyEvent.wVirtualKeyCode == VK_LEFT)
	{
		if (a == 1)
			return 4;
		else if (a == 11)
			return 2;
		else if (a == 12)
			return 1;
		else
			return  (a - 1);
	}
	else if (keyRec.Event.KeyEvent.wVirtualKeyCode == VK_RIGHT || keyRec.Event.KeyEvent.wVirtualKeyCode == VK_DOWN)
	{
		if (a == 4 || a == 12)
			return 1;
		if (a == 11)
			return 2;
		else
			return (a + 1);
	}
	else
	{
		catch_keyboard(a);
	}
}

