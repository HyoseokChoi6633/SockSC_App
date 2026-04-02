// PrjClient.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

// 대용량 파일은 에러가 날것 입니다...
// 공부삼아 한 것으로 위의 대용량 파일 빼고는 잘 될것 이고
// 이걸 보고 실력을 쌓을려고 하시는 분이 대용량 파일 까지 잘 고쳐주시길 바람니다.

// DWORD 단위 4GB 까지의 파일은 무리 없게 전송될 것 입니다.
// 대용량은 알아서 구축해 보시길 바람니다.

#ifndef UNICODE
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include "framework.h"
#include "PrjClient.h"

#define MAX_LOADSTRING 100

#define BUFSIZE 512
#define WM_DRAWIT WM_USER+10

typedef enum en_MsgType {
    CHATING = 0,
    DRAWING,
    FILETRANS
} MsgType;

typedef enum en_CTRL_IDS {
    ID_BTN_SERVER = 1001,
    ID_BTN_CLIENT,
    ID_BTN_CLOSE,
    ID_EDIT_MSG,
    ID_BTN_MSG_SEND,
    ID_BTN_FILE_SEND,
    ID_BTN_CUR_DIR
} CTRL_IDS;

// 채팅 메시지
typedef struct tag_CHATMSG {
    BYTE buf[BUFSIZE + 1];
}CHATMSG, *LPCHATMSG;

// 선 그리기 메시지
typedef struct tag_DRAWMSG {
    POINT ptStart;
    POINT ptEnd;
}DRAWMSG, *LPDRAWMSG;

typedef struct tag_SockParams {
    HWND hWndMain;
    HWND hWndIPCtrl;
    HWND hWndEditPort;
    HWND hWndBtnServer;
    HWND hWndBtnCliient;
    HWND hWndBtnClose;
    HWND hWndEditMSG;

    // 커스텀 CMyChatWndDll 의 윈도우로 대체로 인한 주석처리
    // HWND hWndEditMultiLine;
    CMyChatMsgMan objChatMsgMan;
    CMyChatWnd objChatWnd;

    HWND hWndBtnMSGSend;
    HWND hWndBtnFileSend;
    HWND hWndDraw;
    HWND hWndPort;

    // 스마트 포인터를 사용한 UniqueSock을 사용하기 위해 주석처리
    // SOCKET listenSock;
    // SOCKET sock;
    UniqueWinsock uniqueWinsock;
    UniqueSock uniqueSocket{ INVALID_SOCKET };
    UniqueSock uniqueListenSocket{ INVALID_SOCKET };

    UniqueHandle uniquehReadEvent;
    UniqueHandle uniquehWriteEvent;
    bool bThreadEnable;
    UniqueHandle uniquehThServer;
    DWORD dwThServerID;
    UniqueHandle uniquehThClient;
    DWORD dwThClientID;
    bool bStart;
    UniqueHandle uniquehThread[2];
    DWORD dwThreadID[2];
    TCHAR szMSG[BUFSIZE/sizeof(TCHAR) + 1];
    MsgType enType;
    TCHAR szFileName[MAX_PATH];
    UniqueHandle uniquehMutexForFile;
    ofstream os;        // 파일 출력 c++ 객체
    ifstream is;        // 파일 읽기 c++ 객체
    TCHAR strProgramPath[MAX_PATH];     // 프로그램이 시작할때 폴더 위치를 저장해 주는 문자열 변수
} SockParams, * LPSockParams;

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
TCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
TCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int DrawCtrls(HWND hWnd, LPSockParams pSockParams);
void WndCtrlEnable(LPSockParams pSockParams, bool bFlag, bool bSendBtn);
int InitSockAndEvent(LPSockParams pSockParams);
void ReleaseObject(LPSockParams pSockParams, bool bFinal = true);
DWORD StartServer(LPSockParams pSockParams);
DWORD StartClient(LPSockParams pSockParams);
DWORD MSGSend(LPSockParams pSockParams);
DWORD FileSend(LPSockParams pSockParams);

void DirCompare(LPSockParams pSockParams);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.

    // 전역 문자열을 초기화합니다.
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_PRJCLIENT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PRJCLIENT));

    MSG msg;

    // 기본 메시지 루프입니다:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PRJCLIENT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_PRJCLIENT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 735, 730, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static SockParams sockParams;
    int iRetVal = 0;

    switch (message)
    {
    case WM_CREATE:
        DrawCtrls(hWnd, &sockParams);

        WndCtrlEnable(&sockParams, false, false);

        iRetVal = InitSockAndEvent(&sockParams);

        if (iRetVal != 0) {
            return iRetVal;
        }

        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case ID_BTN_SERVER:
                if (StartServer(&sockParams) == 0) {
                    WndCtrlEnable(&sockParams, true, false);
                }
                else {
                    WndCtrlEnable(&sockParams, false, false);
                }
                break;
            case ID_BTN_CLIENT:
                if (StartClient(&sockParams) == 0) {
                    WndCtrlEnable(&sockParams, true, true);
                }
                else {
                    WndCtrlEnable(&sockParams, false, false);
                }
                break;
            case ID_BTN_CLOSE:
                ReleaseObject(&sockParams, false);
                WndCtrlEnable(&sockParams, false, false);
                break;
            case ID_BTN_MSG_SEND:
                MSGSend(&sockParams);
                break;
            case ID_BTN_FILE_SEND:
                // 독자가 구현할 부분
                FileSend(&sockParams);
                break;
            case ID_BTN_CUR_DIR:
                DirCompare(&sockParams);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PaintGuard paintGuard(hWnd);

            // HDC가 유효한지 확인
            if (!paintGuard.IsValid()) {
                return 0; // 유효하지 않으면 그리지 않고 종료
            }

            HDC hDC = paintGuard;
            PPAINTSTRUCT pPS = paintGuard.GetPaintStruct();
            // TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
        }
        break;
    case WM_DESTROY:
        ReleaseObject(&sockParams);

        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

HWND CreateIPAddressFld(HWND hwndParent, int xcoord, int ycoord)
{

    INITCOMMONCONTROLSEX icex;

    // Ensure that the common control DLL is loaded. 
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_INTERNET_CLASSES;
    InitCommonControlsEx(&icex);

    // Create the IPAddress control.        
    HWND hWndIPAddressFld = CreateWindow(WC_IPADDRESS,
        _T(""),
        WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
        xcoord,
        ycoord,
        150,
        25,
        hwndParent,
        (HMENU)0,
        hInst,
        nullptr);

    return hWndIPAddressFld;
}

// 이미지 그리기 자식 윈도우 Proc 메시지 처리기
LRESULT CALLBACK PaintWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LPSockParams pSockParams = nullptr;

    LPCREATESTRUCT pCreateST = nullptr;
    HDC hDC;
    PAINTSTRUCT* pPS;
    RECT rect;
    POINT pt;

    static UniqueHdc uniquehDCMem;
    static UniqueHBitmap uniquehBitmap;
    static POINT ptPrev;
    static POINT ptNow;
    static bool bDrawing = false;
    DRAWMSG drawmsg;
    MsgType enType = DRAWING;

    UniqueHBrush uniquehBrOld;
    UniqueHPen uniquehPenOld;

    switch (message)
    {
    case WM_CREATE:
        pCreateST = (LPCREATESTRUCT)lParam;
        pSockParams = (LPSockParams)pCreateST->lpCreateParams;

        hDC = GetDC(hWnd);
        GetClientRect(hWnd, &rect);
        // 화면을 저장할 비트맵 생성
        pt.x = rect.right - rect.left;
        pt.y = rect.bottom - rect.top;
        uniquehBitmap.reset(CreateCompatibleBitmap(hDC, pt.x, pt.y));
        // 메모리 DC생성
        uniquehDCMem.reset(CreateCompatibleDC(hDC));
        ReleaseDC(hWnd, hDC);

        // 메모리 DC에 비트맵 생성
        SelectObject(uniquehDCMem.get(), uniquehBitmap.get());
        // 메모리 DC 화면을 흰색으로 칠함
        uniquehBrOld.reset((HBRUSH)SelectObject(uniquehDCMem.get(), GetStockObject(WHITE_BRUSH)));
        uniquehPenOld.reset((HPEN)SelectObject(uniquehDCMem.get(), GetStockObject(WHITE_PEN)));

        Rectangle(uniquehDCMem.get(), 0, 0, pt.x, pt.y);
        SelectObject(uniquehDCMem.get(), uniquehPenOld.get());
        SelectObject(uniquehDCMem.get(), uniquehBrOld.get());
        break;
    case WM_LBUTTONDOWN:
        ptPrev.x = LOWORD(lParam);
        ptPrev.y = HIWORD(lParam);

        bDrawing = true;
        break;
    case WM_MOUSEMOVE:
        if (bDrawing) {
            hDC = GetDC(hWnd);
            ptNow.x = LOWORD(lParam);
            ptNow.y = HIWORD(lParam);

            // 화면에 그리기
            uniquehPenOld.reset((HPEN)SelectObject(hDC, GetStockObject(BLACK_PEN)));
            MoveToEx(hDC, ptPrev.x, ptPrev.y, nullptr);
            LineTo(hDC, ptNow.x, ptNow.y);
            SelectObject(hDC, uniquehPenOld.get());

            ReleaseDC(hWnd, hDC);

            // 메모리 비트맵에 그리기
            uniquehPenOld.reset((HPEN)SelectObject(uniquehDCMem.get(), GetStockObject(BLACK_PEN)));
            MoveToEx(uniquehDCMem.get(), ptPrev.x, ptPrev.y, nullptr);
            LineTo(uniquehDCMem.get(), ptNow.x, ptNow.y);
            SelectObject(uniquehDCMem.get(), uniquehPenOld.get());

            if (pSockParams && pSockParams->bStart) {
                // 선 그리기 정보 보내기
                enType = DRAWING;
                send(pSockParams->uniqueSocket.get(), (PCHAR)&enType, sizeof(enType), 0);
                drawmsg.ptStart = ptPrev;
                drawmsg.ptEnd = ptNow;
                send(pSockParams->uniqueSocket.get(), (PCHAR)&drawmsg, sizeof(drawmsg), 0);
            }

            ptPrev = ptNow;
        }
        break;
    case WM_LBUTTONUP:
        bDrawing = false;
        break;
    case WM_DRAWIT:
        hDC = GetDC(hWnd);

        // 화면에 그리기
        uniquehPenOld.reset((HPEN)SelectObject(hDC, GetStockObject(BLACK_PEN)));
        MoveToEx(hDC, LOWORD(wParam), HIWORD(wParam), nullptr);
        LineTo(hDC, LOWORD(lParam), HIWORD(lParam));
        SelectObject(hDC, uniquehPenOld.get());

        ReleaseDC(hWnd, hDC);

        uniquehPenOld.reset((HPEN)SelectObject(uniquehDCMem.get(), GetStockObject(BLACK_PEN)));
        MoveToEx(uniquehDCMem.get(), LOWORD(wParam), HIWORD(wParam), nullptr);
        LineTo(uniquehDCMem.get(), LOWORD(lParam), HIWORD(lParam));
        SelectObject(uniquehDCMem.get(), uniquehPenOld.get());
        break;
    case WM_PAINT:
        {
            PaintGuard paintGuard(hWnd);

            // HDC가 유효한지 확인
            if (!paintGuard.IsValid()) {
                return 0; // 유효하지 않으면 그리지 않고 종료
            }

            hDC = paintGuard;
            pPS = paintGuard.GetPaintStruct();
            // TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
            // 메모리 비트맵에 저장된 그림을 화면에 복사하기
            GetClientRect(hWnd, &rect);
            BitBlt(hDC, 0, 0, rect.right - rect.left, rect.bottom - rect.top, uniquehDCMem.get(), 0, 0, SRCCOPY);
        }
        break;
    case WM_DESTROY:
        uniquehBitmap.reset();
        uniquehDCMem.reset();

        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

int DrawCtrls(HWND hWnd, LPSockParams pSockParams) {
    pSockParams->hWndMain = hWnd;

    CreateWindow(_T("Button"), _T("시작 정보"), WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 10, 700, 60, hWnd, 0, hInst, 0);

    CreateWindow(_T("static"), _T("주소:"), WS_CHILD | WS_VISIBLE, 20, 35, 50, 25, hWnd, (HMENU)-1, hInst, nullptr);

    pSockParams->hWndIPCtrl = CreateIPAddressFld(hWnd, 65, 32);

    SendMessage(pSockParams->hWndIPCtrl, IPM_SETADDRESS, 0, MAKEIPADDRESS(127, 0, 0, 1));

    CreateWindow(_T("static"), _T("포트 번호:"), WS_CHILD | WS_VISIBLE, 240, 35, 80, 25, hWnd, (HMENU)-1, hInst, nullptr);

    pSockParams->hWndEditPort = CreateWindow(_T("edit"), nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 320, 32, 60, 25, hWnd, (HMENU)0, hInst, nullptr);
    SetWindowText(pSockParams->hWndEditPort, _T("9000"));

    pSockParams->hWndBtnServer = CreateWindow(_T("button"), _T("서버"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 400, 32, 100, 25, hWnd, (HMENU)ID_BTN_SERVER, hInst, nullptr);
    pSockParams->hWndBtnCliient = CreateWindow(_T("button"), _T("클라이언트"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 510, 32, 100, 25, hWnd, (HMENU)ID_BTN_CLIENT, hInst, nullptr);
    pSockParams->hWndBtnClose = CreateWindow(_T("button"), _T("접속 종료"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 620, 32, 80, 25, hWnd, (HMENU)ID_BTN_CLOSE, hInst, nullptr);

    CreateWindow(_T("Button"), _T("채팅, 파일 전송"), WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 80, 700, 230, hWnd, 0, hInst, 0);

    CreateWindow(_T("static"), _T("메시지:"), WS_CHILD | WS_VISIBLE, 20, 105, 60, 25, hWnd, (HMENU)-1, hInst, nullptr);

    pSockParams->hWndEditMSG = CreateWindow(_T("edit"), nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 80, 102, 498, 25, hWnd, (HMENU)ID_EDIT_MSG, hInst, nullptr);

    // 커스텀 쳇 윈도우 사용으로 인한 주석처리
    // pSockParams->hWndEditMultiLine = CreateWindow(_T("edit"), nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY | WS_VSCROLL, 80, 135, 514, 160, hWnd, (HMENU)0, hInst, nullptr);
    pSockParams->objChatWnd.Create(hWnd, hInst, 80, 135, 514, 160, &pSockParams->objChatMsgMan);
    pSockParams->objChatMsgMan.SetChatWnd(pSockParams->objChatWnd.GetChatWnd());

    SendMessage(pSockParams->hWndEditMSG, EM_SETLIMITTEXT, BUFSIZE, 0);

    pSockParams->hWndBtnMSGSend = CreateWindow(_T("button"), _T("메시지 전송"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 600, 102, 100, 25, hWnd, (HMENU)ID_BTN_MSG_SEND, hInst, nullptr);
    pSockParams->hWndBtnFileSend = CreateWindow(_T("button"), _T("파일 전송"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 600, 135, 100, 25, hWnd, (HMENU)ID_BTN_FILE_SEND, hInst, nullptr);
    CreateWindow(_T("button"), _T("Current Dir"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 600, 168, 100, 25, hWnd, (HMENU)ID_BTN_CUR_DIR, hInst, nullptr);

    LPCTSTR lpctWndClsName = _T("MyDrawWndClass");
    WNDCLASS wndClsMyDraw;
    ZeroMemory(&wndClsMyDraw, sizeof(wndClsMyDraw));
    wndClsMyDraw.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndClsMyDraw.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClsMyDraw.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wndClsMyDraw.hInstance = hInst;
    wndClsMyDraw.lpszClassName = lpctWndClsName;
    wndClsMyDraw.style = CS_HREDRAW | CS_VREDRAW;
    wndClsMyDraw.lpfnWndProc = PaintWndProc;
    if (!RegisterClass(&wndClsMyDraw)) {
        return -1;
    }

    pSockParams->hWndDraw = CreateWindow(lpctWndClsName, _T("그림 그릴 윈도우"), WS_CHILD | WS_VISIBLE, 10, 320, 700, 340, hWnd, (HMENU)0, hInst, pSockParams);

    return 0;
}

void WndCtrlEnable(LPSockParams pSockParams, bool bFlag, bool bSendBtn) {
    EnableWindow(pSockParams->hWndIPCtrl, !bFlag);
    EnableWindow(pSockParams->hWndEditPort, !bFlag);

    EnableWindow(pSockParams->hWndBtnServer, !bFlag);
    EnableWindow(pSockParams->hWndBtnCliient, !bFlag);
    EnableWindow(pSockParams->hWndBtnClose, bFlag);

    EnableWindow(pSockParams->hWndEditMSG, bFlag);
    EnableWindow(pSockParams->hWndBtnMSGSend, bSendBtn);
    EnableWindow(pSockParams->hWndBtnFileSend, bSendBtn);

    return;
}

int InitSockAndEvent(LPSockParams pSockParams) {
    int iRetVal;

    // 윈속 초기화
    WSADATA wsaData;;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        // 실패
        return -1;
    }
    else {
        pSockParams->uniqueWinsock.reset(&wsaData);
    }

    pSockParams->uniquehReadEvent.reset(CreateEvent(nullptr, false, true, nullptr));
    if (pSockParams->uniquehReadEvent.get() == nullptr) {
        return -1;
    }

    pSockParams->uniquehWriteEvent.reset(CreateEvent(nullptr, false, false, nullptr));
    if (pSockParams->uniquehWriteEvent.get() == nullptr) {
        return -1;
    }

    pSockParams->uniquehMutexForFile.reset(CreateMutex(nullptr, false, _T("FileCtrlMux")));          // 파일 접속 제한 MUX

    if (pSockParams->uniquehMutexForFile.get() == nullptr) {
        return -1;
    }

    GetCurrentDirectory(MAX_PATH, pSockParams->strProgramPath);

    return 0;
}

void ReleaseObject(LPSockParams pSockParams, bool bFinal) {
    if (pSockParams->bStart) {
        pSockParams->bStart = false;
        Sleep(100);

        for (int i = 0; i < 2; i++) {
            if (pSockParams->uniquehThread[i].get()) {
                GetExitCodeThread(pSockParams->uniquehThread[i].get(), &pSockParams->dwThreadID[i]);

                if (pSockParams->uniquehThread[i].get()) {
                    pSockParams->uniquehThread[i].reset();
                }
            }
        }
    }

    if (bFinal) {
        if (pSockParams->bThreadEnable) {
            pSockParams->bThreadEnable = false;
            Sleep(100);

            if (pSockParams->uniquehThServer.get()) {
                GetExitCodeThread(pSockParams->uniquehThServer.get(), &pSockParams->dwThServerID);

                if (pSockParams->uniquehThServer.get()) {
                    pSockParams->uniquehThServer.reset();
                }
            }

            if (pSockParams->uniquehThClient.get()) {
                GetExitCodeThread(pSockParams->uniquehThClient.get(), &pSockParams->dwThClientID);

                if (pSockParams->uniquehThClient.get()) {
                    pSockParams->uniquehThClient.reset();
                }
            }
        }
    }

    pSockParams->os.close();
    pSockParams->is.close();

    // closesocket()
    if (pSockParams->uniqueSocket.get() != INVALID_SOCKET) {
        pSockParams->uniqueSocket.reset(INVALID_SOCKET);
    }

    if (pSockParams->uniqueListenSocket.get() != INVALID_SOCKET) {
        pSockParams->uniqueListenSocket.reset(INVALID_SOCKET);
    }
    
    if (bFinal) {
        pSockParams->uniquehReadEvent.reset();
        pSockParams->uniquehWriteEvent.reset();

        if (pSockParams->uniquehMutexForFile.get()) {
            pSockParams->uniquehMutexForFile.reset();
        }

        // 윈속 종료
        if (pSockParams->uniqueWinsock) {
            pSockParams->uniqueWinsock.reset();
        }
    }
    
    return;
}

// 소켓 함수 오류 출력 후 종료
void ErrQuit(HWND hWnd, LPCTSTR msg) {
    LPTSTR lpMsgBuf = nullptr;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, nullptr);

    UniqueLocalFreePtr uniqueErrMsg(lpMsgBuf);

    MessageBox(hWnd, (LPCTSTR)uniqueErrMsg.get(), msg, MB_ICONERROR);

    DestroyWindow(hWnd);

    return;
}

void AppendText(CMyChatMsgMan& refObjChatMsgMan, LPCTSTR strNew, bool isLeftAlign = true)
{
    refObjChatMsgMan.AddMessage(strNew, isLeftAlign);

    return;
}

// 소켓 함수 오류 출력
void ErrDisplay(CMyChatMsgMan& refObjChatMsgMan, LPCTSTR msg) {
    TCHAR szBuf[5120] = { 0, };
    LPTSTR lpMsgBuf = nullptr;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, nullptr);

    UniqueLocalFreePtr uniqueErrMsg(lpMsgBuf);

    wsprintf(szBuf, _T("[%s] %s"), msg, (LPCTSTR)uniqueErrMsg.get());

    refObjChatMsgMan.AddMessage(szBuf);

    return;
}

// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char* pBuf, int nLen, int nFlags) {
    int nReceived;
    char* pPtr = pBuf;
    int nLeft = nLen;

    while (nLeft > 0) {
        nReceived = recv(s, pPtr, nLeft, nFlags);
        if (nReceived == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }

        nLeft -= nReceived;
        pPtr += nReceived;

        if (nReceived == 0 || nReceived != nLen) {
            break;
        }
    }

    return (nLen - nLeft);
}

/*
unique_ptr<WCHAR[]> MultiByteToWChar(LPCSTR strChar) {
    unique_ptr<WCHAR[]> pStrUnicode = nullptr;

    int nLen = MultiByteToWideChar(CP_ACP, 0, strChar, strlen(strChar), nullptr, 0);
    pStrUnicode = make_unique<WCHAR[]>(nLen + 1);
    pStrUnicode[nLen] = 0;
    MultiByteToWideChar(CP_ACP, 0, strChar, strlen(strChar), pStrUnicode.get(), nLen);

    return pStrUnicode;
}

unique_ptr<char[]> WCharToUTF8(LPCWSTR strWChar) {
    unique_ptr<char[]> pStr = nullptr;

    int nLen = WideCharToMultiByte(CP_UTF8, 0, strWChar, wcslen(strWChar), nullptr, 0, nullptr, nullptr);
    pStr = make_unique<char[]>(nLen + 1);
    pStr[nLen] = 0;
    WideCharToMultiByte(CP_UTF8, 0, strWChar, wcslen(strWChar), pStr.get(), nLen, nullptr, nullptr);

    return pStr;
}

unique_ptr<char[]> WCharToMultiByte(LPCWSTR strWChar) {
    unique_ptr<char[]> pStr = nullptr;

    int nLen = WideCharToMultiByte(CP_ACP, 0, strWChar, wcslen(strWChar), nullptr, 0, nullptr, nullptr);
    pStr = make_unique<char[]>(nLen + 1);
    pStr[nLen] = 0;
    WideCharToMultiByte(CP_ACP, 0, strWChar, wcslen(strWChar), pStr.get(), nLen, nullptr, nullptr);

    return pStr;
}

unique_ptr<WCHAR[]> UTF8ToWChar(LPCSTR strUTF8) {
    unique_ptr<WCHAR[]> pStrUnicode = nullptr;

    int nLen = MultiByteToWideChar(CP_UTF8, 0, strUTF8, strlen(strUTF8), nullptr, 0);
    pStrUnicode = make_unique<WCHAR[]>(nLen + 1);
    pStrUnicode[nLen] = 0;
    MultiByteToWideChar(CP_UTF8, 0, strUTF8, strlen(strUTF8), pStrUnicode.get(), nLen);

    return pStrUnicode;
}
*/

int RecvFile(LPSockParams pSockParams) {
    ConvTxt convTxt;

    int iRetVal;
    DWORD dwWaitResult;

    UniqueWChar pWStr = nullptr;
    UniqueChar pStr = nullptr;

    TCHAR szBuf[5120] = { 0, };
    TCHAR szSaveFileName[MAX_PATH] = { 0, };

    BYTE Buf[BUFSIZE] = { 0, };

    DWORD nNumTotal = 0;
    // DWORD dwNumWrite;

    // LARGE_INTEGER curPtr;

    // 파일 이름 받기
    char szFileName[MAX_PATH] = { 0, };
    ZeroMemory(szFileName, MAX_PATH);
    iRetVal = recvn(pSockParams->uniqueSocket.get(), szFileName, MAX_PATH, 0);

    if (szFileName[0] == 0) {
        return 0;
    }

    if (iRetVal == SOCKET_ERROR) {
        ErrDisplay(pSockParams->objChatMsgMan, _T("recv()"));
        return -1;
    }

    pWStr = convTxt.UTF8ToWChar(szFileName);

#ifndef UNICODE
    pStr = WCharToMultiByte(pWStr.get());
    wsprintf(szBuf, _T("-> 받은 파일 이름: %s"), (LPCTSTR)pStr.get());
    AppendText(pSockParams->hWndEditMultiLine, (LPCTSTR)szBuf);

    wsprintf(szSaveFileName, _T("%s\\%s"), pSockParams->strProgramPath, (LPCTSTR)pStr.get());

    if (pStr) {
        pStr.reset();
    }
#else
    wsprintf(szBuf, _T("-> 받은 파일 이름: %s"), pWStr.get());

    AppendText(pSockParams->objChatMsgMan, szBuf);

    wsprintf(szSaveFileName, _T("%s\\%s"), pSockParams->strProgramPath, (LPCTSTR)pWStr.get());
#endif

    if (pWStr) {
        pWStr.reset();
    }

    DWORD dwBigEdianFileSize = 0;

    // 파일 크기 받기
    DWORD file_size = 0;

    iRetVal = recvn(pSockParams->uniqueSocket.get(), (PCHAR)&dwBigEdianFileSize, sizeof(dwBigEdianFileSize), 0);

    if (iRetVal == SOCKET_ERROR) {
        ErrDisplay(pSockParams->objChatMsgMan, _T("recv()"));
        return -1;
    }
    else {
        file_size = ntohl(dwBigEdianFileSize);
    }

    wsprintf(szBuf, _T("-> 받은 파일 크기: %d 바이트"), file_size);

    AppendText(pSockParams->objChatMsgMan, szBuf);

    dwWaitResult = WaitForSingleObject(
        pSockParams->uniquehMutexForFile.get(),    // handle to mutex
        INFINITE);

    switch (dwWaitResult)
    {
        // The thread got ownership of the mutex
    case WAIT_OBJECT_0:
        // 파일 열기
        pSockParams->os.open(szSaveFileName, ios::trunc | ios::binary);
        if (!pSockParams->os.is_open()) {
            ErrDisplay(pSockParams->objChatMsgMan, _T("파일 입출력 오류 Write1"));
            return -1;
        }

        // pSockParams->os.close();

        ReleaseMutex(pSockParams->uniquehMutexForFile.get());
        break;

        // The thread got ownership of an abandoned mutex
        // The database is in an indeterminate state
    case WAIT_ABANDONED:
        return -1;
    }

    // 파일 데이터 받기
    while (pSockParams->bStart) {
        DWORD nRemain = file_size - nNumTotal;
        DWORD nWant = (nRemain > BUFSIZE) ? BUFSIZE : nRemain;

        iRetVal = recvn(pSockParams->uniqueSocket.get(), (PCHAR)Buf, nWant, 0);

        if (iRetVal == SOCKET_ERROR) {
            ErrDisplay(pSockParams->objChatMsgMan, _T("recv() 오류"));
            iRetVal = -1;
            break;
        }
        else {
            dwWaitResult = WaitForSingleObject(
                pSockParams->uniquehMutexForFile.get(),    // handle to mutex
                INFINITE);

            switch (dwWaitResult)
            {
                // The thread got ownership of the mutex
            case WAIT_OBJECT_0:
                // pSockParams->os.open(szSaveFileName, ios::app | ios::binary);
                if (pSockParams->os.is_open()) {
                    if (pSockParams->os.write(reinterpret_cast<const char*>(Buf), iRetVal).bad()) {
                        ErrDisplay(pSockParams->objChatMsgMan, _T("파일 입출력 오류 Write2"));
                        iRetVal = -1;
                    }
                    // pSockParams->os.close();
                }

                ReleaseMutex(pSockParams->uniquehMutexForFile.get());
                break;

                // The thread got ownership of an abandoned mutex
                // The database is in an indeterminate state
            case WAIT_ABANDONED:
                return -1;
            }

            if (iRetVal == -1) {
                break;
            }

            nNumTotal += iRetVal;

            if (file_size <= nNumTotal) {
                iRetVal = 0;
                break;
            }
        }
    }

    pSockParams->os.close();

    // 전송 결과 출력

    if (nNumTotal == file_size) {
        wsprintf(szBuf, _T("-> 파일 전송 완료!"));
    }
    else {
        wsprintf(szBuf, _T("-> 파일 전송 실패!"));
    }

    AppendText(pSockParams->objChatMsgMan, szBuf);

    return iRetVal;
}

// 데이터 받기
DWORD WINAPI ReadThread(LPVOID pParam) {
    ConvTxt convTxt;
    LPSockParams pSockParams = (LPSockParams)pParam;
    TCHAR szBuf[1024] = { 0, };
    UniqueWChar pWStr = nullptr;
    UniqueChar pStr = nullptr;

    CHATMSG chatMSG;
    DRAWMSG drawMSG;

    MsgType enType;
    int iRetVal;

    while (pSockParams->bStart) {
        iRetVal = recvn(pSockParams->uniqueSocket.get(), (PCHAR)&enType, sizeof(enType), 0);
        if (iRetVal == 0 || iRetVal == SOCKET_ERROR) {
            pSockParams->bStart = false;
            continue;
        }

        if (enType == CHATING) {
            iRetVal = recvn(pSockParams->uniqueSocket.get(), (PCHAR)chatMSG.buf, sizeof(chatMSG.buf), 0);
            if (iRetVal == 0 || iRetVal == SOCKET_ERROR) {
                pSockParams->bStart = false;
                continue;
            }

            pWStr = convTxt.UTF8ToWChar((LPCSTR)chatMSG.buf);

#ifdef UNICODE
            wsprintf(szBuf, _T("[받은 데이터] %s"), pWStr.get());
#else
            pStr = WCharToMultiByte(pWStr.get());

            wsprintf(szBuf, _T("[받은 데이터] %s"), (LPCTSTR)pStr.get());

            if (pStr) {
                pStr.reset();
            }
#endif

            if (pWStr) {
                pWStr.reset();
            }

            AppendText(pSockParams->objChatMsgMan, szBuf);
        }
        else if (enType == DRAWING) {
            iRetVal = recvn(pSockParams->uniqueSocket.get(), (PCHAR)&drawMSG, sizeof(drawMSG), 0);
            if (iRetVal == 0 || iRetVal == SOCKET_ERROR) {
                pSockParams->bStart = false;
                continue;
            }

            SendMessage(pSockParams->hWndDraw, WM_DRAWIT, MAKEWPARAM(drawMSG.ptStart.x, drawMSG.ptStart.y), MAKELPARAM(drawMSG.ptEnd.x, drawMSG.ptEnd.y));
        }
        else if (enType == FILETRANS) {
            WndCtrlEnable(pSockParams, true, false);
            if (RecvFile(pSockParams) != 0) {
                pSockParams->bStart = false;
                continue;
            }
            WndCtrlEnable(pSockParams, true, true);
        }
    }

    if (pSockParams->uniqueSocket.get() != INVALID_SOCKET) {
        pSockParams->uniqueSocket.reset(INVALID_SOCKET);
    }

    return 0;
}



int SendFile(LPSockParams pSockParams) {
    ConvTxt convTxt;
    int iRetVal;
    DWORD dwWaitResult;

    TCHAR szMsg[1024] = { 0, };
    LPCWSTR lpwStr = nullptr;

    DWORD file_size = 0;
    DWORD dwTmpFileSize = 0;
    LARGE_INTEGER curPtr;

    // 파일 이름 보내기 (UTF-8 형식으로 변환해서 보냄)
    UniqueChar pStrUTF8 = nullptr;
    UniqueWChar pWStr = nullptr;
    LPCTSTR pStrFileNameOnly = nullptr;
    int iLen;

    char szFileName[MAX_PATH] = { 0, };

    // 파일 데이터 전송에 사용할 변수
    BYTE pBuf[BUFSIZE] = { 0, };
    DWORD dwNumRead = 0;
    DWORD nNumTotal = 0;

    int iTTL = 10;

    if (pSockParams->szFileName[0] == 0) {
        return 0;
    }

    dwWaitResult = WaitForSingleObject(
        pSockParams->uniquehMutexForFile.get(),    // handle to mutex
        INFINITE);

    switch (dwWaitResult)
    {
        // The thread got ownership of the mutex
    case WAIT_OBJECT_0:
        // 파일 열기
        pSockParams->is.open(pSockParams->szFileName, ios::binary);

        if (!pSockParams->is.is_open()) {
            ErrDisplay(pSockParams->objChatMsgMan, _T("파일 입출력 오류 Read1"));

            return -1;
        }

        pSockParams->is.close();

        ReleaseMutex(pSockParams->uniquehMutexForFile.get());
        break;

        // The thread got ownership of an abandoned mutex
        // The database is in an indeterminate state
    case WAIT_ABANDONED:
        return -1;
    }

    // 메시지 타입 보내기
    iRetVal = send(pSockParams->uniqueSocket.get(), (PCHAR)&pSockParams->enType, sizeof(pSockParams->enType), 0);
    if (iRetVal == SOCKET_ERROR) {
        pSockParams->bStart = false;
        return -1;
    }

    ZeroMemory(szFileName, sizeof(szFileName));

    pStrFileNameOnly = _tcsrchr(pSockParams->szFileName, _T('\\'));

#ifndef UNICODE
    pWStr = MultiByteToWChar((LPCSTR)pStrFileNameOnly + 1);
    lpwStr = pWStr.get();
#else
    lpwStr = pStrFileNameOnly + 1;
#endif

    pStrUTF8 = convTxt.WCharToUTF8(lpwStr);

#ifndef UNICODE
    if (pWStr) {
        pWStr.reset();
    }
#endif

    strcpy_s(szFileName, MAX_PATH, pStrUTF8.get());

    szFileName[strlen(szFileName)] = 0;

    if (pStrUTF8) {
        pStrUTF8.reset();
    }

    iRetVal = send(pSockParams->uniqueSocket.get(), szFileName, MAX_PATH, 0);

    if (iRetVal == SOCKET_ERROR) {
        ErrDisplay(pSockParams->objChatMsgMan, _T("send() FileName"));

        return -1;
    }

    pSockParams->is.open(pSockParams->szFileName, ios::binary);

    if (!pSockParams->is.is_open()) {
        ErrDisplay(pSockParams->objChatMsgMan, _T("파일 입출력 오류 Read1"));
        return -1;
    }
    else {
        pSockParams->is.seekg(0, pSockParams->is.end);          // 파일 끝으로
        file_size = pSockParams->is.tellg();
        pSockParams->is.seekg(0, pSockParams->is.beg);          // 파일 처음으로
    }

    //dwWaitResult = WaitForSingleObject(
    //    pSockParams->uniquehMutexForFile.get(),    // handle to mutex
    //    INFINITE);

    //switch (dwWaitResult)
    //{
    //    // The thread got ownership of the mutex
    //case WAIT_OBJECT_0:
    //    // 파일 열기
    //    // pSockParams->is.open(pSockParams->szFileName, ios::binary);

    //    if (!pSockParams->is.is_open()) {
    //        ErrDisplay(pSockParams->objChatMsgMan, _T("파일 입출력 오류 Read2"));
    //        return -1;
    //    }

    //    // 파일 크기 얻기
    //    pSockParams->is.seekg(0, pSockParams->is.end);
    //    file_size = pSockParams->is.tellg();

    //    pSockParams->is.close();

    //    ReleaseMutex(pSockParams->uniquehMutexForFile.get());
    //    break;

    //    // The thread got ownership of an abandoned mutex
    //    // The database is in an indeterminate state
    //case WAIT_ABANDONED:
    //    return -1;
    //}

    DWORD dwBigEdianFileSize = htonl(file_size);

    // 파일 크기 보내기
    iRetVal = send(pSockParams->uniqueSocket.get(), (char*)&dwBigEdianFileSize, sizeof(dwBigEdianFileSize), 0);
    if (iRetVal == SOCKET_ERROR) {
        ErrDisplay(pSockParams->objChatMsgMan, _T("send() FileSize"));

        return -1;
    }

    nNumTotal = 0;

    // 파일 데이터 보내기
    while (pSockParams->bStart) {
        DWORD nRemain = file_size - nNumTotal;
        DWORD nWant = (nRemain > BUFSIZE) ? BUFSIZE : nRemain;

        dwNumRead = 0;
        dwWaitResult = WaitForSingleObject(
            pSockParams->uniquehMutexForFile.get(),    // handle to mutex
            INFINITE);

        switch (dwWaitResult)
        {
            // The thread got ownership of the mutex
        case WAIT_OBJECT_0:
            // 파일 열기
            // pSockParams->is.open(pSockParams->szFileName, ios::binary);

            if (!pSockParams->is.is_open()) {
                ErrDisplay(pSockParams->objChatMsgMan, _T("파일 입출력 오류 Read3"));

                iRetVal = -1;
            }
            else {
                pSockParams->is.read((PCHAR)pBuf, nWant);
                dwNumRead = pSockParams->is.gcount();
            }

            ReleaseMutex(pSockParams->uniquehMutexForFile.get());
            break;

            // The thread got ownership of an abandoned mutex
            // The database is in an indeterminate state
        case WAIT_ABANDONED:
            iRetVal = -1;
            break;;
        }

        if (iRetVal == -1) {
            break;
        }

        if (dwNumRead > 0) {
            iRetVal = send(pSockParams->uniqueSocket.get(), (char*)pBuf, dwNumRead, 0);

            if (iRetVal == SOCKET_ERROR) {
                ErrDisplay(pSockParams->objChatMsgMan, _T("send() File"));
                break;
            }

            nNumTotal += dwNumRead;
        }
        else if (dwNumRead == 0 && nNumTotal >= file_size) {
            wsprintf(szMsg, _T("전송 파일: %s"), pSockParams->szFileName);
            AppendText(pSockParams->objChatMsgMan, szMsg, false);

            wsprintf(szMsg, _T("파일 전송 완료!: %d 바이트"), nNumTotal);
            AppendText(pSockParams->objChatMsgMan, szMsg, false);
            iRetVal = 0;
            break;
        }
        else {      // 파일을 끝까지 전송 못 받음...
            ErrDisplay(pSockParams->objChatMsgMan, _T("파일 입출력 오류 Read4"));
            iRetVal = -1;
            break;
        }

        if (iRetVal == -1) {
            break;
        }
    }

    pSockParams->is.close();

    return iRetVal;
}

// 데이터 보내기
DWORD WINAPI WriteThread(LPVOID pParam) {
    ConvTxt convTxt;

    LPSockParams pSockParams = (LPSockParams)pParam;

    UniqueChar pStrUTF8 = nullptr;
    UniqueWChar pWStr = nullptr;
    UniqueChar pStr = nullptr;

    LPWSTR lpwStr = nullptr;

    CHATMSG chatMSG;

    TCHAR szBuf[1024] = { 0, };

    int nTxtLen;

    int iRetVal;

    // 서버와 데이터 통신
    while (pSockParams->bStart) {
        // 쓰기 완료를 기다림
        WaitForSingleObject(pSockParams->uniquehWriteEvent.get(), INFINITE);

        if (pSockParams->enType == CHATING) {
            // 문자열 길이가 0 이면 보내지 않음
            if (_tcslen(pSockParams->szMSG) == 0) {
                // 보내기 버튼 활성화
                WndCtrlEnable(pSockParams, true, true);
                // 읽기 완료를 알림
                SetEvent(pSockParams->uniquehReadEvent.get());

                continue;
            }

            // 데이터 UTF-8로 변환(보낼때는 UTF-8로 보낸다.)
#ifndef UNICODE
            pWStr = MultiByteToWChar((LPCSTR)pSockParams->szMSG);
            lpwStr = pWStr.get();
#else
            lpwStr = pSockParams->szMSG;
#endif

            pStrUTF8 = convTxt.WCharToUTF8(lpwStr);

#ifndef UNICODE
            if (pWStr) {
                pWStr.reset();
            }
#endif

            if (strlen(pStrUTF8.get()) > BUFSIZE) {
                pStrUTF8[BUFSIZE] = 0;
            }

            strcpy_s((PCHAR)chatMSG.buf, BUFSIZE, pStrUTF8.get());

            // 데이터 보내기
            iRetVal = send(pSockParams->uniqueSocket.get(), (PCHAR)&pSockParams->enType, sizeof(pSockParams->enType), 0);
            if (iRetVal == SOCKET_ERROR) {
                pSockParams->bStart = false;
                continue;
            }

            iRetVal = send(pSockParams->uniqueSocket.get(), (PCHAR)chatMSG.buf, sizeof(chatMSG.buf), 0);
            if (iRetVal == SOCKET_ERROR) {
                pSockParams->bStart = false;
                continue;
            }

            wsprintf(szBuf, _T("[보낸 메시지] %s"), pSockParams->szMSG, false);

            AppendText(pSockParams->objChatMsgMan, szBuf, false);
        }
        else if (pSockParams->enType == FILETRANS) {
            WndCtrlEnable(pSockParams, true, false);
            if (SendFile(pSockParams) != 0) {
                pSockParams->bStart = false;
                continue;
            }
            WndCtrlEnable(pSockParams, true, true);
        }

        // 보내기 버튼 활성화
        WndCtrlEnable(pSockParams, true, true);

        // 읽기 완료를 알림
        SetEvent(pSockParams->uniquehReadEvent.get());
    }

    // 보내기 버튼 활성화
    WndCtrlEnable(pSockParams, false, false);

    if (pSockParams->uniqueSocket.get() != INVALID_SOCKET) {
        pSockParams->uniqueSocket.reset(INVALID_SOCKET);
    }

    return 0;
}

DWORD WINAPI ThfnServer(LPVOID pParam) {
    LPSockParams pSockParams = (LPSockParams)pParam;
    
    int iRetVal;

    SOCKADDR_IN clientAddr;
    int iAddrLen;

    TCHAR szBuf[1024] = { 0, };
    TCHAR szIPAddr[30] = { 0, };
    DWORD dwLen;

    LPTSTR pStrIPOnly = nullptr;

    HANDLE hThread[2] = { nullptr };

    pSockParams->bThreadEnable = true;

    while (pSockParams->bThreadEnable) {

        if (pSockParams->uniqueListenSocket.get() == INVALID_SOCKET) {
            pSockParams->bThreadEnable = false;
            continue;
        }

        // accept()
        iAddrLen = sizeof(clientAddr);
        pSockParams->uniqueSocket.reset(accept(pSockParams->uniqueListenSocket.get(), (PSOCKADDR)&clientAddr, &iAddrLen));
        if (pSockParams->uniqueSocket.get() == INVALID_SOCKET) {
            ErrDisplay(pSockParams->objChatMsgMan, _T("accept()"));
            continue;
        }

        dwLen = _countof(szIPAddr);
        WSAAddressToString((SOCKADDR*)&clientAddr, sizeof(clientAddr), nullptr, szIPAddr, &dwLen);

        pStrIPOnly = _tcsrchr(szIPAddr, _T(':'));
        if (pStrIPOnly) {
            (*pStrIPOnly) = 0;
        }

        WndCtrlEnable(pSockParams, true, true);

        wsprintf(szBuf, _T("[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트번호=%d"), szIPAddr, ntohs(clientAddr.sin_port));

        MessageBox(pSockParams->hWndMain, szBuf, szTitle, MB_ICONINFORMATION);

        // 읽기 스레드 생성
        pSockParams->uniquehThread[0].reset(CreateThread(nullptr, 0, ReadThread, pSockParams, 0, &pSockParams->dwThreadID[0]));
        // 쓰기 스레드 생성
        pSockParams->uniquehThread[1].reset(CreateThread(nullptr, 0, WriteThread, pSockParams, 0, &pSockParams->dwThreadID[1]));

        if (pSockParams->uniquehThread[0].get() == nullptr || pSockParams->uniquehThread[1] == nullptr) {
            pSockParams->bThreadEnable = false;
            MessageBox(pSockParams->hWndMain, _T("스레드를 시작할 수 없습니다.\r\n프로그램을 종료합니다."), szTitle, MB_ICONERROR);
            DestroyWindow(pSockParams->hWndMain);
            continue;
        }
        else {
            hThread[0] = pSockParams->uniquehThread[0].get();
            hThread[1] = pSockParams->uniquehThread[1].get();
        }

        pSockParams->bStart = true;

        // 스레드 종료 대기
        iRetVal = WaitForMultipleObjects(2, hThread, false, INFINITE);
        iRetVal -= WAIT_OBJECT_0;
        if (iRetVal == 0) {
            GetExitCodeThread(hThread[1], &pSockParams->dwThreadID[1]);
        }
        else {
            GetExitCodeThread(hThread[0], &pSockParams->dwThreadID[0]);
        }

        pSockParams->uniquehThread[0].reset();
        pSockParams->uniquehThread[1].reset();

        hThread[0] = nullptr;
        hThread[1] = nullptr;

        pSockParams->dwThreadID[0] = 0;
        pSockParams->dwThreadID[1] = 0;

        pSockParams->bStart = false;

        dwLen = _countof(szIPAddr);
        WSAAddressToString((SOCKADDR*)&clientAddr, sizeof(clientAddr), nullptr, szIPAddr, &dwLen);

        pStrIPOnly = _tcsrchr(szIPAddr, _T(':'));
        if (pStrIPOnly) {
            (*pStrIPOnly) = 0;
        }

        wsprintf(szBuf, _T("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트번호=%d"), szIPAddr, ntohs(clientAddr.sin_port));
        MessageBox(pSockParams->hWndMain, szBuf, szTitle, MB_ICONINFORMATION);

        WndCtrlEnable(pSockParams, true, false);
    }

    pSockParams->bStart = false;
    pSockParams->bThreadEnable = false;

    if (pSockParams->uniqueListenSocket.get() != INVALID_SOCKET) {
        pSockParams->uniqueListenSocket.reset(INVALID_SOCKET);
    }

    SetWindowText(pSockParams->hWndMain, szTitle);
    WndCtrlEnable(pSockParams, false, false);

    return 0;
}

DWORD StartServer(LPSockParams pSockParams) {
    int iRetVal;

    u_short uPort;
    TCHAR szPort[6] = { 0, };

    TCHAR szTxt[1024] = { 0, };

    GetWindowText(pSockParams->hWndEditPort, szPort, 6);
    uPort = (u_short)_ttoi(szPort);

    // socket()
    pSockParams->uniqueListenSocket.reset(socket(AF_INET, SOCK_STREAM, 0));
    if (pSockParams->uniqueListenSocket.get() == INVALID_SOCKET) {
        ErrQuit(pSockParams->hWndMain, _T("socket()"));

        return -1;
    }

    // bind()
    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(uPort);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    iRetVal = bind(pSockParams->uniqueListenSocket.get(), (PSOCKADDR)&serverAddr, sizeof(serverAddr));
    if (iRetVal == SOCKET_ERROR) {
        ErrDisplay(pSockParams->objChatMsgMan, _T("bind()"));

        return -1;
    }

    // listen()
    iRetVal = listen(pSockParams->uniqueListenSocket.get(), SOMAXCONN);
    if (iRetVal == SOCKET_ERROR) {
        ErrQuit(pSockParams->hWndMain, _T("listen()"));
        return -1;
    }

    // 서버 스레드 시작
    pSockParams->uniquehThServer.reset(CreateThread(nullptr, 0, ThfnServer, pSockParams, 0, &pSockParams->dwThServerID));
    if (pSockParams->uniquehThServer.get() == nullptr) {
        pSockParams->bThreadEnable = false;
        MessageBox(pSockParams->hWndMain, _T("서버를 시작할 수 없습니다.\r\n프로그램을 종료합니다."), szTitle, MB_ICONERROR);
        DestroyWindow(pSockParams->hWndMain);

        return -1;
    }
    else {
        wsprintf(szTxt, _T("%s %s"), szTitle, _T("[Server]"));
        SetWindowText(pSockParams->hWndMain, szTxt);
        MessageBox(pSockParams->hWndMain, _T("서버를 시작합니다."), szTitle, MB_ICONERROR);
        SetFocus(pSockParams->hWndEditMSG);
    }

    return 0;
}

DWORD WINAPI ThfnClient(LPVOID pParam) {
    LPSockParams pSockParams = (LPSockParams)pParam;

    int iRetVal;

    TCHAR szIPAddr[30] = { 0, };
    TCHAR szBuf[1024] = { 0, };
    LPTSTR pStrIPOnly = nullptr;

    HANDLE hThread[2] = { nullptr };

    DWORD dwLen;

    u_short uPort;
    TCHAR szPort[6] = { 0, };

    GetWindowText(pSockParams->hWndEditPort, szPort, 6);
    uPort = (u_short)_ttoi(szPort);

    DWORD dwIP;
    SendMessage(pSockParams->hWndIPCtrl, IPM_GETADDRESS, 0, (LPARAM)&dwIP);
    wsprintf(szIPAddr, _T("%d.%d.%d.%d"), FIRST_IPADDRESS(dwIP), SECOND_IPADDRESS(dwIP), THIRD_IPADDRESS(dwIP), FOURTH_IPADDRESS(dwIP));

    // connect()
    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(uPort);
    InetPton(AF_INET, szIPAddr, &serverAddr.sin_addr);

    iRetVal = connect(pSockParams->uniqueSocket.get(), (SOCKADDR*)&serverAddr, sizeof(serverAddr));

    if (iRetVal == SOCKET_ERROR) {
        ErrDisplay(pSockParams->objChatMsgMan, _T("connect()"));
        WndCtrlEnable(pSockParams, false, false);

        return -1;
    }

    // 읽기 스레드 생성
    pSockParams->uniquehThread[0].reset(CreateThread(nullptr, 0, ReadThread, pSockParams, 0, &pSockParams->dwThreadID[0]));
    // 쓰기 스레드 생성
    pSockParams->uniquehThread[1].reset(CreateThread(nullptr, 0, WriteThread, pSockParams, 0, &pSockParams->dwThreadID[1]));

    if (pSockParams->uniquehThread[0].get() == nullptr || pSockParams->uniquehThread[1].get() == nullptr) {
        pSockParams->bThreadEnable = false;
        MessageBox(pSockParams->hWndMain, _T("스레드를 시작할 수 없습니다.\r\n프로그램을 종료합니다."), szTitle, MB_ICONERROR);
        DestroyWindow(pSockParams->hWndMain);
        return -1;
    }
    else {
        hThread[0] = pSockParams->uniquehThread[0].get();
        hThread[1] = pSockParams->uniquehThread[1].get();
    }

    pSockParams->bStart = true;


    // 스레드 종료 대기
    iRetVal = WaitForMultipleObjects(2, hThread, false, INFINITE);
    iRetVal -= WAIT_OBJECT_0;
    if (iRetVal == 0) {
        GetExitCodeThread(pSockParams->uniquehThread[1].get(), &pSockParams->dwThreadID[1]);
    }
    else {
        GetExitCodeThread(pSockParams->uniquehThread[0].get(), &pSockParams->dwThreadID[0]);
    }

    pSockParams->uniquehThread[0].reset();
    pSockParams->uniquehThread[1].reset();

    hThread[0] = nullptr;
    hThread[1] = nullptr;

    pSockParams->dwThreadID[0] = 0;
    pSockParams->dwThreadID[1] = 0;

    pSockParams->bStart = false;

    dwLen = _countof(szIPAddr);
    WSAAddressToString((SOCKADDR*)&serverAddr, sizeof(serverAddr), nullptr, szIPAddr, &dwLen);

    pStrIPOnly = _tcsrchr(szIPAddr, _T(':'));
    if (pStrIPOnly) {
        (*pStrIPOnly) = 0;
    }

    wsprintf(szBuf, _T("[TCP 클라이언트] 서버 종료: IP 주소=%s, 포트번호=%d"), szIPAddr, ntohs(serverAddr.sin_port));
    MessageBox(pSockParams->hWndMain, szBuf, szTitle, MB_ICONINFORMATION);

    SetWindowText(pSockParams->hWndMain, szTitle);
    WndCtrlEnable(pSockParams, false, false);

    return 0;
}

DWORD StartClient(LPSockParams pSockParams) {
    int iRetVal;

    TCHAR szTxt[1024] = { 0, };

    // socket()
    pSockParams->uniqueSocket.reset(socket(AF_INET, SOCK_STREAM, 0));

    if (pSockParams->uniqueSocket.get() == INVALID_SOCKET) {
        ErrQuit(pSockParams->hWndMain, _T("socket()"));

        return -1;
    }

    // 클라이언트 스레드 시작
    pSockParams->uniquehThClient.reset(CreateThread(nullptr, 0, ThfnClient, pSockParams, 0, &pSockParams->dwThClientID));
    if (pSockParams->uniquehThClient.get() == nullptr) {
        pSockParams->bThreadEnable = false;
        MessageBox(pSockParams->hWndMain, _T("클라이언트를 시작할 수 없습니다.\r\n프로그램을 종료합니다."), szTitle, MB_ICONERROR);
        DestroyWindow(pSockParams->hWndMain);

        return -1;
    }
    else {
        wsprintf(szTxt, _T("%s %s"), szTitle, _T("[Client]"));
        SetWindowText(pSockParams->hWndMain, szTxt);

        MessageBox(pSockParams->hWndMain, _T("클라이언트를 시작합니다."), szTitle, MB_ICONERROR);
        SetFocus(pSockParams->hWndEditMSG);
    }

    return 0;
}

DWORD MSGSend(LPSockParams pSockParams) {
    int nTxtLen;

    // 읽기 완료를 기다림
    WaitForSingleObject(pSockParams->uniquehReadEvent.get(), INFINITE);
    
    GetWindowText(pSockParams->hWndEditMSG, pSockParams->szMSG, BUFSIZE / sizeof(TCHAR) + 1);
    nTxtLen = _tcslen(pSockParams->szMSG);
    if (pSockParams->szMSG[nTxtLen - 1] == _T('\n')) {
        pSockParams->szMSG[nTxtLen - 1] = _T('\0');
    }

    pSockParams->enType = CHATING;

    // 쓰기 완료를 알림
    SetEvent(pSockParams->uniquehWriteEvent.get());

    // 입력된 텍스트 전체를 선택 표시
    SendMessage(pSockParams->hWndEditMSG, EM_SETSEL, 0, -1);

    return 0;
}

int ChooseSendFile(LPSockParams pSockParams) {

    OPENFILENAME OFN;
    TCHAR filePathName[MAX_PATH] = _T("");
    TCHAR lpstrFile[MAX_PATH] = _T("");
    static TCHAR filter[] = _T("모든 파일\0*.*");

    ZeroMemory(&OFN, sizeof(OFN));
    OFN.lStructSize = sizeof(OFN);
    OFN.hwndOwner = pSockParams->hWndMain;
    OFN.lpstrFilter = filter;
    OFN.lpstrFile = pSockParams->szFileName;
    OFN.nMaxFile = MAX_PATH;
    OFN.lpstrInitialDir = _T(".");

    if (GetOpenFileName(&OFN) != 0) {
        wsprintf(filePathName, _T("%s 파일을 열겠습니까?"), OFN.lpstrFile);
        if (MessageBox(pSockParams->hWndMain, filePathName, _T("열기 선택"), MB_YESNO) == IDYES) {
            pSockParams->enType = FILETRANS;
        }
        else {
            pSockParams->szFileName[0] = 0;
        }
    }

    return 0;
}


DWORD FileSend(LPSockParams pSockParams) {
    // 읽기 완료를 기다림
    WaitForSingleObject(pSockParams->uniquehReadEvent.get(), INFINITE);

    ChooseSendFile(pSockParams);

    // 쓰기 완료를 알림
    SetEvent(pSockParams->uniquehWriteEvent.get());

    return 0;
}

void DirCompare(LPSockParams pSockParams) {
    TCHAR szTitle[1024] = { 0, };
    TCHAR szTxt[MAX_PATH * 3] = { 0, };
    TCHAR szCurDir[MAX_PATH] = { 0, };

    GetWindowText(pSockParams->hWndMain, szTitle, 1024);

    GetCurrentDirectory(MAX_PATH, szCurDir);

    wsprintf(szTxt, _T("Init Dir : %s\nNow Dir : %s"), pSockParams->strProgramPath, szCurDir);

    MessageBox(pSockParams->hWndMain, szTxt, szTitle, MB_OK);
}