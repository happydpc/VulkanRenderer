#pragma once

#include <chrono>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "util/FileWatcher.h"

namespace job
{
class ThreadPool;
}
namespace Resource::Shader
{

enum class ShaderType
{
	vertex,
	tess_control,
	tess_eval,
	geometry,
	fragment,
	compute,
	error,
};

struct ShaderInfo
{
	std::string name;
	ShaderType type;
	std::vector<uint32_t> spirv_data;
};

using ShaderID = uint32_t;

class ShaderDatabase
{
	public:
	ShaderDatabase ();

	void load ();
	void save ();
	void refresh ();
	void discover ();
	std::vector<std::filesystem::path> stale_handles ();

	// void AddEntry (ShaderDatabaseHandle handle);

	struct DBHandle
	{
		std::string name;
		ShaderType type;
		ShaderID id;
	};

	private:
	std::vector<DBHandle> entries;
};

class ShaderCompiler
{
	public:
	ShaderCompiler ();
	std::optional<std::vector<uint32_t>> const compile_glsl_to_spirv (std::string const& shader_name,
	    std::string const& shader_data,
	    ShaderType const shader_type,
	    std::filesystem::path include_path = std::filesystem::path{});

	std::optional<std::string> load_file_data (std::string const& filename);
};


class Shaders
{
	public:
	Shaders (job::ThreadPool& thread_pool);

	ShaderID add_shader (std::string name, std::string path);

	std::vector<uint32_t> get_spirv_data (ShaderID id);
	std::vector<uint32_t> get_spirv_data (std::string const& name, ShaderType type);


	ShaderCompiler compiler;
	ShaderDatabase database;

	private:
	std::vector<uint32_t> align_data (std::vector<char> const& code);

	job::ThreadPool& thread_pool;
	std::mutex lock;
	ShaderID cur_id = 0;
	std::unordered_map<ShaderID, ShaderInfo> shaders;
};
} // namespace Resource::Shader