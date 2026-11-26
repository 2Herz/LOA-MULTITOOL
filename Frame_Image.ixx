export module LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 이미지 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기

// 화면 인식에 쓰는 이미지 값 객체
// - FrameImage: 캡처한 컬러 프레임 (BGRA 8비트, 위에서 아래로)
// - GrayImage: 템플릿 매칭 · 숫자 읽기용 흑백 이미지 (밝기 0 ~ 255)
// 게임 프로세스와는 아무 관계없이, 윈도우 화면 캡처 API가 넘긴 픽셀만 다룸
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 픽셀 단위 사각형 (왼쪽 위 기준)
    struct PixelRect
    {
        int m_x{};        // 왼쪽 X
        int m_y{};        // 위쪽 Y
        int m_width{};    // 가로 크기
        int m_height{};   // 세로 크기

        [[nodiscard]] bool IsEmpty() const noexcept;      // 넓이가 0 이하인지
        [[nodiscard]] int GetRight() const noexcept;      // 오른쪽 끝 X (포함하지 않음)
        [[nodiscard]] int GetBottom() const noexcept;     // 아래쪽 끝 Y (포함하지 않음)
        [[nodiscard]] bool operator==(const PixelRect&) const = default;   // 값 비교
    };

    // 픽셀 단위 점
    struct PixelPoint
    {
        int m_x{};   // X
        int m_y{};   // Y
        [[nodiscard]] bool operator==(const PixelPoint&) const = default;   // 값 비교
    };

    // 사각형을 이미지 크기 안으로 자름 (밖으로 나간 부분 제거, 전부 밖이면 빈 사각형)
    [[nodiscard]] PixelRect ClampRect(const PixelRect& rect, int width, int height) noexcept;

    // 흑백 이미지 (한 픽셀 = 밝기 1바이트, 줄 사이 빈칸 없음)
    class GrayImage
    {
    public:
        GrayImage() = default;                                       // 빈 이미지
        GrayImage(int width, int height, std::uint8_t fill = 0);     // 크기 지정 + 채울 밝기

        [[nodiscard]] int GetWidth() const noexcept;                 // 가로 크기
        [[nodiscard]] int GetHeight() const noexcept;                // 세로 크기
        [[nodiscard]] bool IsEmpty() const noexcept;                 // 픽셀이 없는지
        [[nodiscard]] std::uint8_t At(int x, int y) const noexcept;  // 픽셀 밝기 (범위 밖이면 0)
        void Set(int x, int y, std::uint8_t value) noexcept;         // 픽셀 밝기 설정 (범위 밖이면 무시)
        [[nodiscard]] std::span<const std::uint8_t> GetPixels() const noexcept;   // 전체 픽셀 (읽기 전용)

        [[nodiscard]] GrayImage Crop(const PixelRect& rect) const;              // 영역 잘라내기 (이미지 밖은 버림)
        [[nodiscard]] GrayImage Resize(int width, int height) const;            // 크기 바꾸기 (쌍선형 보간)
        [[nodiscard]] GrayImage Threshold(std::uint8_t level) const;            // 이진화 (level 이상 = 255, 미만 = 0)
        [[nodiscard]] GrayImage Invert() const;                                 // 밝기 뒤집기 (255 - 값)
        [[nodiscard]] std::uint8_t GetOtsuLevel() const noexcept;               // 오츠 방식 자동 이진화 기준값
        [[nodiscard]] std::optional<PixelRect> FindInkBounds() const noexcept;  // 0이 아닌 픽셀을 모두 감싸는 사각형 (없으면 nullopt)

    private:
        int m_width{};                       // 가로 크기
        int m_height{};                      // 세로 크기
        std::vector<std::uint8_t> m_pixels;  // 픽셀 (m_width * m_height)
    };

    // 컬러 프레임 (BGRA, 한 픽셀 4바이트, 줄 사이 빈칸 없음)
    class FrameImage
    {
    public:
        static constexpr int BYTES_PER_PIXEL{ 4 };   // B · G · R · A

        FrameImage() = default;                                                // 빈 프레임
        FrameImage(int width, int height, std::vector<std::uint8_t> bgra);    // 픽셀 버퍼로 만들기 (크기가 안 맞으면 예외)

        // 줄 간격(rowPitch)이 있는 원본 버퍼에서 복사해서 만들기 (GPU 매핑 메모리 · WIC 버퍼용)
        [[nodiscard]] static FrameImage FromRows(int width, int height, std::span<const std::uint8_t> source, int rowPitch);

        [[nodiscard]] int GetWidth() const noexcept;                  // 가로 크기
        [[nodiscard]] int GetHeight() const noexcept;                 // 세로 크기
        [[nodiscard]] bool IsEmpty() const noexcept;                  // 픽셀이 없는지
        [[nodiscard]] std::span<const std::uint8_t> GetPixels() const noexcept;   // 전체 픽셀 (읽기 전용)

        [[nodiscard]] FrameImage Crop(const PixelRect& rect) const;   // 영역 잘라내기 (이미지 밖은 버림)
        [[nodiscard]] GrayImage ToGray() const;                       // 흑백으로 바꾸기 (BT.601 밝기)
        [[nodiscard]] GrayImage ToGray(const PixelRect& rect) const;  // 영역만 흑백으로 (자르기 + 변환을 한 번에)

    private:
        int m_width{};                       // 가로 크기
        int m_height{};                      // 세로 크기
        std::vector<std::uint8_t> m_pixels;  // 픽셀 (m_width * m_height * 4)
    };
}
