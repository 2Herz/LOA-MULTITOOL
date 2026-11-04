export module LostArkMultiTool.Services.GistServerConfigChecker;

import std;
import LostArkMultiTool.Core.ServerConfigChecker;
import LostArkMultiTool.Models.ServerConfig;

export namespace LostArkMultiTool::Services
{
    // GitHub Gist에 올린 설정 파일(raw)을 읽는 원격 설정 확인 (점검 스위치 · 최신 버전 · 공지)
    // Gist 내용만 고치면 배포된 프로그램 전체에 점검·업데이트 안내를 켜고 끌 수 있음 (원본 ServerConfigChecker와 같은 방식)
    class GistServerConfigChecker final : public Core::ServerConfigChecker
    {
    public:
        [[nodiscard]] std::optional<Models::ServerConfig> Fetch() override;
    };
}