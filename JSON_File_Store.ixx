export module LostArkMultiTool.Services.JSONFileStore;

import std;
import LostArkMultiTool.Services.JSONValue;

export namespace LostArkMultiTool::Services
{
    // 저장 파일 읽기 결과
    struct JSONLoadResult
    {
        std::optional<std::string> m_warning;   // 손상 복구 · 이전 기록 가져오기 안내 문구 (문제가 없었으면 nullopt)
        bool m_isImported{};                    // 저장 파일이 없어서 이전 위치 · 다른 프로그램의 파일을 가져옴 → 받는 쪽이 새 위치에 저장할 것
    };

    // 툴별 저장 파일 하나를 안전하게 읽고 쓰는 도우미 (툴마다 이 객체를 하나씩 가짐)
    // - 위치: 사용자 데이터 폴더 (%LOCALAPPDATA%\LostArkMultiTool) → exe 위치가 바뀌거나 쓰기 권한이 없는 폴더에 있어도 저장됨
    // - 읽기: 손상(파싱·변환 실패)이면 손상본을 따로 보관하고 백업(.bak)으로 다시 시도
    //         저장 파일이 없으면 가져올 파일 목록(importPaths)을 차례로 시도 (각 파일의 .bak까지, 가져온 원본은 건드리지 않음)
    // - 쓰기: 임시 파일(.tmp)에 먼저 쓰고 원자적으로 교체 (기존 파일은 .bak으로 보관)
    class JSONFileStore
    {
    public:
        // fileName: 사용자 데이터 폴더 기준 저장 파일 이름 (예: APPConstants::ROSTER_DATA_FILE_NAME)
        // importPaths: 저장 파일이 없을 때 처음 한 번 읽어 올 파일들 (앞에 있는 것부터 시도)
        explicit JSONFileStore(std::string_view fileName, std::vector<std::filesystem::path> importPaths = {});

        // 저장 파일(손상 시 백업, 없으면 가져올 파일)을 읽어 onLoaded에 넘김. onLoaded가 예외를 던지면 그 파일은 손상으로 처리
        [[nodiscard]] JSONLoadResult Load(const std::function<void(const JSONValue&)>& onLoaded);

        // JSON 텍스트를 임시 파일 → 교체 방식으로 저장 (폴더가 없으면 만듦)
        void Save(const std::string& jsonText);

    private:
        // 저장 파일이 없을 때: 가져올 파일을 차례로 시도 (성공하면 안내 문구, 하나도 없으면 nullopt)
        [[nodiscard]] std::optional<std::string> TryImport(const std::function<void(const JSONValue&)>& onLoaded) const;

        std::filesystem::path m_savePath;     // 경로는 생성할 때 한 번만 계산해 둠
        std::filesystem::path m_backupPath;
        std::filesystem::path m_tempPath;
        std::vector<std::filesystem::path> m_importPaths;
    };
}
