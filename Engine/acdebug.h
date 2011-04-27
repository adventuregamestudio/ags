#ifndef __ACDEBUG_H
#define __ACDEBUG_H

struct IAGSEditorDebugger
{
public:
  virtual bool Initialize() = 0;
  virtual void Shutdown() = 0;
  virtual bool SendMessageToEditor(const char *message) = 0;
  virtual bool IsMessageAvailable() = 0;
  // Message will be allocated on heap with malloc
  virtual char* GetNextMessage() = 0;
};

extern IAGSEditorDebugger *GetEditorDebugger(const char* instanceToken);

#endif
