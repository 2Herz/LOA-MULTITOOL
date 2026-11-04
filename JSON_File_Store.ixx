export module LostArkMultiTool.Services.JSONFileStore;

import std;
import LostArkMultiTool.Services.JSONValue;

export namespace LostArkMultiTool::Services
{
    // 툴별 저장 파일 하나를 안전하게 읽고 쓰는 도우미 (툴마다 이 객체를 하나씩 가짐)
    // - 읽기: 손상(파싱·변환 실패)이면 손상본을 따로 보관하고 백업(.bak)으로 다시 시도
    // - 쓰기: 임시 파일(.tmp)에 먼저 쓰고 원자적으로 교체 (기존 파일은 .bak으로 보관)
    class JSONFileStore
    {
    public:
        // fileName: 실행 파일 폴더 기준 저장 파일 이름 (예: APPConstants::ROSTER_DATA_FILE_NAME)
        explicit JSONFileStore(std::string_view fileName);

        // 저장 파일(손상 시 백업)을 읽어 onLoaded에 넘김. onLoaded가 예외를 던지면 그 파일은 손상으로 처리
        // 반환: 손상 복구 안내 문구 (문제가 없었으면 nullopt)
        [[nodiscard]] std::optional<std::string> Load(const std::function<void(const JSONValue&)>& onLoaded);

        // JSON 텍스트를 임시 파일 → 교체 방식으로 저장
        void Save(const std::string& jsonText);

    private:
        std::filesystem::path m_savePath;     // 경로는 생성할 때 한 번만 계산해 둠
        std::filesystem::path m_backupPath;
        std::filesystem::path m_tempPath;
    };
}