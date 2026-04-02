#pragma once

#include "CMyDllExport.h"

// Chat Window 에서 메시지를 관리해 주는 클래스
namespace CMyChatMsgMan_Library
{
	class CMYCHATWND_API CMyChatMsgMan
	{
	public:
		CMyChatMsgMan();
		~CMyChatMsgMan();

		// 메시지 구조체
		typedef struct _stChatMessage {
			tstring text; // 와이드 문자열 사용 (_TCHAR과 호환)
			// 필요에 따라 시간, 사용자 정보 등을 추가할 수 있습니다.
			bool bLeftAlign;		// 왼쪽 아니면 오른쪽 정렬을 설정한다.
		}stChatMessage, *LPstChatMessage;


		// Chat Wnd의 OnPaint() 에서 사용되는 메시지 저장 백터 반환 메서드
		const std::vector<std::unique_ptr<stChatMessage>>& GetMessages() const;

		// 메시지 입력시
		void AddMessage(tstring strMsg, bool bLeftAlign = true);

		// 메시지 전체 초기화
		void Clear();

		// Chat Window의 핸들 저장
		void SetChatWnd(HWND hWndChat);

		// 최대 메시지 개수 정의
		static const int MAX_CHAT_MESSAGES = 1000;

	private:
		std::vector<std::unique_ptr<stChatMessage>> m_vecMsgs;		// 메시지가 저장되는 vector
		HWND m_hWndChat;						// Chat Window 핸들
	};
}