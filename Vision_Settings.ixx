export module LostArkMultiTool.Vision.VisionSettings;  // 화면 인식 설정 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Services.JSONFileStore;  // 안전한 JSON 파일 저장 가져오기

// 화면 인식 켜기 설정 (사용자 데이터 폴더 VisionSettings.json)
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 화면 인식 설정
    class VisionSettings
    {
    public:
        VisionSettings();   // 생성자 (저장 파일을 읽음, 없으면 꺼짐)

        [[nodiscard]] bool IsEnabled() const noexcept;   // 화면 인식을 켰는지
        void SetEnabled(bool isEnabled) noexcept;        // 켜기 · 끄기 (저장은 Save)
        void Save();                                     // 파일로 저장 (실패하면 예외)

    private:
        Services::JSONFileStore m_fileStore;   // 안전한 파일 읽기 · 쓰기
        bool m_isEnabled{};                    // 화면 인식 켜짐
    };
}
