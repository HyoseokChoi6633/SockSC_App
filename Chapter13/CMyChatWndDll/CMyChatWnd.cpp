#include "pch.h"
#include "CMyChatWnd.h"

namespace CMyChatWnd_Library
{

    CMyChatWnd::CMyChatWnd() : m_hWnd(NULL), m_pMsgManager(nullptr), m_hFont(nullptr), s_atomClass(0)
    {
    }

    CMyChatWnd::~CMyChatWnd()
    {
        // 윈도우 파괴는 WinAPI가 담당하므로 여기서는 특별히 할 일 없음
    }

    bool CMyChatWnd::Create(HWND hParent, HINSTANCE hInst, int iX, int iY, int iWidth, int iHeight, CMyChatMsgMan_Library::CMyChatMsgMan* pManager)
    {
        // 윈도우 클래스 등록 (최초 1회만)
        if (s_atomClass == 0) {
            WNDCLASSEX wc = { 0 };
            wc.cbSize = sizeof(WNDCLASSEX);
            wc.lpfnWndProc = CMyChatWnd::WndProc; // static 함수를 콜백으로
            wc.hInstance = hInst;
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // 배경색 지정
            wc.lpszClassName = _T("ChatChildWindow");
            s_atomClass = RegisterClassEx(&wc);

            if (s_atomClass == 0) {
                return false;
            }
        }

        m_hWnd = CreateWindowEx(
            0,
            _T("ChatChildWindow"),
            NULL, // Child Window는 보통 타이틀 없음
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL, // 스크롤바 추가
            iX, iY, iWidth, iHeight,
            hParent,          // 부모 윈도우 핸들
            (HMENU)CHAT_ID,      // Child Window ID (고유해야 함)
            hInst,
            this              // CREATESTRUCT의 lpCreateParams로 객체 포인터 전달
        );

        if (!m_hWnd) {
            return false;
        }

        SetMessageManager(pManager); // 메시지 매니저 설정

        return true;
    }

    void CMyChatWnd::SetMessageManager(CMyChatMsgMan_Library::CMyChatMsgMan* pManager)
    {
        m_pMsgManager = pManager;
    }

    HWND CMyChatWnd::GetChatWnd()
    {
        return m_hWnd;
    }

    int CMyChatWnd::GetFontHeight()
    {
        HFONT hFontBtn = nullptr;

        // LOGFONT 구조체 선언 및 초기화
        LOGFONT logFont;

        int iFontHeight;

        if (m_hWnd) {
            hFontBtn = (HFONT)SendMessage(m_hWnd, WM_GETFONT, 0, 0);
        }

        if (hFontBtn == nullptr) {
            NONCLIENTMETRICS ncm;
            ZeroMemory(&ncm, sizeof(NONCLIENTMETRICS));
            ncm.cbSize = sizeof(NONCLIENTMETRICS);

            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

            memcpy_s(&logFont, sizeof(logFont), &ncm.lfMessageFont, sizeof(logFont));
        }
        else {
            GetObject(hFontBtn, sizeof(logFont), &logFont);
        }

        return abs(logFont.lfHeight);
    }

    // 윈도우 프로시저 (static)
    LRESULT CALLBACK CMyChatWnd::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        CMyChatWnd* pThis = nullptr;

        UniqueHdc hDC;

        // WM_NCCREATE에서 객체 포인터를 저장
        if (message == WM_NCCREATE) {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = reinterpret_cast<CMyChatWnd*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        }
        else {
            pThis = reinterpret_cast<CMyChatWnd*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        }

        // 객체 포인터가 유효할 경우 멤버 함수로 메시지 처리 위임
        if (pThis) {
            switch (message) {
            case WM_CREATE:
                pThis->OnCreate(hWnd, pThis);
                break;
            case WM_SETFONT:
                pThis->OnSetFont(hWnd, pThis, wParam, lParam);
                break;

            case WM_GETFONT:
                // 현재 설정된 폰트 핸들을 반환
                return pThis->OnGetFont(pThis);
            case WM_PAINT:
                return pThis->OnPaint(hWnd);
            case WM_VSCROLL: // 스크롤바 메시지 처리
                // TODO: 스크롤바 이동 로직 구현 (아래에서 설명)
                pThis->OnVScroll(hWnd, wParam);
                break;
            case WM_SIZE: // 윈도우 크기 변경 시
                {
                    // TODO: 스크롤바 정보 업데이트 필요
                    // 이전에 스크롤바가 있었는지 여부를 저장할 정적 변수
                    static bool wasVScroll = false;
                    bool hasVScroll = (GetWindowLong(hWnd, GWL_STYLE) & WS_VSCROLL) != 0;

                    // 스크롤바의 상태가 변경되었을 경우에만 다시 그립니다.
                    if (hasVScroll != wasVScroll) {
                        InvalidateRect(hWnd, nullptr, true);
                        wasVScroll = hasVScroll;
                    }

                    break;
                }
            case WM_MOUSEWHEEL: // 마우스 휠 스크롤 처리
                return pThis->OnMouseWheel(hWnd, pThis, wParam);
            case WM_DESTROY:
                // 객체 소멸은 메인 윈도우에서 관리
                // 폰트 해제 (직접 생성한 폰트만 해제)
                pThis->m_hFont.reset();
                break;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    // WM_PAINT 메시지 처리 (더블 버퍼링 적용)
    LRESULT CMyChatWnd::OnPaint(HWND hWnd) {
        // PaintGuard 객체를 생성하면 BeginPaint가 자동으로 호출됩니다.
        PaintGuard paintGuard(hWnd);
        SIZE sizeFontSize;

        // HDC가 유효한지 확인
        if (!paintGuard.IsValid()) {
            return 0; // 유효하지 않으면 그리지 않고 종료
        }

        HDC hDC = paintGuard;

        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        int iWidth = rcClient.right - rcClient.left;
        int iHeight = rcClient.bottom - rcClient.top;

        // 더블 버퍼링을 위한 메모리 DC 생성
        UniqueHdc hDCMem(CreateCompatibleDC(hDC));
        UniqueHBitmap hBitmap(CreateCompatibleBitmap(hDC, iWidth, iHeight));

        GdiObjectSelector gdiBitmapSelector(hDCMem.get(), hBitmap.get());

        // 배경색으로 채우기
        FillRect(hDCMem.get(), &rcClient, (HBRUSH)(COLOR_WINDOW + 1));
        SetBkMode(hDCMem.get(), TRANSPARENT); // 텍스트 배경 투명

        // 메시지 그리기
        if (m_pMsgManager) {
            const auto& messages = m_pMsgManager->GetMessages();
            const int X_BLANK = 10;
            int yPos = 10; // 시작 Y 좌표
            int iLineHeight = (float)GetFontHeight() * 1.2; // 한 줄 높이 (폰트 크기에 따라 조절)

            // ... (메시지 그리기 로직 전)
            int iTotalContentHeight = messages.size() * iLineHeight + iLineHeight * 2;
            int iClientHeight = rcClient.bottom - rcClient.top;

            SCROLLINFO si = { sizeof(SCROLLINFO) };
            si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
            si.nMin = 0;
            si.nMax = max(0, iTotalContentHeight - 1); // 콘텐츠 전체 높이
            si.nPage = iClientHeight; // 클라이언트 영역 높이
            si.nPos = GetScrollPos(hWnd, SB_VERT); // 현재 스크롤 위치

            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
            // ... (메시지 그리기 로직)

            // 스크롤 위치 적용
            int scrollPos = GetScrollPos(hWnd, SB_VERT);
            yPos -= scrollPos; // 현재 스크롤 위치만큼 위로 올림

            GdiObjectSelector gdiFontSelector(hDCMem.get(), m_hFont.get());

            for (const auto& msg : messages) {
                // 윈도우 범위 내에 있는 메시지만 그림
                if (yPos + iLineHeight > 0 && yPos < iHeight) {
                    if (!msg->text.empty()) {
                        if (msg->bLeftAlign) {
                            TextOut(hDCMem.get(), X_BLANK, yPos, msg->text.c_str(), msg->text.length());
                        }
                        else {
                            ::GetTextExtentPoint32((HDC)hDCMem.get(), msg->text.c_str(), msg->text.size(), &sizeFontSize);
                            TextOut(hDCMem.get(), iWidth - X_BLANK - sizeFontSize.cx, yPos, msg->text.c_str(), msg->text.length());
                        }
                    }
                }
                yPos += iLineHeight;
            }

            gdiFontSelector.reset();
        }

        // 메모리 DC 내용을 화면 DC로 복사
        BitBlt(hDC, 0, 0, iWidth, iHeight, hDCMem.get(), 0, 0, SRCCOPY);

        // 리소스 해제
        gdiBitmapSelector.reset();
        hBitmap.reset();
        hDCMem.reset();

        return 0;
    }

    LRESULT CMyChatWnd::OnVScroll(HWND hWnd, WPARAM wParam)
    {
        SCROLLINFO si = { sizeof(SCROLLINFO) };
        si.fMask = SIF_ALL; // 모든 스크롤 정보 가져오기
        GetScrollInfo(hWnd, SB_VERT, &si);

        int yNewPos = si.nPos;

        int iLineHeight = (float)GetFontHeight() * 1.2; // 한 줄 높이 (폰트 크기에 따라 조절)

        switch (LOWORD(wParam)) {
        case SB_TOP:        yNewPos = si.nMin; break;
        case SB_BOTTOM:     yNewPos = si.nMax; break;
        case SB_LINEUP:     yNewPos -= iLineHeight; break; // 한 줄 위로
        case SB_LINEDOWN:   yNewPos += iLineHeight; break; // 한 줄 아래로
        case SB_PAGEUP:     yNewPos -= si.nPage; break;
        case SB_PAGEDOWN:   yNewPos += si.nPage; break;
        case SB_THUMBTRACK: // 스크롤 바를 끌 때
        case SB_THUMBPOSITION: // 스크롤 바를 놓았을 때
            yNewPos = HIWORD(wParam);
            break;
        }

        // 스크롤 범위 제한
        yNewPos = max(si.nMin, yNewPos);
        yNewPos = min(si.nMax - (int)si.nPage + 1, yNewPos); // 실제 스크롤 가능 범위

        if (yNewPos != si.nPos) {
            // 스크롤 위치 변경 및 화면 갱신
            ScrollWindowEx(hWnd, 0, si.nPos - yNewPos, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
            SetScrollPos(hWnd, SB_VERT, yNewPos, TRUE);
            UpdateWindow(hWnd); // 즉시 갱신
        }

        return 0;
    }

    LRESULT CMyChatWnd::OnCreate(HWND hWnd, CMyChatWnd* pThis)
    {
        tstring strMsg;

        // 폰트 초기화: 시스템 기본 GUI 폰트를 가져와 사용
        // 또는 CreateFont 등으로 새로운 폰트 생성 후 저장
        pThis->m_hFont.reset((HFONT)GetStockObject(DEFAULT_GUI_FONT)); // 시스템 기본 폰트

        if (m_pMsgManager && m_pMsgManager->GetMessages().size() > 0) {
            strMsg = m_pMsgManager->GetMessages()[0].get()->text;
        }
        else {
            strMsg = _T("A");
        }

        UniqueHdc hDC(
            ::GetDC(hWnd), HdcDeleter(hWnd) // GetDC() 호출 및 사용자 정의 삭제자 전달
        );

        hDC.reset();

        return 0;
    }

    LRESULT CMyChatWnd::OnSetFont(HWND hWnd, CMyChatWnd* pThis, WPARAM wParam, LPARAM lParam)
    {
        // 외부에서 폰트를 설정하는 메시지 (컨트롤처럼 동작하게 할 때 유용)
                // wParam: 설정할 폰트의 HFONT 핸들
                // lParam: 폰트 적용 후 다시 그릴지 여부 (TRUE/FALSE)
        pThis->m_hFont.reset((HFONT)wParam); // 새 폰트 핸들 저장

        if (lParam) { // lParam이 TRUE이면 즉시 갱신
            InvalidateRect(hWnd, NULL, TRUE);
        }

        return 0;
    }

    LRESULT CMyChatWnd::OnGetFont(CMyChatWnd* pThis) {
        return (LRESULT)pThis->m_hFont.get();
    }

    LRESULT CMyChatWnd::OnMouseWheel(HWND hWnd, CMyChatWnd* pThis, WPARAM wParam) {
        // 마우스 휠이 돌아간 정도를 나타내는 값
                // WHEEL_DELTA (120)의 배수로 전달됩니다.
        short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

        // 스크롤 방향 결정: 양수면 위로, 음수면 아래로 스크롤
        int scrollLines = 0;
        // 시스템 설정에 따라 한 번에 스크롤되는 라인 수를 가져옵니다.
        // SPI_GETWHEELSCROLLLINES는 0 (페이지 스크롤), 1-n (n줄 스크롤)
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &scrollLines, 0);

        // 스크롤바 정보 가져오기
        SCROLLINFO si = { sizeof(SCROLLINFO) };
        si.fMask = SIF_ALL;
        GetScrollInfo(hWnd, SB_VERT, &si);

        int yNewPos = si.nPos;

        if (scrollLines == WHEEL_PAGESCROLL) { // 한 번에 페이지 단위로 스크롤
            if (zDelta > 0) { // 위로 스크롤
                yNewPos -= si.nPage;
            }
            else { // 아래로 스크롤
                yNewPos += si.nPage;
            }
        }
        else { // 라인 단위로 스크롤
            yNewPos -= (zDelta / WHEEL_DELTA) * (scrollLines * pThis->GetFontHeight()); // m_iLineHeight는 한 줄 높이
        }

        // 스크롤 범위 제한
        yNewPos = max(si.nMin, yNewPos);
        // si.nMax는 콘텐츠의 끝을 나타내고, si.nPage는 보이는 영역의 크기이므로
        // 실제 스크롤 가능한 최대 위치는 nMax - nPage + 1 입니다.
        yNewPos = min(si.nMax - (int)si.nPage + 1, yNewPos);

        if (yNewPos != si.nPos) {
            // 스크롤 위치 변경 및 화면 갱신
            // ScrollWindowEx는 화면을 직접 이동시키고 Invalidate 영역을 만듭니다.
            ::ScrollWindowEx(hWnd, 0, si.nPos - yNewPos, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
            ::SetScrollPos(hWnd, SB_VERT, yNewPos, TRUE); // 스크롤바 위치 업데이트
            ::UpdateWindow(hWnd); // 즉시 화면 갱신 요청
        }

        return 0;
    }
}
