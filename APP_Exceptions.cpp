module LostArkMultiTool.Core.APPExceptions;

import std;

namespace LostArkMultiTool::Core
{
	namespace
	{
		constexpr std::string_view API_KEY_INVALID_MESSAGE{ "API 키가 유효하지 않아요!, 키를 확인해 주세요!" };
		constexpr std::string_view API_RATE_LIMIT_MESSAGE{ "API 호출 한도를 초과했어요!, 잠시 후 다시 시도해 주세요!" };
	}

	APIKeyInvalidException::APIKeyInvalidException()
		: std::runtime_error{ std::string{ API_KEY_INVALID_MESSAGE } }
	{
	}

	APIRateLimitException::APIRateLimitException()
		: std::runtime_error{ std::string{ API_RATE_LIMIT_MESSAGE } }
	{
	}
}