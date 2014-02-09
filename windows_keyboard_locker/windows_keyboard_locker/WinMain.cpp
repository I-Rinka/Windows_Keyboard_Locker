#define _WIN32_IE 0x0500
#include <Windows.h>
#include <string>
#include <strsafe.h>
#include "resource.h"
using namespace std;
enum
{
	WM_TRAYICON = WM_USER + 100, //�Զ���������Ϣ
	ID_LOCK,
	ID_UNLOCK,
	ID_EXIT
};


HHOOK KBhook = NULL;//���Ӿ��

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);//���̹��ӻص�����

void TrayMessage(HWND hwnd, LPCTSTR szText);//��ʾ����������Ϣ
void DestroyTrayIcon(HWND hwnd);//ɾ������ͼ��
void CreateTrayMenu(HWND hwnd);//�������̲˵�
BOOL isValidMessage(WORD message);          //��֤��Ϣ�ĺϷ���

/*void PopUpMenu(ClipBoard clipboard, POINT point);   */      //����ѡ��˵�
VOID APIENTRY DisplayContextMenu(HWND hwnd, POINT pt);

int EnableKeyboardCapture();//������̹���
int DisableKeyboardCapture();//������̹���

void OnLock(HWND hwnd);
void OnUnlock(HWND hwnd);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	HWND hwnd;
	MSG msg;
	NOTIFYICONDATA IconData;
	WNDCLASS wndclass;
	static TCHAR szAppName[] = { L"Keyboard Lock" };

	//��ʼ������
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

	//����֮����Ҫ��ʾ���ڣ��½�����ͼ�꣬����ȫ��������ͼ�������
	//Warning: HWND �� HINSTANCE ���ԵĻ����ͻ�������

	IconData.cbSize = sizeof(NOTIFYICONDATA);
	IconData.hWnd = hwnd;
	IconData.uID = (UINT)hInstance;
	IconData.hIcon = LoadIcon(0, MAKEINTRESOURCE(IDI_INFORMATION));
	IconData.uFlags = NIF_MESSAGE + NIF_ICON + NIF_TIP;
	IconData.uCallbackMessage = WM_TRAYICON;

	//strcpy(IconData.szTip, "������");

	Shell_NotifyIcon(NIM_ADD, &IconData);

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
			//�˵���������Ϣ
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
		case WM_LBUTTONDOWN:
			CreateTrayMenu(hwnd);
			break;
		}
		break;

	case WM_DESTROY:
		DestroyTrayIcon(hwnd);
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}


LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	//�Ǽ��̵Ķ���ȫ������
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

string copyToCustomeClipBoard()
{
	char * buffer = NULL;
	//�򿪼�����
	string fromClipboard;
	if (OpenClipboard(NULL))
	{
		HANDLE hData = GetClipboardData(CF_TEXT);
		char * buffer = (char*)GlobalLock(hData);
		fromClipboard = buffer;
		GlobalUnlock(hData);
		CloseClipboard();

	}

	return fromClipboard;
}



//the function to display shortcut menu
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

int EnableKeyboardCapture()
{
	// 13 ��ʾʹ�õͼ����̹��ӣ��� WM_KEYBOARD ��ͬ
	KBhook = !KBhook ? SetWindowsHookEx(13, (HOOKPROC)KeyboardHookProc, (HINSTANCE)GetModuleHandle(NULL), 0) : KBhook;

	return (KBhook ? 1 : -1);
}


int DisableKeyboardCapture()
{
	if (KBhook == NULL)
	{
		return 0;
	}
	else
	{
		BOOL flag;
		flag = UnhookWindowsHookEx(KBhook);
		KBhook = NULL;
		return (flag ? 1 : -1);
	}
}


void TrayMessage(HWND hwnd, LPCTSTR szText)
{
	NOTIFYICONDATA nid = {};
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = (UINT)GetModuleHandle(NULL);
	nid.uFlags = NIF_INFO | NIF_ICON;
	nid.dwInfoFlags = NIIF_INFO;
	nid.uTimeout = 1000;
	nid.hIcon = LoadIcon(0, MAKEINTRESOURCE(IDI_INFORMATION));

	//StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), L"Windows KeyBoard Locker");
	/*wsprintf(nid.szInfo, szText);*/
	StringCchCopy(nid.szInfo, ARRAYSIZE(nid.szInfo), szText);
	/*strcpy(nid.szInfoTitle, (TCHAR )"������");*/
	
	Shell_NotifyIcon(NIM_MODIFY, &nid);
}


void DestroyTrayIcon(HWND hwnd)
{
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(nid);
	nid.uID = (UINT)GetModuleHandle(NULL);
	nid.hWnd = hwnd;
	nid.uFlags = 0;
	Shell_NotifyIcon(NIM_DELETE, &nid);
}


void CreateTrayMenu(HWND hwnd)
{
	HMENU hMenu;
	hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_LOCK, L"Lock KeyBoard");
	AppendMenu(hMenu, MF_STRING, ID_UNLOCK, L"Unlock KeyBoard");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, ID_EXIT, L"Exit");

	POINT pt;
	GetCursorPos(&pt);

	TrackPopupMenu(hMenu, TPM_RIGHTALIGN, pt.x, pt.y, 0, hwnd, NULL);
}


void OnUnlock(HWND hwnd)
{
	switch (DisableKeyboardCapture())
	{
	case 0:
		TrayMessage(hwnd, (LPCTSTR)"KeyBoard hasn't been locked");
		break;
	case 1:
		TrayMessage(hwnd, (LPCTSTR)"KeyBoard has been unlocked");
		break;
	case -1:
		TrayMessage(hwnd, (LPCTSTR)"KeyBoard unlock failed");
		break;
	}
}


void OnLock(HWND hwnd)
{
	switch (EnableKeyboardCapture())
	{
	case 1:
		TrayMessage(hwnd, (LPCTSTR)"");
		break;

	case -1:
		TrayMessage(hwnd, (LPCTSTR)"KeyBoard lock failed");
		break;
	}
}
