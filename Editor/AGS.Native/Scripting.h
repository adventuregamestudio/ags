//#include "cscomp.h"
#include "script/cc_script.h"

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
		};
	}
}