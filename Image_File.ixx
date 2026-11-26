export module LostArkMultiTool.Vision.ImageFile;  // 이미지 파일 읽기 · 쓰기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 이미지 값 객체 가져오기

// PNG · BMP · JPG 파일 ↔ FrameImage (윈도우 이미지 구성 요소 WIC 사용)
// 쓰는 곳: 템플릿(창 제목 · 아이콘 · 숫자 글자) 읽기, 교정용 캡처 저장, 자체 검사 샘플 읽기
export namespace LostArkMultiTool::Vision::ImageFile  // 이미지 파일 네임스페이스 내보내기 시작
{
    // 이미지 파일 읽기 → BGRA 프레임 (파일이 없거나 읽을 수 없으면 nullopt)
    [[nodiscard]] std::optional<FrameImage> Load(const std::filesystem::path& path);

    // 프레임을 PNG 파일로 저장 (폴더가 없으면 만듦, 실패하면 false)
    [[nodiscard]] bool SavePNG(const std::filesystem::path& path, const FrameImage& image);
}
