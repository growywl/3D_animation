#pragma once

#include <string>
#include <cctype>

static inline std::string SanitizeAnimNodeName(std::string name)
{
	if (name.empty()) return name;
	const size_t lastSep = name.find_last_of("|:/\\");
	if (lastSep != std::string::npos && lastSep + 1 < name.size())
		name = name.substr(lastSep + 1);
	// trim whitespace
	const size_t first = name.find_first_not_of(" \t\r\n");
	if (first == std::string::npos) return std::string();
	const size_t last = name.find_last_not_of(" \t\r\n");
	return name.substr(first, last - first + 1);
}

static inline std::string NormalizeAnimBoneKey(std::string name)
{
	name = SanitizeAnimNodeName(std::move(name));
	std::string out;
	out.reserve(name.size());
	for (unsigned char c : name)
	{
		if (std::isalnum(c))
			out.push_back(static_cast<char>(std::tolower(c)));
	}
	return out;
}

static inline bool AnimBoneKeyMatches(const std::string& a, const std::string& b)
{
	if (a.empty() || b.empty()) return false;
	if (a == b) return true;
	// allow common prefixes/namespaces (e.g. "mixamorigHips" vs "hips")
	auto endsWith = [](const std::string& s, const std::string& suffix) -> bool
	{
		if (suffix.size() > s.size()) return false;
		return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
	};
	if (a.size() > b.size() && endsWith(a, b)) return true;
	if (b.size() > a.size() && endsWith(b, a)) return true;
	return false;
}
