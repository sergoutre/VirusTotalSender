#ifndef IMGUI_CONFIG_H
#define IMGUI_CONFIG_H

#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace VTSender
{
	class Config
	{
	public:
		Config(std::filesystem::path filepath);

		json load(json data);
		bool save(json& data);

	private:
		std::string filepath_m;
	};
} // namespace VTSender

#endif // IMGUI_CONFIG_H

