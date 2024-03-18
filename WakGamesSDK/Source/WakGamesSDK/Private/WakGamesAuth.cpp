// Fill out your copyright notice in the Description page of Project Settings.


#include "WakGamesAuth.h"

/** Generates a random string for the OAuth Code Verifier */
static FString GenerateCodeVerifier()
{
    const FString chars = TEXT("abcdefghijklmnopqrstuvwxyz123456789");
    FString nonce;
    for (int32 i = 0; i < 128; i++) // nonce : char[128]
    {
        int32 index = FMath::RandRange(0, chars.Len() - 1);
        nonce.AppendChar(chars[index]);
    }

    return nonce;
}


/** Generates the Code Challenge from the Code Verifier */
static FString GenerateCodeChallenge(const FString& CodeVerifier)
{
    // CodeVerifier 문자열을 ANSI로 변환 (UTF-8 인코딩)
    TArray<uint8> CodeVerifierBytes;
    for (auto Char : CodeVerifier)
    {
        CodeVerifierBytes.Add((uint8)Char);
    }
    CodeVerifierBytes.Add(0);   // Null terminator 추가 --> 본 배열이 문자열 데이터임을 명시

    FSHA256Signature OutSignature;
    if (FGenericPlatformMisc::GetSHA256Signature(CodeVerifierBytes.GetData(), CodeVerifierBytes.Num() - 1, OutSignature))
    {
        FString Base64Hash = FBase64::Encode(OutSignature.Signature, sizeof(OutSignature.Signature));
        Base64Hash = Base64Hash.Replace(TEXT("+"), TEXT("-")).Replace(TEXT("/"), TEXT("_")).Replace(TEXT("="), TEXT(""));   // HTTP URL-safe Base64 인코딩
        return Base64Hash;
    }

    // 해시 계산에 실패한 경우 빈 문자열 반환
    return TEXT("");
}


/** Generates a random string for CSRF protection */
static FString GenerateCsrfState()
{
    const FString chars = TEXT("abcdefghijklmnopqrstuvwxyz123456789");
    FString nonce;
    for (int32 i = 0; i < 16; i++) // nonce : char[16]
    {
        int32 index = FMath::RandRange(0, chars.Len() - 1);
        nonce.AppendChar(chars[index]);
    }

    return nonce;
}