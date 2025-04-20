#pragma once

#include <utility>
#include <functional> // For std::hash

struct PairHasher
{
	template <class T1, class T2>
	size_t operator()(const std::pair<T1, T2>& p) const
	{
		auto h1 = std::hash<T1>{}(p.first);
		auto h2 = std::hash<T2>{}(p.second);
		size_t seed = h1;
		// boost::hash_combine
		return h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
};