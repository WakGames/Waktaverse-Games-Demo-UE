// Fill out your copyright notice in the Description page of Project Settings.


#include "WakGamesAuth.h"
#include "SHA256Hash.h"

/** Generates a random string for the OAuth Code Verifier */
FString UWakGamesAuth::GenerateCodeVerifier()
{
    const FString chars = TEXT("abcdefghijklmnopqrstuvwxyz123456789");
    FString nonce;
    int32 nonceLength = 128;
    nonce.Reserve(nonceLength);
    int32 CharLength = chars.Len();
    for (int32 i = 0; i < nonceLength; i++) // nonce : char[128]
    {
        int32 index = FMath::RandRange(0, CharLength - 1);
        nonce.AppendChar(chars[index]);
    }

    return nonce;
}


/** Generates the Code Challenge from the Code Verifier */
FString UWakGamesAuth::GenerateCodeChallenge(const FString& CodeVerifier)
{
    FSHA256Hash Hash;
    Hash.FromString(CodeVerifier);
    if (Hash.GetHash().Len() == 64) // 64자리 해시가 생성되었는지 확인
    {
        FString Base64Hash = FBase64::Encode(Hash.GetHash());   // 32바이트 해시를 64바이트 Base64 인코딩
        Base64Hash = Base64Hash.Replace(TEXT("+"), TEXT("-")).Replace(TEXT("/"), TEXT("_")).Replace(TEXT("="), TEXT(""));   // HTTP URL-safe Base64 인코딩
        return Base64Hash;
    }

    // 해시 계산에 실패한 경우 빈 문자열 반환
    return TEXT("");
}


/** Generates a random string for CSRF protection */
FString UWakGamesAuth::GenerateCsrfState()
{
    const FString chars = TEXT("abcdefghijklmnopqrstuvwxyz123456789");
    FString nonce;
    int32 nonceLength = 16;
    nonce.Reserve(nonceLength);
    int32 CharLength = chars.Len();
    for (int32 i = 0; i < nonceLength; i++) // nonce : char[16]
    {
        int32 index = FMath::RandRange(0, CharLength - 1);
        nonce.AppendChar(chars[index]);
    }

    return nonce;
}