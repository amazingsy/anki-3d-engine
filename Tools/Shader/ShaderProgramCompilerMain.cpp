// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/ShaderCompiler/ShaderProgramCompiler.h>
#include <AnKi/Util.h>
using namespace anki;

static const char* USAGE = R"(Usage: %s input_shader_program_file [options]
Options:
-o <name of output>    : The name of the output binary
-j <thread count>      : Number of threads. Defaults to system's max
-I <include path>      : The path of the #include files
)";

class CmdLineArgs
{
public:
	HeapAllocator<U8> m_alloc{allocAligned, nullptr};
	StringAuto m_inputFname = {m_alloc};
	StringAuto m_outFname = {m_alloc};
	StringAuto m_includePath = {m_alloc};
	U32 m_threadCount = getCpuCoresCount();
};

static Error parseCommandLineArgs(int argc, char** argv, CmdLineArgs& info)
{
	// Parse config
	if(argc < 2)
	{
		return Error::USER_DATA;
	}

	info.m_inputFname.create(argv[1]);

	for(I i = 2; i < argc; i++)
	{
		if(strcmp(argv[i], "-o") == 0)
		{
			++i;

			if(i < argc)
			{
				if(std::strlen(argv[i]) > 0)
				{
					info.m_outFname.sprintf("%s", argv[i]);
				}
				else
				{
					return Error::USER_DATA;
				}
			}
			else
			{
				return Error::USER_DATA;
			}
		}
		else if(strcmp(argv[i], "-j") == 0)
		{
			++i;

			if(i < argc)
			{
				ANKI_CHECK(CString(argv[i]).toNumber(info.m_threadCount));
			}
			else
			{
				return Error::USER_DATA;
			}
		}
		else if(strcmp(argv[i], "-I") == 0)
		{
			++i;

			if(i < argc)
			{
				if(std::strlen(argv[i]) > 0)
				{
					info.m_includePath.sprintf("%s", argv[i]);
				}
				else
				{
					return Error::USER_DATA;
				}
			}
			else
			{
				return Error::USER_DATA;
			}
		}
		else
		{
			return Error::USER_DATA;
		}
	}

	return Error::NONE;
}

static Error work(const CmdLineArgs& info)
{
	HeapAllocator<U8> alloc{allocAligned, nullptr};

	// Load interface
	class FSystem : public ShaderProgramFilesystemInterface
	{
	public:
		CString m_includePath;
		U32 m_fileReadCount = 0;

		Error readAllTextInternal(CString filename, StringAuto& txt)
		{
			StringAuto fname(txt.getAllocator());

			// The first file is the input file. Don't append the include path to it
			if(m_fileReadCount == 0)
			{
				fname.sprintf("%s", filename.cstr());
			}
			else
			{
				fname.sprintf("%s/%s", m_includePath.cstr(), filename.cstr());
			}
			++m_fileReadCount;

			File file;
			ANKI_CHECK(file.open(fname, FileOpenFlag::READ));
			ANKI_CHECK(file.readAllText(txt));
			return Error::NONE;
		}

		Error readAllText(CString filename, StringAuto& txt) final
		{
			const Error err = readAllTextInternal(filename, txt);
			if(err)
			{
				ANKI_LOGE("Failed to read file: %s", filename.cstr());
			}

			return err;
		}
	} fsystem;
	fsystem.m_includePath = info.m_includePath;

	// Threading interface
	class TaskManager : public ShaderProgramAsyncTaskInterface
	{
	public:
		ThreadHive* m_hive = nullptr;
		HeapAllocator<U8> m_alloc;

		void enqueueTask(void (*callback)(void* userData), void* userData)
		{
			struct Ctx
			{
				void (*m_callback)(void* userData);
				void* m_userData;
				HeapAllocator<U8> m_alloc;
			};
			Ctx* ctx = m_alloc.newInstance<Ctx>();
			ctx->m_callback = callback;
			ctx->m_userData = userData;
			ctx->m_alloc = m_alloc;

			m_hive->submitTask(
				[](void* userData, U32 threadId, ThreadHive& hive, ThreadHiveSemaphore* signalSemaphore) {
					Ctx* ctx = static_cast<Ctx*>(userData);
					ctx->m_callback(ctx->m_userData);
					auto alloc = ctx->m_alloc;
					alloc.deleteInstance(ctx);
				},
				ctx);
		}

		Error joinTasks()
		{
			m_hive->waitAllTasks();
			return Error::NONE;
		}
	} taskManager;
	taskManager.m_hive =
		(info.m_threadCount) ? alloc.newInstance<ThreadHive>(info.m_threadCount, alloc, true) : nullptr;
	taskManager.m_alloc = alloc;

	// Compiler options
	ShaderCompilerOptions compilerOptions;
	compilerOptions.m_bindlessLimits.m_bindlessImageCount = 16;
	compilerOptions.m_bindlessLimits.m_bindlessTextureCount = 16;

	// Compile
	ShaderProgramBinaryWrapper binary(alloc);
	ANKI_CHECK(compileShaderProgram(info.m_inputFname, fsystem, nullptr, (info.m_threadCount) ? &taskManager : nullptr,
									alloc, compilerOptions, binary));

	// Store the binary
	ANKI_CHECK(binary.serializeToFile(info.m_outFname));

	// Cleanup
	alloc.deleteInstance(taskManager.m_hive);

	return Error::NONE;
}

int main(int argc, char** argv)
{
	CmdLineArgs info;
	if(parseCommandLineArgs(argc, argv, info))
	{
		ANKI_LOGE(USAGE, argv[0]);
		return 1;
	}

	if(info.m_outFname.isEmpty())
	{
		getFilepathFilename(info.m_inputFname, info.m_outFname);
		info.m_outFname.append("bin");
	}

	if(info.m_includePath.isEmpty())
	{
		info.m_includePath.create("./");
	}

	if(work(info))
	{
		ANKI_LOGE("Failed");
		return 1;
	}

	ANKI_LOGI("Done!");

	return 0;
}
