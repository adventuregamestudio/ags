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

            virtual void __clrcall Write(System::IO::FileStream ^ostream, System::String ^scriptFileName)
            {
                if (_compiledScript == NULL)
                {
                    throw gcnew AGS::Types::CompileError(gcnew System::String("Script has not been compiled: ") + scriptFileName);
                }
                System::IO::BinaryWriter ^writer = gcnew System::IO::BinaryWriter(ostream);
                for (int i = 0; i < 4; ++i)
                {
                    // the BinaryWriter seems to be treating CHAR as a 4-byte type here?
                    writer->Write((System::Byte)scfilesig[i]);
                }
                writer->Write(SCOM_VERSION);
                writer->Write(_compiledScript->globaldatasize);
                writer->Write(_compiledScript->codesize);
                writer->Write(_compiledScript->stringssize);
                for (int i = 0; i < _compiledScript->globaldatasize; ++i)
                {
                    writer->Write((System::Byte)_compiledScript->globaldata[i]);
                }
                for (int i = 0; i < _compiledScript->codesize; ++i)
                {
                    writer->Write((int)_compiledScript->code[i]);
                }
                for (int i = 0; i < _compiledScript->stringssize; ++i)
                {
                    writer->Write((System::Byte)_compiledScript->strings[i]);
                }
                writer->Write(_compiledScript->numfixups);
                for (int i = 0; i < _compiledScript->numfixups; ++i)
                {
                    writer->Write((System::Byte)_compiledScript->fixuptypes[i]);
                }
                for (int i = 0; i < _compiledScript->numfixups; ++i)
                {
                    writer->Write(_compiledScript->fixups[i]);
                }
                writer->Write(_compiledScript->numimports);
                for (int i = 0; i < _compiledScript->numimports; ++i)
                {
                    for (int j = 0, len = strlen(_compiledScript->imports[i]); j <= len; ++j)
                    {
                        writer->Write((System::Byte)_compiledScript->imports[i][j]);
                    }
                }
                writer->Write(_compiledScript->numexports);
                for (int i = 0; i < _compiledScript->numexports; ++i)
                {
                    for (int j = 0, len = strlen(_compiledScript->exports[i]); j <= len; ++j)
                    {
                        writer->Write((System::Byte)_compiledScript->exports[i][j]);
                    }
                    writer->Write(_compiledScript->export_addr[i]);
                }
                writer->Write(_compiledScript->numSections);
                for (int i = 0; i < _compiledScript->numSections; ++i)
                {
                    for (int j = 0, len = strlen(_compiledScript->sectionNames[i]); j <= len; ++j)
                    {
                        writer->Write((System::Byte)_compiledScript->sectionNames[i][j]);
                    }
                    writer->Write(_compiledScript->sectionOffsets[i]);
                }
                writer->Write(ENDFILESIG);
            }
        };
	}
}