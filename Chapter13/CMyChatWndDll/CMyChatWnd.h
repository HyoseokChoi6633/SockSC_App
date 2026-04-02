#pragma once

#ifndef CMYCHATWND_H
#define CMYCHATWND_H
#include "pch.h"

#include "CMyDllExport.h"
#include "CMyChatMsgMan.h"
#include "SS_Legacing.h"

#endif      // CMYCHATWND_H

// Chat Window 생성 클래스

namespace CMyChatWnd_Library
{
    class CMYCHATWND_API CMyChatWnd
    {
    public:
        CMyChatWnd();
        ~CMyChatWnd();

        // Chat 윈도우 등록 및 생성
        bool Create(HWND hParent, HINSTANCE hInst, int iX, int iY, int iWidth, int iHeight, CMyChatMsgMan_Library::CMyChatMsgMan* pManager);

        // 메시지 매니저 설정
        void SetMessageManager(CMyChatMsgMan_Library::CMyChatMsgMan* pManager);

        // 생성된 Chat Window 핸들 반환
        HWND GetChatWnd();

        // 현재 설정된 폰트의 높이 반환
        int GetFontHeight();

        // 윈도우 프로시저 (static으로 선언하여 WinAPI 콜백으로 사용)
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

        const int CHAT_ID = 50002;      // Chat Window 의 아이디 상수

    private:
        HWND m_hWnd;
        CMyChatMsgMan_Library::CMyChatMsgMan* m_pMsgManager;   // 메시지 매니저 포인터
        ATOM s_atomClass;               // 윈도우 클래스 등록 여부
        UniqueHFont m_hFont;            // 폰트 핸들을 저장할 멤버 변수

        // 내부 메시지 처리 함수
        LRESULT OnPaint(HWND hWnd);     // WM_PAINT 처리(Chat Window에서 메시지 그리기 처리)
        // 필요하다면 다른 메시지 처리 함수 추가

        LRESULT OnVScroll(HWND hWnd, WPARAM wParam);        // WM_VSCROLL 처리

        LRESULT OnCreate(HWND hWnd, CMyChatWnd* pThis);     // WM_CREATE 처리(Chat Window 생성시 발생)

        LRESULT OnSetFont(HWND hWnd, CMyChatWnd* pThis, WPARAM wParam, LPARAM lParam);      // Chat Window 에서 설정할 폰트 처리 
        LRESULT OnGetFont(CMyChatWnd* pThis);               // Chat Window 에 설정된 폰트 반환

        LRESULT OnMouseWheel(HWND hWnd, CMyChatWnd* pThis, WPARAM wParam);      // 마우스 휠 처리
    };
}
