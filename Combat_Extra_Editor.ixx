export module LostArkMultiTool.Views.CombatExtraEditor;

import std;
import LostArkMultiTool.Models.CombatExtraState;
import LostArkMultiTool.Views.CombatSimulatorView;

export namespace LostArkMultiTool::Views
{
    // 목록형 카드(악세서리 · 아크 그리드 · 팔찌/스톤/각인서 · 아크 패시브/아바타)의 행 명세 + 편집 반영
    // 카드마다 "행 이름 · 편집 방식 · 읽기 · 선택지 · 쓰기"를 한 번 만들어 두고,
    // 매니저는 현재 / 목표 상태를 넘겨 화면 행을 받고(BuildRows), 사용자가 고친 값을 목표에 반영(Apply)함
    // (화면 부품을 모름 → 행 글자만 만들어 줌, 전략 패턴: 행마다 읽기/쓰기 함수를 가짐)
    class CombatExtraEditor
    {
    public:
        CombatExtraEditor();
        ~CombatExtraEditor();

        // 카드 하나의 화면 행들 (현재 / 목표 글자, 바뀐 행 표시, 선택지)
        [[nodiscard]] std::vector<CombatListRow> BuildRows(CombatCard card, const Models::ExtraState& current, const Models::ExtraState& target) const;

        // 사용자가 고친 값(숫자 글자 또는 선택한 선택지 글자)을 목표에 반영 (구분 행 · 범위 밖 행 번호면 false)
        // 범위 맞춤은 부르는 쪽에서 (CombatExtra::Clamp)
        [[nodiscard]] bool Apply(CombatCard card, std::size_t row, const std::wstring& text, Models::ExtraState& target) const;

    private:
        // 행 하나의 명세
        struct RowSpec
        {
            std::wstring m_label;
            CombatRowKind m_kind{ CombatRowKind::ROW_HEADER };
            std::function<std::wstring(const Models::ExtraState&)> m_read;                              // 표시 글자
            std::function<std::vector<std::wstring>(const Models::ExtraState&)> m_choices;             // 선택지 (현재 상태에 없는 값이면 맨 앞에 끼움)
            std::function<void(Models::ExtraState&, const std::wstring&)> m_write;                      // 입력 · 선택 반영
        };

        void BuildAccessoryRows();
        void BuildBraceletRows();
        void BuildArkGridRows();
        void BuildPassiveRows();

        std::array<std::vector<RowSpec>, static_cast<std::size_t>(CombatCard::CARD_COUNT)> m_specs;
    };
}
