#ifndef _WB_NOTE_EDITOR_H_
#define _WB_NOTE_EDITOR_H_

#include "grt/editor_base.h"
#include "grts/structs.workbench.model.h"

#include "wb_editor_backend_public_interface.h"

class WBEDITOR_BACKEND_PUBLIC_FUNC NoteEditorBE : public bec::BaseEditor
{
  workbench_model_NoteFigureRef _note;

public:
  NoteEditorBE(bec::GRTManager *grtm, const workbench_model_NoteFigureRef &note);

  virtual bool should_close_on_delete_of(const std::string &oid);
  
  void set_text(const std::string &text);
  std::string get_text();

  void set_name(const std::string &name);
  std::string get_name();
  
  virtual GrtObjectRef get_object() { return _note; }
  virtual std::string get_title();
};

#endif /* _WB_NOTE_EDITOR_H_ */
