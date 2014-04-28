#ifndef _WB_IMAGE_EDITOR_H_
#define _WB_IMAGE_EDITOR_H_

#include "grt/editor_base.h"
#include "grts/structs.workbench.model.h"

#include "wb_editor_backend_public_interface.h"

class WBEDITOR_BACKEND_PUBLIC_FUNC ImageEditorBE : public bec::BaseEditor
{
  workbench_model_ImageFigureRef _image;

public:
  ImageEditorBE(bec::GRTManager *grtm, const workbench_model_ImageFigureRef &image);

  virtual bool should_close_on_delete_of(const std::string &oid);
  
  GrtObjectRef get_object() { return _image; }
  
  void get_size(int &w, int &h);
  void set_size(int w, int h);
  void set_width(int w);
  void set_height(int h);
  
  bool get_keep_aspect_ratio();
  void set_keep_aspect_ratio(bool flag);

  void set_filename(const std::string &text);
  std::string get_filename() const;
  
  std::string get_attached_image_path();
  
  virtual std::string get_title();
};

#endif /* _WB_IMAGE_EDITOR_H_ */
