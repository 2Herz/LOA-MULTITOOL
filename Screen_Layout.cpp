module LostArkMultiTool.Vision.ScreenLayout;  // 화면 배치 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.APPConstants;  // 파일 이름 · 경로 상수 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 픽셀 사각형 가져오기
import LostArkMultiTool.Services.JSONFileStore;  // 안전한 JSON 파일 저장 가져오기
import LostArkMultiTool.Services.JSONValue;  // JSON 트리 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        using Services::JSONType;    // JSON 값 종류
        using Services::JSONValue;   // JSON 값

        constexpr double MIN_RATIO{ 0.0 };                     // 비율 최솟값
        constexpr double MAX_RATIO{ 1.0 };                     // 비율 최댓값
        constexpr double DATA_VERSION{ 1.0 };                  // 저장 형식 버전
        constexpr std::string_view KEY_VERSION{ "Version" };   // 저장 형식 버전 키
        constexpr std::string_view KEY_LAYOUTS{ "Layouts" };   // 화면 목록 키
        constexpr std::string_view KEY_SCREEN{ "Screen" };     // 화면 이름 키
        constexpr std::string_view KEY_ANCHOR_FILE{ "AnchorFile" };       // 앵커 파일 키
        constexpr std::string_view KEY_ANCHOR_AREA{ "AnchorArea" };       // 앵커 범위 키
        constexpr std::string_view KEY_ANCHOR_SCORE{ "AnchorMinScore" };  // 앵커 점수 키
        constexpr std::string_view KEY_REGIONS{ "Regions" };   // 영역 목록 키
        constexpr std::string_view KEY_NAME{ "Name" };         // 영역 이름 키
        constexpr std::string_view KEY_RECT{ "Rect" };         // 영역 위치 키
        constexpr std::string_view KEY_LEFT{ "Left" };         // 왼쪽 비율 키
        constexpr std::string_view KEY_TOP{ "Top" };           // 위쪽 비율 키
        constexpr std::string_view KEY_WIDTH{ "Width" };       // 가로 비율 키
        constexpr std::string_view KEY_HEIGHT{ "Height" };     // 세로 비율 키

        // 키 목록 만들기 (string_view → string)
        [[nodiscard]] std::vector<std::string> MakeKeys(std::initializer_list<std::string_view> keys)
        {
            std::vector<std::string> result;                         // 결과
            result.reserve(keys.size());                             // 할당 한 번
            for (std::string_view key : keys) result.emplace_back(key);   // 복사
            return result;                                           // 키 목록
        }

        // 객체에서 필요한 멤버 읽기 (종류가 다르면 예외 → 파일 손상으로 처리됨)
        [[nodiscard]] const JSONValue& Require(const JSONValue& object, std::string_view key, JSONType type)
        {
            const JSONValue& member = object.FindMember(key);                                         // 멤버 찾기
            if (member.GetType() != type) throw std::runtime_error(std::format("ScreenLayouts: '{}' 형식 오류", key));   // 형식 검사
            return member;                                                                            // 멤버
        }

        // 비율 사각형 → JSON
        [[nodiscard]] JSONValue WriteRect(const NormalizedRect& rect)
        {
            return JSONValue::CreateObject(MakeKeys({ KEY_LEFT, KEY_TOP, KEY_WIDTH, KEY_HEIGHT }),
                { JSONValue::CreateNumber(rect.m_left), JSONValue::CreateNumber(rect.m_top),
                  JSONValue::CreateNumber(rect.m_width), JSONValue::CreateNumber(rect.m_height) });   // 네 값
        }

        // JSON → 비율 사각형
        [[nodiscard]] NormalizedRect ReadRect(const JSONValue& value)
        {
            return NormalizedRect{
                Require(value, KEY_LEFT, JSONType::NUMBER_VALUE).GetNumber(),     // 왼쪽
                Require(value, KEY_TOP, JSONType::NUMBER_VALUE).GetNumber(),      // 위쪽
                Require(value, KEY_WIDTH, JSONType::NUMBER_VALUE).GetNumber(),    // 가로
                Require(value, KEY_HEIGHT, JSONType::NUMBER_VALUE).GetNumber() }; // 세로
        }
    }

    // ===== NormalizedRect =====

    PixelRect NormalizedRect::ToPixels(int width, int height) const noexcept
    {
        const int left = static_cast<int>(std::lround(m_left * width));               // 왼쪽 픽셀
        const int top = static_cast<int>(std::lround(m_top * height));                // 위쪽 픽셀
        const int right = static_cast<int>(std::lround((m_left + m_width) * width));  // 오른쪽 픽셀
        const int bottom = static_cast<int>(std::lround((m_top + m_height) * height));// 아래쪽 픽셀
        return ClampRect(PixelRect{ left, top, right - left, bottom - top }, width, height);   // 화면 안으로
    }

    NormalizedRect NormalizedRect::FromPixels(const PixelRect& rect, int width, int height) noexcept
    {
        if (width <= 0 || height <= 0) return NormalizedRect{};                     // 화면 크기가 없으면 빈 값
        const double fullWidth = static_cast<double>(width);                        // 가로 (실수)
        const double fullHeight = static_cast<double>(height);                      // 세로 (실수)
        return NormalizedRect{ rect.m_x / fullWidth, rect.m_y / fullHeight,
            rect.m_width / fullWidth, rect.m_height / fullHeight };                  // 비율
    }

    bool NormalizedRect::IsValid() const noexcept
    {
        return m_width > MIN_RATIO && m_height > MIN_RATIO                          // 넓이가 있고
            && m_left >= MIN_RATIO && m_top >= MIN_RATIO                            // 왼쪽 위가 화면 안이고
            && m_left + m_width <= MAX_RATIO && m_top + m_height <= MAX_RATIO;      // 오른쪽 아래도 화면 안
    }

    // ===== ScreenLayout =====

    const ScreenRegion* ScreenLayout::FindRegion(std::string_view name) const noexcept
    {
        const auto found = std::ranges::find(m_regions, name, &ScreenRegion::m_name);   // 이름으로 찾기
        return found == m_regions.end() ? nullptr : &*found;                            // 없으면 nullptr
    }

    // ===== ScreenLayoutStore =====

    ScreenLayoutStore::ScreenLayoutStore()
        : m_fileStore{ Core::APPConstants::SCREEN_LAYOUT_FILE_NAME }   // 저장 파일
    {
        std::optional<std::vector<ScreenLayout>> loaded;                          // 읽은 목록 (끝까지 성공했을 때만)
        auto result = m_fileStore.Load([&loaded](const JSONValue& root)           // 파일 읽기
            {
                loaded = ReadText(Services::WriteJSONText(root));                 // 같은 해석 경로 재사용
            });
        m_loadWarning = std::move(result.m_warning);                              // 안내 문구
        if (loaded) m_layouts = std::move(*loaded);                               // 성공한 목록만 반영
    }

    const ScreenLayout* ScreenLayoutStore::Find(std::string_view screenName) const noexcept
    {
        const auto found = std::ranges::find(m_layouts, screenName, &ScreenLayout::m_screenName);   // 이름으로 찾기
        return found == m_layouts.end() ? nullptr : &*found;                                         // 없으면 nullptr
    }

    std::span<const ScreenLayout> ScreenLayoutStore::GetLayouts() const noexcept
    {
        return m_layouts;   // 전체 목록
    }

    void ScreenLayoutStore::Set(ScreenLayout layout)
    {
        const auto found = std::ranges::find(m_layouts, layout.m_screenName, &ScreenLayout::m_screenName);   // 같은 화면 찾기
        if (found != m_layouts.end()) *found = std::move(layout);   // 있으면 교체
        else m_layouts.push_back(std::move(layout));                // 없으면 추가
    }

    void ScreenLayoutStore::Save()
    {
        m_fileStore.Save(WriteText(m_layouts));   // 임시 파일 → 교체 방식 저장 (실패하면 예외)
    }

    const std::optional<std::string>& ScreenLayoutStore::GetLoadWarning() const noexcept
    {
        return m_loadWarning;   // 읽기 안내
    }

    std::filesystem::path ScreenLayoutStore::GetTemplateFolder()
    {
        return Core::APPConstants::GetUserDataFilePath(Core::APPConstants::VISION_TEMPLATE_FOLDER_NAME);   // 사용자 데이터 폴더\Vision
    }

    std::string ScreenLayoutStore::WriteText(std::span<const ScreenLayout> layouts)
    {
        std::vector<JSONValue> items;                                              // 화면 목록
        items.reserve(layouts.size());                                             // 할당 한 번
        for (const ScreenLayout& layout : layouts)                                 // 화면마다
        {
            std::vector<JSONValue> regions;                                        // 영역 목록
            regions.reserve(layout.m_regions.size());                              // 할당 한 번
            for (const ScreenRegion& region : layout.m_regions)                    // 영역마다
                regions.push_back(JSONValue::CreateObject(MakeKeys({ KEY_NAME, KEY_RECT }),
                    { JSONValue::CreateString(region.m_name), WriteRect(region.m_rect) }));   // 이름 + 위치
            items.push_back(JSONValue::CreateObject(
                MakeKeys({ KEY_SCREEN, KEY_ANCHOR_FILE, KEY_ANCHOR_AREA, KEY_ANCHOR_SCORE, KEY_REGIONS }),
                { JSONValue::CreateString(layout.m_screenName), JSONValue::CreateString(layout.m_anchorFile),
                  WriteRect(layout.m_anchorArea), JSONValue::CreateNumber(layout.m_anchorMinScore),
                  JSONValue::CreateArray(std::move(regions)) }));                  // 화면 하나
        }
        return Services::WriteJSONText(JSONValue::CreateObject(MakeKeys({ KEY_VERSION, KEY_LAYOUTS }),
            { JSONValue::CreateNumber(DATA_VERSION), JSONValue::CreateArray(std::move(items)) }));   // 전체 문서
    }

    std::vector<ScreenLayout> ScreenLayoutStore::ReadText(std::string_view text)
    {
        const JSONValue root = Services::ParseJSONText(text);                     // 해석 (문법 오류면 예외)
        std::vector<ScreenLayout> layouts;                                         // 결과
        for (const JSONValue& item : Require(root, KEY_LAYOUTS, JSONType::ARRAY_VALUE).GetItems())   // 화면마다
        {
            ScreenLayout layout;                                                   // 화면 하나
            layout.m_screenName = Require(item, KEY_SCREEN, JSONType::STRING_VALUE).GetString();          // 이름
            layout.m_anchorFile = Require(item, KEY_ANCHOR_FILE, JSONType::STRING_VALUE).GetString();     // 앵커 파일
            layout.m_anchorArea = ReadRect(Require(item, KEY_ANCHOR_AREA, JSONType::OBJECT_VALUE));      // 앵커 범위
            layout.m_anchorMinScore = Require(item, KEY_ANCHOR_SCORE, JSONType::NUMBER_VALUE).GetNumber();// 앵커 점수
            for (const JSONValue& region : Require(item, KEY_REGIONS, JSONType::ARRAY_VALUE).GetItems())  // 영역마다
                layout.m_regions.push_back(ScreenRegion{
                    Require(region, KEY_NAME, JSONType::STRING_VALUE).GetString(),                         // 이름
                    ReadRect(Require(region, KEY_RECT, JSONType::OBJECT_VALUE)) });                        // 위치
            layouts.push_back(std::move(layout));                                  // 추가
        }
        return layouts;   // 목록
    }
}
