#include<Windows.h>

#define BUFSIZE 65535
#define SHIFTED 0x8000
const  char* ClassName = "Chương trình nhập ký tự từ bàn phím";
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = ClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Cannot Register window", "Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	HWND hwnd;
	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		ClassName,
		"Chuong trinh nhap ky tu tu ban phim",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		600,
		300,
		NULL,
		NULL,
		hInstance,
		NULL
	);
	if (hwnd == NULL)
	{
		MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	MSG Msg;

	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;

}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc; // Thiết bị ngữ cảnh
	TEXTMETRIC tm; // cấu trúc metric của văn bản
	static DWORD dwCharX; //Bề ngang của kí tự
	static DWORD dwCharY; // Chiêu dài của kí tự
	static DWORD dwClientX; //Bê ngang của vùng làm việc
	static DWORD dwClientY; //Chiều dài của vùng làm việc
	static DWORD dwLineLen; // Chiều dài của một dòng
	static DWORD dwLines; // sô dòng văn bẩn trong vùng làm việc
	static int nCaretPosX = 0; //Tọa độ X của caret
	static int nCaretPosY = 0; //Tọa độy của caret
	static int nCharWidth = 0; //Bề dày của kí tự
	static int cch = 0; // SỐ kí tự trong buffer
	static int nCurChar = 0; // Chỉ đến kí tự hiện thời trong buffer
	static PTCHAR pchlnputBuf; //Kí tự tạm đưa vào buffer
	int i, j; //Biến lặp
	int cCR = 0; //Số kí tự xuống dòng
	int nCRIndex = 0; //Chỉ đến kí tự xuống dòng cuối cùng
	int nVirtKey; //Mã phím ảo
	TCHAR szBuf[128]; //Buffer tạm
	TCHAR ch; //Kí tự
	PAINTSTRUCT ps; //Dùng cho hàm BeginPaint 
	RECT rc; //Hình chữ nhật trong hàm DrawText 
	SIZE sz; //Kích thước của chuỗi 
	COLORREF crPrevText; //Màu của văn bản 
	COLORREF crPrevBk; //Màu nền 

	switch (message)
	{
	case WM_CREATE:
		/* Lấy thông tin font hiện thời */
		hdc = GetDC(hWnd);
		GetTextMetrics(hdc, &tm);
		ReleaseDC(hWnd, hdc);
		dwCharX = tm.tmAveCharWidth;
		dwCharY = tm.tmHeight;
		/* Cấp phát bộ nhớ đệm để lưu kí tự nhập vào */
		pchlnputBuf = (LPTSTR)GlobalAlloc(GPTR, BUFSIZE * sizeof(TCHAR));
		return 0;
	case WM_SIZE:
		/* LƯU giữ kích thước của vùng làm việc */
		dwClientX = LOWORD(lParam);
		dwClientY = HIWORD(lParam);
		/* Tính kích thước tối đa của một dòng
		và số dòng tối đa trong vùng làm việc */
		dwLineLen = dwClientX - dwCharX;
		dwLines = dwClientY / dwCharY;
		break;
	case WM_SETFOCUS:
		/* Tạo và hiển thị caret khi cửa sổ nhận được sự quan tâm */
		CreateCaret(hWnd, (HBITMAP)1, 0, dwCharY);
		SetCaretPos(nCaretPosX, nCaretPosY * dwCharY);
		ShowCaret(hWnd);
		break;
	case WM_KILLFOCUS:
		/*Án caret và hủy khi cửa sổ không còn nhận được sự quan tăm */
		HideCaret(hWnd);
		DestroyCaret();
		break;
	case WM_CHAR:
		switch (wParam)
		{
		case 0x08: // Backspace 
		case 0x0A: //Linefeed 
		case 0x1B: //Escape 
			MessageBeep((UINT)-1);
			return 0;
		case 0x09: // tab
			/* Chuyển phím tab thành 4 kí tự trắng liên tục nhau */
			for (i = 0; i < 4; i++)
				SendMessage(hWnd, WM_CHAR, 0x20, 0);
			return 0;
		case 0x0D: //Xuống dòng
		/* Lưu kí tự xuống dòng và tạo dòng mới đưa caret xuống vị trí mới */
			pchlnputBuf[cch++] = 0x0D;
			nCaretPosX = 0;
			nCaretPosY += 1;
			break;
		default: //xử lý những kí tự có thề hiển thị được
			ch = (TCHAR)wParam;
			HideCaret(hWnd);
			/* Lấy bề dày của kí tự và xuất ra */
			hdc = GetDC(hWnd);
			GetCharWidth32(hdc, (UINT)wParam, (UINT)wParam, &nCharWidth);
			TextOut(hdc, nCaretPosX, nCaretPosY*dwCharY, &ch, 1);
			ReleaseDC(hWnd, hdc);
			/* Lưu kí tự vào buffer */
			pchlnputBuf[cch++] = ch;
			/* Tính lại vị trí ngang cúa caret. Nếu vị trí này tới cuối dòng thì chèn thêm kí tự xuống dòng và di chuyển caret đến dòng mới */
			nCaretPosX += nCharWidth;
			if ((DWORD)nCaretPosX > dwLineLen)
			{
				nCaretPosX = 0;
				pchlnputBuf[cch++] = 0x0D;
				++nCaretPosY;
			}
			nCurChar = cch;
			ShowCaret(hWnd);
			break;
		}
		SetCaretPos(nCaretPosX, nCaretPosY * dwCharY);
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_LEFT: //LEFTarrow
		/* Caret di chuyển qua trái và chỉ đến đầu dòng */
			if (nCaretPosX > 0)
			{
				HideCaret(hWnd);
				/* Tính toán lại vị trí của caret khi qua trái */
				ch = pchlnputBuf[--nCurChar];
				hdc = GetDC(hWnd);
				GetCharWidth32(hdc, ch, ch, &nCharWidth);
				ReleaseDC(hWnd, hdc);
				nCaretPosX = max(nCaretPosX - nCharWidth, 0);
				ShowCaret(hWnd);
			}
			break;
		case VK_RIGHT: //RIGHT arrow 
		/* Di chuyển caret sang phải */
			if (nCurChar < cch)
			{
				HideCaret(hWnd);
				/* Tính toán lại vị trí của caret khi sang phải */
				ch = pchlnputBuf[nCurChar];
				if (ch == 0x0D)
				{
					nCaretPosX = 0;
					nCaretPosY++;
				}
				/* Nếu kí tự không phải là enter thì kiểm tra xem phím shift có được nhấn hay không. Nếu có nhấn thì đổi màu và xuất ra màn hình. */
				else
				{
					hdc = GetDC(hWnd);
					nVirtKey = GetKeyState(VK_SHIFT);
					if (nVirtKey & SHIFTED)
					{
						crPrevText = SetTextColor(hdc, RGB(255, 255, 255));
						crPrevBk = SetBkColor(hdc, RGB(0, 0, 0));
						TextOut(hdc, nCaretPosX, nCaretPosY*dwCharY,
							&ch, 1);
						SetTextColor(hdc, crPrevText);
						SetBkColor(hdc, crPrevBk);
					}
					GetCharWidth32(hdc, ch, ch, &nCharWidth);
					ReleaseDC(hWnd, hdc);
					nCaretPosX = nCaretPosX + nCharWidth;
				}
				nCurChar++;
				ShowCaret(hWnd);
				break;
			}
			break;
		case VK_UP: //Phím mũi tên lên 
		case VK_DOWN: //Phím mũi tên xuống 
			MessageBeep((UINT)-1);
			return 0;
		case VK_HOME: //Phím home
		/* Thiết lập vị trí cúa caret ớ dòng đầu tiên */
			nCaretPosX = nCaretPosY = 0;
			nCurChar = 0;
			break;
		case VK_END: //Phím end
		/* Di chuyển vè cuối văn bán và lưu vị trí của kí tự xuống dòng cuối */
			for (i = 0; i < cch; i++)
			{
				if (pchlnputBuf[i] == 0x0D)
				{
					cCR++;
					nCRIndex = i + 1;
				}
			}
			nCaretPosY = cCR;
			/* Chép tất cả các kí tự từ kí tự xuống dòng cuối đến kí tự hiện thời vừa nhập từ bàn phím vào bộ nhớ
			đệm. */
			for (i = nCRIndex, j = 0; i < cch; i++, j++)
				szBuf[j] = pchlnputBuf[i];
			szBuf[j] = TEXT('\0');
			/* Tính vị trí dọc của caret */
			hdc = GetDC(hWnd);
			GetTextExtentPoint32(hdc, szBuf, lstrlen(szBuf), &sz);
			nCaretPosX = sz.cx;
			ReleaseDC(hWnd, hdc);
			nCurChar = cch;
			break;
		default:
			break;
		}
		SetCaretPos(nCaretPosX, nCaretPosY*dwCharY);
		break;
	case WM_PAINT:
		if (cch == 0)
			break;
		hdc = BeginPaint(hWnd, &ps);
		HideCaret(hWnd);
		SetRect(&rc, 0, 0, dwLineLen, dwClientY);
		DrawText(hdc, pchlnputBuf, -1, &rc, DT_LEFT);
		ShowCaret(hWnd);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		/* Giải phóng buffer */
		GlobalFree((HGLOBAL)pchlnputBuf);
		UnregisterHotKey(hWnd, 0xAAAA);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return NULL;
}
