#define _WIN32_IE 0x0500
#define WM_TASKBAR_CREATED RegisterWindowMessage(TEXT("TaskbarCreated"))

#include <Windows.h>
#include <string>
#include <strsafe.h>
#include "resource.h"
using namespace std;
enum
{
	WM_TRAYICON = WM_USER + 100, //自定义托盘消息
	ID_LOCK,
	ID_UNLOCK,
	ID_EXIT
};

HHOOK KBhook = NULL;//钩子句柄
HINSTANCE hInst;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);//键盘钩子回调函数

void TrayMessage(HWND hwnd, int nFlag);//显示托盘气泡信息
void DestroyTrayIcon(HWND hwnd);//删除托盘图标
void CreateTrayMenu(HWND hwnd);//建立托盘菜单
BOOL isValidMessage(WORD message);          //验证信息的合法性
/*void PopUpMenu(ClipBoard clipboard, POINT point);   */      //弹出选择菜单
VOID APIENTRY DisplayContextMenu(HWND hwnd, POINT pt);
BOOL isLocked = false; //当前是否是锁定状态

int EnableKeyboardCapture();//激活键盘钩子
int DisableKeyboardCapture();//解除键盘钩子
void OnLock(HWND hwnd);
void OnUnlock(HWND hwnd);
void FirstLoad(HWND hwnd);
NOTIFYICONDATA IconData;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	HWND hwnd;
	//HICON Icon;
	MSG msg;
	WNDCLASS wndclass;
	static TCHAR szAppName[] = { L"Keyboard Locker" };
	hInst = hInstance;
	//初始化窗口
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hCursor = LoadCursor(NULL, IDI_APPLICATION);
	wndclass.hIcon = LoadIcon(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.hInstance = hInstance;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szAppName;

	RegisterClass(&wndclass);

	hwnd = CreateWindow(szAppName, szAppName, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, 0, 0, hInstance, NULL);
	//这里之后不需要显示窗口，新建托盘图标，操作全部在托盘图标上完成
	//Warning: HWND 和 HINSTANCE 不对的话，就会有问题
	IconData.cbSize = sizeof(NOTIFYICONDATA);
	IconData.hWnd = hwnd;
	IconData.uID = (UINT)hInstance;
	//IconData.hIcon = LoadIcon(0, MAKEINTRESOURCE(IDI_ICON1));
	IconData.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	IconData.uFlags = NIF_MESSAGE + NIF_ICON + NIF_TIP;
	IconData.uCallbackMessage = WM_TRAYICON;
	StringCchCopy(IconData.szTip, ARRAYSIZE(IconData.szTip), L"右键设置");
	Shell_NotifyIcon(NIM_ADD, &IconData);
	FirstLoad(hwnd
	);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (HIWORD(wParam) == 0)
		{
			//菜单动作的消息
			switch (LOWORD(wParam))
			{
			case ID_LOCK:
				OnLock(hwnd);
				break;

			case ID_UNLOCK:
				OnUnlock(hwnd);
				break;

			case ID_EXIT:
				::DestroyWindow(hwnd);
				break;
			}
		}
		break;

	case WM_TRAYICON:
		switch (lParam)
		{
			//case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
			SetForegroundWindow(hwnd);//加了这个后可以使得鼠标在点击别的区域选框消失
			CreateTrayMenu(hwnd);
			break;
		}
		break;

	case WM_DESTROY:
		DestroyTrayIcon(hwnd);
		PostQuitMessage(0);
		break;
	}
	if (message == WM_TASKBAR_CREATED)
	{
		//系统Explorer崩溃重启时，重新加载托盘
		Shell_NotifyIcon(NIM_ADD, &IconData);
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}


LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	//是键盘的动作全部忽略
	if (nCode == HC_ACTION)
	{
		return 1;
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}


BOOL isValidMessage(WORD message)
{
	return TRUE;
}


//the function to display shortcut menu
/*

VOID APIENTRY DisplayContextMenu(HWND hwnd, POINT pt)
{
	HMENU hmenu;            // top-level menu
	HMENU hmenuTrackPopup;  // shortcut menu

	// Load the menu resource.

	if ((hmenu = LoadMenu(NULL, L"ShortcutExample")) == NULL)
		return;

	// TrackPopupMenu cannot display the menu bar so get
	// a handle to the first shortcut menu.

	hmenuTrackPopup = GetSubMenu(hmenu, 0);

	// Display the shortcut menu. Track the right mouse
	// button.

	TrackPopupMenu(hmenuTrackPopup,
		TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		pt.x, pt.y, 0, hwnd, NULL);

	// Destroy the menu.

	DestroyMenu(hmenu);
}
*/

int EnableKeyboardCapture()
{
	isLocked = true;
	// 13 表示使用低级键盘钩子，和 WM_KEYBOARD 不同
	KBhook = !KBhook ? SetWindowsHookEx(13, (HOOKPROC)KeyboardHookProc, (HINSTANCE)GetModuleHandle(NULL), 0) : KBhook;
	if (KBhook)
	{
		return  1;
	}
	else return -1;
}


int DisableKeyboardCapture()
{
	isLocked = false;
	if (KBhook == NULL)
	{
		return 0;
	}
	else
	{
		BOOL flag;
		flag = UnhookWindowsHookEx(KBhook);
		KBhook = NULL;
		if (flag)
		{
			return  1;
		}
		else return  0;
	}
}
void FirstLoad(HWND hwnd)
{
	NOTIFYICONDATA fstNote = {};
	fstNote.cbSize = sizeof(NOTIFYICONDATA);
	fstNote.uTimeout = 1000;
	fstNote.hWnd = hwnd;
	fstNote.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	fstNote.uID = (UINT)GetModuleHandle(NULL);
	fstNote.uFlags = NIF_INFO | NIF_ICON | NIF_TIP;
	lstrcpy(fstNote.szInfoTitle, L"Keyboard-Locker β\n");
	lstrcpy(fstNote.szInfo, L"程序已启动！\n请在系统托盘里进行操作\n");
	Shell_NotifyIcon(NIM_MODIFY, &fstNote);
}

void TrayMessage(HWND hwnd)
{
	NOTIFYICONDATA MyNotice = {};
	MyNotice.cbSize = sizeof(NOTIFYICONDATA);
	MyNotice.hWnd = hwnd;
	MyNotice.uID = (UINT)GetModuleHandle(NULL);
	MyNotice.uFlags = NIF_INFO | NIF_ICON | NIF_TIP;//之前是uFlags没有赋值导致弹不出来！
	MyNotice.uTimeout = 500;
	//MyNotice.hBalloonIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	MyNotice.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	lstrcpy(MyNotice.szInfoTitle, L"Keyboard-Locker β\n");
	if (isLocked)
	{
		lstrcpy(MyNotice.szInfo, L"键盘已锁定");
	}
	else
	{
		lstrcpy(MyNotice.szInfo, L"键盘已解锁");
	}
	Shell_NotifyIcon(NIM_MODIFY, &MyNotice);
}


void DestroyTrayIcon(HWND hwnd)
{
	NOTIFYICONDATA nid;
	nid.uID = (UINT)GetModuleHandle(NULL);
	nid.cbSize = sizeof(nid);
	lstrcpy(nid.szInfoTitle, L"Keyboard-Locker β\n");
	lstrcpy(nid.szInfo, L"程序已退出\n");
	nid.hWnd = hwnd;
	//nid.uTimeout = 1000;
	nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	nid.uFlags = NIF_INFO | NIF_ICON ;
	Shell_NotifyIcon(NIM_MODIFY, &nid);
	//Shell_NotifyIcon(NIM_DELETE, &nid);
}


void CreateTrayMenu(HWND hwnd)
{
	HMENU hMenu;
	hMenu = CreatePopupMenu();
	if (isLocked)
	{
		AppendMenu(hMenu, MF_STRING, ID_UNLOCK, L"解锁键盘");
	}
	else
	{
		AppendMenu(hMenu, MF_STRING, ID_LOCK, L"锁定键盘");
	}
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, ID_EXIT, L"退出");
	POINT pt;
	GetCursorPos(&pt);
	TrackPopupMenu(hMenu, TPM_RIGHTALIGN, pt.x, pt.y, 0, hwnd, NULL);
	//DestroyMenu(hMenu);

}
void OnUnlock(HWND hwnd)
{
	DisableKeyboardCapture();
	TrayMessage(hwnd);
}
void OnLock(HWND hwnd)
{
	EnableKeyboardCapture();
	TrayMessage(hwnd);
}
