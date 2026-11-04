module LostArkMultiTool.Core.APPExceptions;

import std;

namespace LostArkMultiTool::Core
{
	namespace
	{
		constexpr std::string_view API_KEY_INVALID_MESSAGE{ "API 키가 유효하지 않아요!, 키를 확인해 주세요!" };
		constexpr std::string_view API_RATE_LIMIT_MESSAGE{ "API 호출 한도를 초과했어요!, 잠시 후 다시 시도해 주세요!" };
		constexpr std::string_view API_MAINTENANCE_MESSAGE{ "로스트아크 API 서버 점검 중이에요!, 점검이 끝난 뒤 다시 시도해 주세요!" };
		constexpr std::string_view REQUEST_CANCELED_MESSAGE{ "요청이 중단됐어요!" };
		constexpr int SERVER_ERROR_MIN{ 500 };	//	5xx 시작
	}

	APIKeyInvalidException::APIKeyInvalidException()
		: std::runtime_error{ std::string{ API_KEY_INVALID_MESSAGE } }
	{
	}

	APIRateLimitException::APIRateLimitException()
		: std::runtime_error{ std::string{ API_RATE_LIMIT_MESSAGE } }
	{
	}

	APIMaintenanceException::APIMaintenanceException()
		: std::runtime_error{ std::string{ API_MAINTENANCE_MESSAGE } }
	{
	}

	APIRequestException::APIRequestException(int statusCode, const std::string& message)
		: std::runtime_error{ message }
		, m_statusCode{ statusCode }
	{
	}

	int APIRequestException::GetStatusCode() const noexcept
	{
		return m_statusCode;
	}

	bool APIRequestException::IsServerError() const noexcept
	{
		return m_statusCode >= SERVER_ERROR_MIN;
	}

	RequestCanceledException::RequestCanceledException()
		: std::runtime_error{ std::string{ REQUEST_CANCELED_MESSAGE } }
	{
	}
}