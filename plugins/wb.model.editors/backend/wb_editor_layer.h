/*
 *  wb_editor_layer.h
 *  MySQLWorkbench
 *
 *  Created by Alfredo Kojima on 17/Apr/09.
 *  Copyright 2009 Sun Microsystems Inc. All rights reserved.
 *
 */

#ifndef _WB_LAYER_EDITOR_H_
#define _WB_LAYER_EDITOR_H_

#include "grt/editor_base.h"
#include "grts/structs.workbench.physical.h"

#include "wb_editor_backend_public_interface.h"

class WBEDITOR_BACKEND_PUBLIC_FUNC LayerEditorBE : public bec::BaseEditor
{
  workbench_physical_LayerRef _layer;
  
public:
  LayerEditorBE(bec::GRTManager *grtm, const workbench_physical_LayerRef &layer);
  
  virtual bool should_close_on_delete_of(const std::string &oid);
  
  void set_color(const std::string &color);
  std::string get_color();
  
  void set_name(const std::string &name);
  std::string get_name();
  
  virtual std::string get_title();
  
  virtual GrtObjectRef get_object() { return _layer; }
};

#endif /* _WB_LAYER_EDITOR_H_ */
