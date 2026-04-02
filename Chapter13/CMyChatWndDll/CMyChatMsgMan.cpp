#include "pch.h"
#include "CMyChatMsgMan.h"

using namespace std;

namespace CMyChatMsgMan_Library
{
    CMyChatMsgMan::CMyChatMsgMan() : m_hWndChat(nullptr)
    {
    }

    CMyChatMsgMan::~CMyChatMsgMan()
    {
    }

    const vector<unique_ptr<CMyChatMsgMan::stChatMessage>>& CMyChatMsgMan::GetMessages() const
    {
        return m_vecMsgs;
    }

    void CMyChatMsgMan::AddMessage(tstring strMsg, bool bLeftAlign)
    {
        m_vecMsgs.push_back(make_unique<stChatMessage>(stChatMessage{ strMsg, bLeftAlign }));

        // 필요하다면 메시지 개수 제한 로직 추가
        if (m_vecMsgs.size() > MAX_CHAT_MESSAGES) {
            m_vecMsgs.erase(m_vecMsgs.begin()); // 가장 오래된 메시지 삭제
        }

        if (m_hWndChat) {
            InvalidateRect(m_hWndChat, nullptr, true);
            SendMessage(m_hWndChat, WM_VSCROLL, SB_BOTTOM, 0);
        }
    }

    void CMyChatMsgMan::Clear()
    {
        m_vecMsgs.clear();
    }

    void CMyChatMsgMan::SetChatWnd(HWND hWndChat)
    {
        m_hWndChat = hWndChat;
    }
}
