export module LostArkMultiTool.Services.RefineCalculator;

import std;
import LostArkMultiTool.Models.RefinePlan;

// 재련 시뮬레이터 계산 (데이터를 바꾸지 않는 순수 계산 → 백그라운드 스레드에서 호출해도 안전)
// 1) 비용표: 일반 재련 1단계마다 숨결 · 책을 시도마다 최적으로 고르는 기대 비용 (장인의 기운 포함)
//            상급 재련 10단계 묶음마다 턴 종류별 숨결 · 책 조합 중 가장 싼 기대 비용
// 2) 경로: 부위마다 가능한 목표 상태(일반 · 상급 · 계승)를 모두 놓고, 목표 평균 아이템 레벨을 가장 싸게 채우는 조합을 찾음
export namespace LostArkMultiTool::Services::RefineCalculator
{
    // 재료 가격 → 비용표 (가격 · 귀속 설정이 바뀔 때만 다시 계산)
    [[nodiscard]] Models::RefineCostCatalog BuildCostCatalog(const Models::RefinePrices& prices);

    // 시작 · 목표 → 비용 최소 경로
    [[nodiscard]] Models::RefineRouteResult CalculateRoute(const Models::RefineRequest& request, const Models::RefineCostCatalog& catalog);

    // 보유 재료 반영: 경로 순서대로 걸음마다 필요한 재료를 보유분에서 먼저 쓰고, 쓴 만큼 걸음 · 부위 · 총 골드에서 뺌
    // (골드 · 귀속 재료는 원래 비용이 0이거나 골드 자체라 제외, 경로 선택은 바꾸지 않음)
    void ApplyHoldings(Models::RefineRouteResult& result, const Models::RefineCostCatalog& catalog,
        const Models::RefinePrices& prices, const Models::MaterialAmounts& holdings);
}
