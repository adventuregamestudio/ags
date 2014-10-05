//#include "cscomp.h"
#include "script/cc_script.h"
#include "Common/util/filestream.h"
#include <io.h>
#include <cstdio>
#include <Fcntl.h>

namespace AGS
{
	namespace Native 
	{
		public ref class CompiledScript : ICompiledScript
		{
		private:
			ccScript* _compiledScript;
		public:
			CompiledScript(ccScript *script) 
			{
				_compiledScript = script;
			}

			property ccScript* Data
			{
				ccScript* get() { return _compiledScript; }
				void set(ccScript* newScript) { _compiledScript = newScript; }
			}

			~CompiledScript() 
			{
				this->!CompiledScript();
			}

			!CompiledScript()
			{
				delete _compiledScript;
				_compiledScript = NULL;
			}

            void Write(AGS::Common::Stream *out)
            {
                Data->Write(out);
            }

            virtual void __clrcall Write(System::IO::FileStream ^out)
            {
                // NOTE: This is a TEMPORARY hack to allow the managed code to call the native
                // Data->Write method. Refactoring that method will require basically refactoring the
                // entire compiler into managed code (which is partially done). -monkey0506
                using namespace AGS::Common;
                if (!out->CanWrite)
                {
                    throw gcnew AGS::Types::CompileError(gcnew System::String("Cannot write to file!"));
                }
                void *handle = out->SafeFileHandle->DangerousGetHandle().ToPointer();
                int fd = _open_osfhandle((intptr_t)handle, _O_APPEND);
                if (fd == -1)
                {
                    throw gcnew AGS::Types::CompileError(gcnew System::String("Cannot write to file!"));
                }
                FILE *fp = _fdopen(fd, "a+");
                if (fp == NULL)
                {
                    throw gcnew AGS::Types::CompileError(gcnew System::String("Cannot write to file!"));
                }
                FileStream fs(fp, kFile_Create, kFile_Write);
                Write(&fs);
                System::GC::KeepAlive(out);
            }
		};
	}
}