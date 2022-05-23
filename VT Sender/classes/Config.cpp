#include "Config.h"

#include <nlohmann/json.hpp>

#include <string>
#include <fstream>
#include <filesystem>

using nlohmann::json;

namespace VTSender
{
	Config::Config(std::filesystem::path filepath) : filepath_m{ filepath.string() }
	{
		if (!std::filesystem::exists(filepath))
		{
			std::filesystem::create_directories(std::filesystem::path(filepath).parent_path());
			std::ofstream file(filepath_m, std::ios::out);
		}
	}
	json Config::load(json data)
	{
		if (std::filesystem::exists(filepath_m))
		{
			std::ifstream file(filepath_m, std::ios::in);
			if (file.good())
			{
				json parsedData = json::parse(file, nullptr, false);

				if (parsedData != nullptr && !parsedData.is_discarded() && !parsedData.empty())
				{
					file.close();
					return parsedData;
				}
			}
		}
		return data;
	}
	bool Config::save(json& data)
	{
		if (data != nullptr)
		{
			std::ofstream file(filepath_m, std::ios::out);
			if (file.good())
			{
				file << data.dump(4);
				file.close();
				return true;
			}
		}
		return false;
	}
} // namespace VTSender