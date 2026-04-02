#include "pch.h"
#include "ConvTxt.h"
// 해더 파일에 설명이 있음

namespace ConvTxt_Library {
    ConvTxt::ConvTxt()
    {
    }

    ConvTxt::~ConvTxt()
    {
    }

    // 1. MultiByte -> WCHAR 변환
    UniqueWChar ConvTxt::MultiByteToWChar(LPCSTR strChar) {
        if (!strChar) {
            return nullptr;
        }

        int nLen = MultiByteToWideChar(CP_ACP, 0, strChar, -1, NULL, 0);
        if (nLen == 0) {
            return nullptr;
        }

        UniqueWChar pStrUnicode = std::make_unique<wchar_t[]>(nLen);
        if (!MultiByteToWideChar(CP_ACP, 0, strChar, -1, pStrUnicode.get(), nLen)) {
            return nullptr;
        }

        return pStrUnicode;
    }


    // 2. WCHAR -> UTF-8 변환
    UniqueChar ConvTxt::WCharToUTF8(LPCWSTR strWChar) {
        if (!strWChar) {
            return nullptr;
        }

        int nLen = WideCharToMultiByte(CP_UTF8, 0, strWChar, -1, NULL, 0, NULL, NULL);

        if (nLen == 0) {
            return nullptr;
        }

        UniqueChar pStr = std::make_unique<char[]>(nLen);
        if (!WideCharToMultiByte(CP_UTF8, 0, strWChar, -1, pStr.get(), nLen, NULL, NULL)) {
            return nullptr;
        }

        return pStr;
    }


    // 3. WCHAR -> MultiByte 변환
    UniqueChar ConvTxt::WCharToMultiByte(LPCWSTR strWChar) {
        if (!strWChar) {
            return nullptr;
        }


        int nLen = WideCharToMultiByte(CP_ACP, 0, strWChar, -1, NULL, 0, NULL, NULL);

        if (nLen == 0) {
            return nullptr;
        }

        UniqueChar pStr = std::make_unique<char[]>(nLen);
        if (!WideCharToMultiByte(CP_ACP, 0, strWChar, -1, pStr.get(), nLen, NULL, NULL)) {
            return nullptr;
        }

        return pStr;
    }


    // 4. UTF-8 -> WCHAR 변환
    UniqueWChar ConvTxt::UTF8ToWChar(LPCSTR strUTF8) {
        if (!strUTF8) {
            return nullptr;
        }

        int nLen = MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, NULL, 0);

        if (nLen == 0) {
            return nullptr;
        }

        UniqueWChar pStrUnicode = std::make_unique<wchar_t[]>(nLen);
        if (!MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, pStrUnicode.get(), nLen)) {
            return nullptr;
        }

        return pStrUnicode;
    }

    // 유니코드 에디안 변환
    bool ConvTxt::SwitchEdian(LPWSTR strWChar)
    {
        int i;
        WCHAR wchTmp = 0;

        if (!strWChar) {
            return false;
        }

        int iStrLen = lstrlenW(strWChar);

        // 3. 빈 문자열 처리
        if (iStrLen == 0) {
            return true; // 빈 문자열은 스왑할 것이 없으므로 성공으로 간주
        }

        // UNICODE 바이트들을 엔디안 스왑
        for (i = 0; i < iStrLen; ++i)
        {
            // 바이트 스왑: 0xAABB -> 0xBBAA
            wchTmp = strWChar[i];
            strWChar[i] = (wchTmp << 8) | (wchTmp >> 8);
        }

        return true;
    }

    enEndian ConvTxt::GetSystemEndianness() {
        enEndian enEdianCurrent;
        // 4바이트 정수 (uint32_t)를 예시로 사용
        uint32_t testValue = 0x01020304; // 1:MSB, 4:LSB

        // 이 정수를 1바이트 배열의 포인터로 캐스팅하여 메모리의 첫 번째 바이트 확인
        // 즉, testValue가 메모리에 어떻게 저장되어 있는지 첫 바이트를 통해 확인
        uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&testValue);

        // 첫 번째 바이트가 0x04 (LSB)라면 리틀 엔디안
        if (bytePtr[0] == 0x04) {
            enEdianCurrent = enEndian::LittleEndian;
        }
        // 첫 번째 바이트가 0x01 (MSB)라면 빅 엔디안
        else if (bytePtr[0] == 0x01) {
            enEdianCurrent = enEndian::BigEndian;
        }
        // 그 외의 경우 (매우 드물겠지만)
        else {
            enEdianCurrent = enEndian::Unknown;
        }

        return enEdianCurrent;
    }
}
    
