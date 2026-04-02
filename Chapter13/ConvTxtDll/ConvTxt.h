#pragma once

/*
 * Creator : Choi HyoSeok (goto co[kr] Chaos)
 * Phone : 010-7121-6633
 * Copyright 2025. Chaos. all rights reserved.
 *
 * Data(Init) : 2025-0811_0933
 *
 * program subject :
 * 		1. DLL로 만든 스마트 포인터로 구현한 문자열 포멧 변경 함수 셋
 *      2. 문자열을 읽을 때 필요한 CPU 에디안 구조 파악 함수
 *      3. 반대의 에디안으로 변형시켜 줄 함수
 *
 * require Args :
 * 		1. 변경할 문자열
 *
 */

#ifndef CONVTXT_H
#define CONVTXT_H
#include "pch.h"

#include "framework.h"

// --- 엔디안 타입 열거형 ---
typedef enum class _enEndian {
    LittleEndian,
    BigEndian,
    Unknown
} enEndian;

using UniqueWChar = std::unique_ptr<wchar_t[]>;
using UniqueChar = std::unique_ptr<char[]>;

// 스마트 포인터로 문자열을 변환해 주는 함수 선언

namespace ConvTxt_Library
{
    class CONVTXT_API ConvTxt {
    public:
        ConvTxt();
        ~ConvTxt();

        // 1. MultiByte -> WCHAR 변환
        UniqueWChar MultiByteToWChar(LPCSTR strChar);

        // 2. WCHAR -> UTF-8 변환
        UniqueChar WCharToUTF8(LPCWSTR strWChar);

        // 3. WCHAR -> MultiByte 변환
        UniqueChar WCharToMultiByte(LPCWSTR strWChar);

        // 4. UTF-8 -> WCHAR 변환
        UniqueWChar UTF8ToWChar(LPCSTR strUTF8);

        // 유니코드 에디안 변환
        bool SwitchEdian(LPWSTR strWChar);

        // --- 현재 시스템의 엔디안을 판별하는 함수 ---
        enEndian GetSystemEndianness();
    };
}

#endif      // CONVTXT_H