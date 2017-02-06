#ifndef _WB_OVERVIEW_PHYSICAL_SCHEMA_H_
#define _WB_OVERVIEW_PHYSICAL_SCHEMA_H_

#include "workbench/wb_overview.h"

namespace wb {
  namespace internal {

    class PhysicalSchemaContentNode;
    class SchemaObjectNode;

    class PhysicalSchemaNode : public OverviewBE::ContainerNode {
    public:
      virtual void init();

    protected:
      bool _is_routine_group_enabled;

    public:
      PhysicalSchemaNode(db_SchemaRef schema);
      virtual bool is_pasteable(bec::Clipboard *clip);
      virtual void paste_object(WBContext *wb, bec::Clipboard *clip);
      virtual bool is_deletable();
      virtual void delete_object(WBContext *wb);
      virtual bool is_renameable();
      virtual bool rename(WBContext *wb, const std::string &name);
      virtual bool activate(WBContext *wb);
      virtual void focus(OverviewBE *sender);
      virtual void refresh();

    public:
      virtual bool add_new_db_table(WBContext *wb);
      virtual bool add_new_db_view(WBContext *wb);
      virtual bool add_new_db_routine_group(WBContext *wb);
      virtual bool add_new_db_routine(WBContext *wb);

      virtual SchemaObjectNode *create_table_node(const db_DatabaseObjectRef &dbobject);
      virtual SchemaObjectNode *create_view_node(const db_DatabaseObjectRef &dbobject);
      virtual SchemaObjectNode *create_routine_node(const db_DatabaseObjectRef &dbobject);
      virtual SchemaObjectNode *create_routine_group_node(const db_DatabaseObjectRef &dbobject);
    };

    class SchemaObjectNode : public OverviewBE::ObjectNode {
      SchemaObjectNode(const SchemaObjectNode &copy) : OverviewBE::ObjectNode(copy) {
      }

    public:
      SchemaObjectNode(const db_DatabaseObjectRef &dbobject);
      virtual void delete_object(WBContext *wb);
      virtual bool is_deletable();
      virtual bool is_renameable();
      virtual void copy_object(WBContext *wb, bec::Clipboard *clip);
      virtual bool is_copyable();
    };

    class SchemaTableNode : public SchemaObjectNode {
    public:
      SchemaTableNode(const db_DatabaseObjectRef &dbobject) : SchemaObjectNode(dbobject) {
      }
      virtual std::string get_detail(int field);
    };

    class SchemaViewNode : public SchemaObjectNode {
    public:
      SchemaViewNode(const db_DatabaseObjectRef &dbobject) : SchemaObjectNode(dbobject) {
      }
      virtual bool is_renameable();
      virtual std::string get_detail(int field);
    };

    class SchemaRoutineGroupNode : public SchemaObjectNode {
    public:
      SchemaRoutineGroupNode(const db_DatabaseObjectRef &dbobject) : SchemaObjectNode(dbobject) {
      }
      virtual std::string get_detail(int field);
    };

    class SchemaRoutineNode : public SchemaObjectNode {
    public:
      SchemaRoutineNode(const db_DatabaseObjectRef &dbobject) : SchemaObjectNode(dbobject) {
      }
      virtual std::string get_detail(int field);
      virtual bool is_renameable();
    };
  };
};

#endif /* _WB_OVERVIEW_PHYSICAL_SCHEMA_H_ */
