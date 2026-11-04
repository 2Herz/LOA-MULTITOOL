export module LostArkMultiTool.Core.GemPriceService;

import std;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.GemCatalog;
import LostArkMultiTool.Models.GemQuote;
import LostArkMultiTool.Models.PriceAlarm;
import LostArkMultiTool.Models.RefreshSettings;

export namespace LostArkMultiTool::Core
{
    // 보석 하나의 조회 결과 (매물이 없으면 m_price = nullopt)
    struct FetchedPrice
    {
        std::string m_gemKey;
        std::optional<std::int64_t> m_price;
    };

    // 이번 갱신에서 새로 목표가에 도달한 알림 (풍선 알림 글자용)
    struct ReachedAlarm
    {
        std::string m_displayName;   // "10레벨 겁화"
        std::int64_t m_price{};
        std::int64_t m_targetPrice{};
    };

    // 갱신 결과: 새로 도달한 알림 + 저장 실패 안내를 따로 전달 (원본 RefreshResult)
    struct RefreshResult
    {
        std::vector<ReachedAlarm> m_reachedAlarms;
        std::optional<std::string> m_saveError;   // 시세는 반영됐지만 파일 저장에 실패 (nullopt = 성공 또는 저장할 변경 없음)
    };

    // 보석 시세 조회 · 알림 판정 · 시세 기록 · 설정 (원본 IGemPriceService)
    // 스레드 규칙: FetchPrices만 백그라운드 스레드에서 호출 가능, 나머지는 화면 스레드에서만 호출
    // (원본 C#의 await 이후 UI 스레드 복귀와 같은 구조 → 내부 데이터에 잠금이 없어도 데이터 레이스가 없음)
    class GemPriceService
    {
    public:
        virtual ~GemPriceService();

        [[nodiscard]] virtual const std::map<std::string, Models::GemQuote>& GetQuotes() const noexcept = 0;   // 보석 키 → 현재 시세
        [[nodiscard]] virtual const std::vector<Models::PriceAlarm>& GetAlarms() const noexcept = 0;
        [[nodiscard]] virtual const std::optional<std::int64_t>& GetLastFullUpdated() const noexcept = 0;   // 마지막 전체 갱신 시각 (UTC 초)
        [[nodiscard]] virtual const std::optional<std::int64_t>& GetLastWatchUpdated() const noexcept = 0;  // 마지막 감시 갱신 시각 (UTC 초)

        [[nodiscard]] virtual std::vector<Models::GemSelection> GetAllTargets() const = 0;       // 전체 갱신 대상 (표의 모든 보석)
        [[nodiscard]] virtual std::vector<Models::GemSelection> GetWatchedTargets() const = 0;   // 감시 갱신 대상 (알림이 걸린 보석, 중복 제외)

        // 동시 요청 수를 제한해서 모두 조회 (데이터를 건드리지 않아 백그라운드 스레드에서 호출해도 안전)
        // 하나라도 실패하면 나머지를 멈추고 "처음 실패한 원인" 예외를 그대로 다시 던짐
        [[nodiscard]] virtual std::vector<FetchedPrice> FetchPrices(MarketAPI& api, const std::vector<Models::GemSelection>& targets) const = 0;

        // 조회 결과 반영 → 알림 판정 → (바뀐 게 있으면) 저장 (isFull: 전체 갱신이면 등락 기준을 넘김)
        [[nodiscard]] virtual RefreshResult ApplyPrices(const std::vector<FetchedPrice>& prices, bool isFull) = 0;

        // 알림 추가 / 목표가 변경 (같은 보석이면 목표가만 변경), 저장에 실패하면 되돌린 뒤 예외 전달
        virtual void SetAlarm(const Models::GemSelection& gem, std::int64_t targetPrice) = 0;
        // 알림 삭제 (저장에 실패하면 원래 위치에 되돌린 뒤 예외 전달)
        virtual void RemoveAlarm(std::string_view gemKey) = 0;

        [[nodiscard]] virtual Models::RefreshSettings GetSettings() const = 0;           // 설정 복사본
        virtual void UpdateSettings(const Models::RefreshSettings& settings) = 0;        // 설정 변경 + 저장 (실패하면 되돌린 뒤 예외 전달)

        // 날짜 키 → 그날 최저가 (기록이 없으면 빈 목록)
        [[nodiscard]] virtual const std::map<std::string, std::int64_t>& GetHistory(std::string_view gemKey) const = 0;
    };
}