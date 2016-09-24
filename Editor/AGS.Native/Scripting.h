//#include "cscomp.h"
#include "script/cc_script.h"
#include "script/script_common.h"

namespace AGS
{
	namespace Native 
	{
		public ref class CompiledScript : ICompiledScript
		{
		private:
			PScript* _compiledScript;
		public:
			CompiledScript(PScript script) 
			{
				_compiledScript = new PScript();
				*_compiledScript = script;
			}

			property PScript Data
			{
				PScript get() { return *_compiledScript; }
				void set(PScript newScript) { *_compiledScript = newScript; }
			}

			~CompiledScript() 
			{
				this->!CompiledScript();
			}

			!CompiledScript()
			{
				_compiledScript->reset();
				delete _compiledScript;
				_compiledScript = NULL;
			}

            virtual void __clrcall Write(System::IO::FileStream ^ostream, System::String ^scriptFileName)
            {
                if (*_compiledScript == NULL)
                {
                    throw gcnew AGS::Types::CompileError(gcnew System::String("Script has not been compiled: ") + scriptFileName);
                }
                System::IO::BinaryWriter ^writer = gcnew System::IO::BinaryWriter(ostream);
                for (int i = 0; i < 4; ++i)
                {
                    // the BinaryWriter seems to be treating CHAR as a 4-byte type here?
                    writer->Write((System::Byte)scfilesig[i]);
                }
                const ccScript *cs = _compiledScript->get();
                writer->Write(SCOM_VERSION);
                writer->Write(cs->globaldatasize);
                writer->Write(cs->codesize);
                writer->Write(cs->stringssize);
                for (int i = 0; i < cs->globaldatasize; ++i)
                {
                    writer->Write((System::Byte)cs->globaldata[i]);
                }
                for (int i = 0; i < cs->codesize; ++i)
                {
                    writer->Write((int)cs->code[i]);
                }
                for (int i = 0; i < cs->stringssize; ++i)
                {
                    writer->Write((System::Byte)cs->strings[i]);
                }
                writer->Write(cs->numfixups);
                for (int i = 0; i < cs->numfixups; ++i)
                {
                    writer->Write((System::Byte)cs->fixuptypes[i]);
                }
                for (int i = 0; i < cs->numfixups; ++i)
                {
                    writer->Write(cs->fixups[i]);
                }
                writer->Write(cs->numimports);
                for (int i = 0; i < cs->numimports; ++i)
                {
                    for (int j = 0, len = strlen(cs->imports[i]); j <= len; ++j)
                    {
                        writer->Write((System::Byte)cs->imports[i][j]);
                    }
                }
                writer->Write(cs->numexports);
                for (int i = 0; i < cs->numexports; ++i)
                {
                    for (int j = 0, len = strlen(cs->exports[i]); j <= len; ++j)
                    {
                        writer->Write((System::Byte)cs->exports[i][j]);
                    }
                    writer->Write(cs->export_addr[i]);
                }
                writer->Write(cs->numSections);
                for (int i = 0; i < cs->numSections; ++i)
                {
                    for (int j = 0, len = strlen(cs->sectionNames[i]); j <= len; ++j)
                    {
                        writer->Write((System::Byte)cs->sectionNames[i][j]);
                    }
                    writer->Write(cs->sectionOffsets[i]);
                }
                writer->Write(ENDFILESIG);
            }
        };
	}
}