#ifndef _MYSQL_EDITOR_RELATIONSHIP_H_
#define _MYSQL_EDITOR_RELATIONSHIP_H_

#include "grt/editor_base.h"

#include "grts/structs.workbench.physical.h"

#include "mysql_support_backend_public_interface.h"

#define RelationshipEditorBE_VERSION 1

class MYSQLWBMYSQLSUPPORTBACKEND_PUBLIC_FUNC RelationshipEditorBE : public bec::BaseEditor {
protected:
  workbench_physical_ConnectionRef _relationship;

public: // editor interface
  enum VisibilityType { Visible = 1, Splitted = 2, Hidden = 3 };

  RelationshipEditorBE(const workbench_physical_ConnectionRef &relationship);
  virtual bool should_close_on_delete_of(const std::string &oid);

  bool model_only() {
    return *get_relationship()->foreignKey()->modelOnly() == 1;
  }
  void set_model_only(bool flag);

  GrtObjectRef get_object() {
    return get_relationship();
  }

  workbench_physical_ConnectionRef get_relationship() {
    return _relationship;
  }

  virtual std::string get_title();

  void set_caption(const std::string &caption);
  std::string get_caption();
  std::string get_caption_long();

  void set_extra_caption(const std::string &caption);
  std::string get_extra_caption();
  std::string get_extra_caption_long();

  void set_left_mandatory(bool flag);
  bool get_left_mandatory();

  void set_right_mandatory(bool flag);
  bool get_right_mandatory();

  VisibilityType get_visibility();
  void set_visibility(VisibilityType type);

  void open_editor_for_table(const db_TableRef &table);
  void open_editor_for_left_table();
  void open_editor_for_right_table();

  void set_to_many(bool flag);
  bool get_to_many();

  bool get_is_identifying();
  void set_is_identifying(bool flag);

  void set_comment(const std::string &comment);
  std::string get_comment();

  std::string get_left_table_name();
  std::string get_right_table_name();

  std::string get_left_table_fk();

  std::string get_left_table_info();
  std::string get_right_table_info();

  void edit_left_table();
  void edit_right_table();
  void invert_relationship();
};

#endif /* _EDITOR_RELATIONSHIP_H_ */
