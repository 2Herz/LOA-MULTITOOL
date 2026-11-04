module LostArkMultiTool.Views.PlaceholderView;

import std;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr float TITLE_LINE_HEIGHT{ 40.0f };         // 제목 한 줄 높이 (DIP)
        constexpr float DESCRIPTION_LINE_HEIGHT{ 24.0f };   // 설명 한 줄 높이 (DIP)
        constexpr float LINE_GAP{ 8.0f };                   // 제목과 설명 사이 간격 (DIP)
        constexpr float CENTER_RATIO{ 0.5f };               // 세로 가운데 배치

        constexpr Core::TextStyle TITLE_STYLE{ Theme::FONT_SIZE_TITLE, Core::FontWeight::BOLD, Core::TextAlign::CENTER };
        constexpr Core::TextStyle DESCRIPTION_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::CENTER };
    }

    void DrawPlaceholder(Core::Renderer& renderer, const Core::RectF& contentArea, std::wstring_view title, std::wstring_view description)
    {
        // 제목 + 간격 + 설명 묶음을 내용 영역 세로 가운데에 둠
        const float blockHeight{ TITLE_LINE_HEIGHT + LINE_GAP + DESCRIPTION_LINE_HEIGHT };
        const float blockTop{ contentArea.m_top + (contentArea.GetHeight() - blockHeight) * CENTER_RATIO };

        const Core::RectF titleRect{ contentArea.m_left, blockTop, contentArea.m_right, blockTop + TITLE_LINE_HEIGHT };
        const float descriptionTop{ titleRect.m_bottom + LINE_GAP };
        const Core::RectF descriptionRect{ contentArea.m_left, descriptionTop, contentArea.m_right, descriptionTop + DESCRIPTION_LINE_HEIGHT };

        renderer.DrawString(title, titleRect, Theme::HEADER_TEXT_COLOR, TITLE_STYLE);
        renderer.DrawString(description, descriptionRect, Theme::MUTED_TEXT_COLOR, DESCRIPTION_STYLE);
    }
}