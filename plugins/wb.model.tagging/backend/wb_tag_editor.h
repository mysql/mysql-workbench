/*
 *  wb_tag_editor.h
 *  MySQLWorkbench
 *
 *  Created by Alfredo Kojima on 10/Apr/09.
 *  Copyright 2009 Sun Microsystems Inc. All rights reserved.
 *
 */

#ifndef _WB_TAG_EDITOR_BE_
#define _WB_TAG_EDITOR_BE_

#include <sigc++/sigc++.h>
#include "grts/structs.workbench.physical.h"
#include "grt/tree_model.h"

class TagObjectListBE : public bec::ListModel {
public:
  struct Node {
    meta_TaggedObjectRef owner;

    db_DatabaseObjectRef object;
    std::string doc;
  };

private:
  db_CatalogRef _catalog;
  meta_TagRef _tag;

  std::vector<Node> _content;

public:
  enum { Name, Documentation };

  TagObjectListBE(const db_CatalogRef &catalog);

  bool add_dropped_objectdata(const std::string &data);

  void set_tag(const meta_TagRef &tag);

  virtual bool get_field(const bec::NodeId &node, int column, std::string &value);
  virtual bool set_field(const bec::NodeId &node, int column, const std::string &value);
  virtual bec::IconId get_field_icon(const bec::NodeId &node, int column, bec::IconSize size);
  virtual void refresh();
  virtual int count();

  bool commit();
};

class TagEditorBE : public sigc::trackable {
  TagObjectListBE _object_list;

  workbench_physical_ModelRef _model;
  int _selected_category;
  int _selected_tag;
  bool _changed;

public:
  TagEditorBE(const workbench_physical_ModelRef &model);

  TagObjectListBE *get_object_list() {
    return &_object_list;
  }

  std::vector<std::string> get_categories() const;

  void edit_categories();
  void set_selected_category(int index);
  int get_selected_category() const {
    return _selected_category;
  }

  std::vector<std::string> get_tags() const;

  void add_tag();
  bool delete_tag();

  void begin_save();
  void end_save();

  void set_selected_tag(int index);

  void set_tag_name(const std::string &name);
  void set_tag_label(const std::string &label);
  void set_tag_color(const std::string &color);
  void set_tag_comment(const std::string &color);

  std::string get_tag_name();
  std::string get_tag_label();
  std::string get_tag_color();
  std::string get_tag_comment();

  std::vector<std::string> get_objects();
};

#endif