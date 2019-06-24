/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include "grt/common.h"
#include "grt/grt_manager.h"
#include "wb_model_file.h"
#include "upgrade_helper.h"
#include "grtdb/db_object_helpers.h"
#include "grts/structs.db.mysql.h"
#include "sqlide/table_inserts_loader_be.h"
#include "base/log.h"

#include "base/string_utilities.h"
#include "mforms/utilities.h"
#include "grtsqlparser/sql_facade.h"

DEFAULT_LOG_DOMAIN("ModelUpgrader");

using namespace wb;
using namespace grt;
using namespace base;

#ifdef _MSC_VER
#ifndef strcasecmp
#define strcasecmp strcmpi
#endif
#endif

static void fixup_layer_tree(XMLTraverser &xml, xmlNodePtr layerNode, double xoffs, double yoffs) {
  xmlNodePtr sublayer_list = xml.get_object_child(layerNode, "subLayers");
  xmlNodePtr sublayer;

  // if (!sublayers) return;

  for (size_t i = 0; (sublayer = xml.get_object_child_by_index(sublayer_list, (int)i)) != 0; i++) {
    double top = xml.get_object_double_value(sublayer, "top");
    double left = xml.get_object_double_value(sublayer, "left");

    set_grt_object_item_value(sublayer, "top", yoffs + top);
    set_grt_object_item_value(sublayer, "left", xoffs + left);

    fixup_layer_tree(xml, sublayer, xoffs + left, yoffs + top);
  }
}

static bool fix_user_datatypes(xmlNodePtr parent, xmlNodePtr node) {
  xmlString prop = xmlGetProp(parent, (xmlChar *)"struct-name");

  if (prop) {
    if (xmlStrcmp(prop, (xmlChar *)"db.mysql.Column") == 0) {
      xmlString key = xmlGetProp(node, (xmlChar *)"key");
      // its a column and the field is simpleType
      if (key && xmlStrcmp(node->name, (xmlChar *)"link") == 0 && xmlStrcmp(key, (xmlChar *)"simpleType") == 0) {
        xmlString type_id = xmlNodeGetContent(node);
        // this type used to be simpleTypes, but was turned into userType
        if (strcmp(type_id, "com.mysql.rdbms.mysql.datatype.boolean") == 0) {
          xmlSetProp(node, (xmlChar *)"struct-name", (xmlChar *)"db.UserDatatype");
          xmlSetProp(node, (xmlChar *)"key", (xmlChar *)"userType");
          xmlNodeSetContent(node, (xmlChar *)"com.mysql.rdbms.mysql.userdatatype.boolean");
        }
      }
      return false;
    }
  }

  return true;
}

// Low-level upgrade by manipulation of XML document
bool ModelFile::attempt_xml_document_upgrade(xmlDocPtr xmldoc, const std::string &version) {
  std::vector<std::string> ver = base::split(version, ".");
  int major, minor, revision;

  major = base::atoi<int>(ver[0], 0);
  minor = base::atoi<int>(ver[1], 0);
  revision = base::atoi<int>(ver[2], 0);

  if (major == 1 && minor == 1) {
    // Versions: 1.1.0 -> 1.1.1
    // Performed Changes:
    // * convert views from old pageWidth/Height to pageSettings data
    // since older WB versions didnt have a way to change the page size and
    // number of pages, we will just assume A4 as paper type and use some
    // default settings, keeping the total view size
    if (revision == 0) {
      XMLTraverser xml(xmldoc);
      std::vector<xmlNodePtr> views(xml.scan_objects_of_type("workbench.physical.View"));

      for (size_t c = views.size(), i = 0; i < c; i++) {
        xmlNodePtr view = views[i];
        std::string id = xml.node_prop(view, "id");

        // delete pageWidth and pageHeight
        xml.delete_object_item(view, "pageWidth");
        xml.delete_object_item(view, "pageHeight");
#if 0
        char *guid= myx_grt_get_guid();

        xmlNodePtr page_settings= create_grt_object_node(guid, model_PageSettings::static_class_name());
        g_free(guid);
        
        set_grt_object_item(view, "pageSettings", page_settings);
        set_grt_object_item_link(page_settings, "owner", workbench_physical_Diagram::static_class_name(), id.c_str());
        set_grt_object_item_value(page_settings, "orientation", "portrait");
        set_grt_object_item_link(page_settings, "paperType", app_PaperType::static_class_name(), 
            grt::find_named_object_in_list(get_root().options().paperTypes(), "A4").id().c_str());
#endif
      }

      // document is now 1.1.1 compatible
      revision = 1;
    }

    // fixes a bug that apparently was in <= 1.1.1 where db.IndexColumn was created instead of db.mysql.IndexColumn and
    // others
    if (revision == 1) {
      find_replace_xml_attribute(xmlDocGetRootElement(xmldoc), "struct-name", "db.IndexColumn", "db.mysql.IndexColumn");
      find_replace_xml_attribute(xmlDocGetRootElement(xmldoc), "struct-name", "db.ForeignKey", "db.mysql.ForeignKey");
    }

    // Versions: 1.1.1 -> 1.1.2
    // Performed Changes:
    // * Moved pageSettings property from /wb/options/pageSettings to /wb/doc/pageSettings
    if (revision == 1) {
      XMLTraverser xml(xmldoc);

      if (!xml.get_object_by_path("/pageSettings")) {
        std::string id = grt::get_guid();
        xmlNodePtr page_settings = create_grt_object_node(id.c_str(), "app.PageSettings");

        set_grt_object_item_link(page_settings, "paperType", "app.PaperType",
                                 "com.mysql.wb.papertype.a4"); // hardcode a4
        set_grt_object_item_value(page_settings, "orientation", "portrait");
        set_grt_object_item_value(page_settings, "marginTop", 6.35);
        set_grt_object_item_value(page_settings, "marginBottom", 14.46);
        set_grt_object_item_value(page_settings, "marginLeft", 6.35);
        set_grt_object_item_value(page_settings, "marginRight", 6.35);
        xml.set_object_link(page_settings, "owner", xml.get_root());

        set_grt_object_item(xml.get_root(), "pageSettings", page_settings);
      }

      // document is now 1.1.2 compatible
      revision = 2;
    }

    // Versions: 1.1.2 -> 1.1.3
    // Performed Changes:
    // * Rename referedTable -> referencedTable
    // * Rename referedColumn -> referencedColumn
    // * Rename referedColumns -> referencedColumns
    // * Rename referedMandatory -> referencedMandatory
    // * Rename drawSplitted -> drawSplit
    if (revision == 2) {
      XMLTraverser xml(xmldoc);
      std::list<xmlNodePtr> nodes;

#define RENAME_FIELDS(from, to)                                                             \
  do {                                                                                      \
    nodes = xml.scan_nodes_with_key(from);                                                  \
    for (std::list<xmlNodePtr>::iterator iter = nodes.begin(); iter != nodes.end(); ++iter) \
      xmlSetProp((*iter), (xmlChar *)"key", (xmlChar *)to);                                 \
  } while (0)

      RENAME_FIELDS("drawSplitted", "drawSplit");
      RENAME_FIELDS("referedTable", "referencedTable");
      RENAME_FIELDS("referedColumn", "referencedColumn");
      RENAME_FIELDS("referedColumns", "referencedColumns");
      RENAME_FIELDS("referedMandatory", "referencedMandatory");

#undef RENAME_FIELDS

      // document is now 1.1.3 compatible
      revision = 3;
    }

    // No changes needed
    if (revision == 3)
      revision = 4;

    // No change needed
    if (revision == 4)
      revision = 5;

    // Version 1.1.5 -> 1.1.6
    // * model.Layer::subLayers doesnt exist anymore, sublayers should be reparented
    // * model.View::layers does not include rootLayer anymore (handled in attempt_document_upgrade)
    if (revision == 5) {
      XMLTraverser xml(xmldoc);
      xmlNodePtr node;

      // go through all layers to find the parent layer for each
      node = xml.get_object_by_path("/physicalModels");
      if (node) {
        node = xml.get_object_child_by_index(node, 0);
        if (node) {
          xmlNodePtr view;

          node = xml.get_object_child(node, "views");

          for (size_t v = 0; (view = xml.get_object_child_by_index(node, (int)v)) != 0; v++) {
            xmlNodePtr rootLayer = xml.get_object_child(view, "rootLayer");

            fixup_layer_tree(xml, rootLayer, 0, 0);
          }
        }
      }
      revision = 6;
    }

    // version 1.1.6 -> 1.2.0
    // * workbench.model.Layer -> workbench.physical.Layer
    if (revision == 6) {
      const char *attribs[] = {"struct-name", "content-struct-name", NULL};
      const char *from[] = {"workbench.model.Layer", NULL};
      const char *to[] = {"workbench.physical.Layer", NULL};

      find_replace_xml_attributes(xmlDocGetRootElement(xmldoc), attribs, from, to);

      revision = 0;
      minor = 2;
    }

    // future versions not supported
    if (revision > 6)
      return false;
  }

  if (major == 1 && minor == 2) {
    // version: 1.2.0 -> 1.3.0
    // * renamed view to diagram

    // struct names (look for name, struct-name, content-struct-name)
    // model.View -> model.Diagram
    // workbench.physical.View -> workbench.physical.Diagram
    // workbench.logical.View -> workbench.logical.Diagram

    // member names (key)
    // model.Model:currentView -> currentDiagram
    // model.Model:views -> diagrams
    // model.Marker:view -> diagram
    // workbench.physical.Model:views -> diagrams
    // workbench.logical.Model:views -> diagrams

    // removed model.Figure:enabled model.Layer:enabled

    // fix type of datatypes that are built-in UDTs (got broken a long time ago)

    {
      const char *attribs[] = {"struct-name", "content-struct-name", NULL};
      const char *from[] = {"model.View", "workbench.physical.View", "workbench.logical.View", NULL};
      const char *to[] = {"model.Diagram", "workbench.physical.Diagram", "workbench.logical.Diagram", NULL};

      find_replace_xml_attributes(xmlDocGetRootElement(xmldoc), attribs, from, to);
    }

    {
      const char *klass[] = {"model.Model",  "workbench.physical.Model", "workbench.logical.Model", "model.Model",
                             "model.Marker", "workbench.physical.Model", "workbench.logical.Model", NULL};
      const char *from[] = {"currentView", "currentView", "currentView", "views", "view", "views", "views", NULL};
      const char *to[] = {"currentDiagram", "currentDiagram", "currentDiagram", "diagrams",
                          "diagram",        "diagrams",       "diagrams",       NULL};

      rename_xml_grt_members(xmlDocGetRootElement(xmldoc), klass, from, to);
    }

    {
      const char *klass[] = {"workbench.physical.Layer",
                             "workbench.physical.TableFigure",
                             "workbench.physical.ViewFigure",
                             "workbench.physical.RoutineGroupFigure",
                             "workbench.model.NoteFigure",
                             "workbench.model.ImageFigure",
                             NULL};
      const char *member[] = {"enabled", "enabled", "enabled", "enabled", "enabled", "enabled", NULL};
      delete_xml_grt_members(xmlDocGetRootElement(xmldoc), klass, member);
    }

    {
      XMLTraverser xml(xmldoc);

      // delete deprecated boolean type from simpleTypes list
      xmlNodePtr typesList = xml.get_object_by_path("/physicalModels/0/catalog/simpleDatatypes");
      if (typesList) {
        for (xmlNodePtr node = typesList->children; node != NULL; node = node->next) {
          if (node->type == XML_ELEMENT_NODE && xmlStrcmp(node->name, (xmlChar *)"link") == 0) {
            xmlString oid = xmlNodeGetContent(node);
            if (strcmp(oid, "com.mysql.rdbms.mysql.datatype.boolean") == 0) {
              xmlUnlinkNode(node);
              xmlFreeNode(node);
              break;
            }
          }
        }
      }

      xml.traverse_subtree("/physicalModels/0/catalog/schemata/0/tables",  std::bind(&fix_user_datatypes, std::placeholders::_1, std::placeholders::_2));

      // add boolean UDT to the userTypes list
      typesList = xml.get_object_by_path("/physicalModels/0/catalog/userDatatypes");
      if (typesList) {
        xmlNodePtr userType = create_grt_object_node("com.mysql.rdbms.mysql.userdatatype.boolean", "db.UserDatatype");

        set_grt_object_item_value(userType, "name", "BOOLEAN");
        set_grt_object_item_value(userType, "sqlDefinition", "TINYINT(1)");
        xml.set_object_link_literal(userType, "actualType", "com.mysql.rdbms.mysql.datatype.tinyint",
                                    "db.SimpleDatatype");
        xml.set_object_link(userType, "owner", xml.get_object_by_path("/physicalModels/0/catalog"));

        xmlAddChild(typesList, userType);
      }
    }

    minor = 3;
    revision = 0;
  }

  if (major == 1 && minor == 3) {
    // 1.3.0 -> 1.3.1
    // some extra stuff added, no update needed
    if (revision == 0)
      revision = 1;
  }

  if (major == 1 && minor == 3) {
    // prepare for moving table inserts sql scripts from xml to sqlite db
    // cache needed info because sqlite db file doesn't exist at this stage
    XMLTraverser xml(xmldoc);
    std::vector<xmlNodePtr> tables(xml.scan_objects_of_type("db.mysql.Table"));
    for (size_t c = tables.size(), i = 0; i < c; i++) {
      xmlNodePtr table = tables[i];
      std::string id = xml.node_prop(table, "id");
      xmlNodePtr inserts = xml.get_object_child(table, "inserts");
      if (inserts != NULL) {
        xmlChar *sql = xmlNodeGetContent(inserts);
        table_inserts_sql_scripts[id] = (char *)sql;
        xmlFree(sql);
        xml.delete_object_item(table, "inserts");
      }
    }

    minor = 3;
    revision = 0;
  }

  if (major == 1 && minor == 4) {
    // in 1.4.1 a new index field was added to db.ForeignKey pointing to the
    // matching index created for the FK (this is done in higher level converter)
    if (revision == 0) {
      revision = 1;
    }
  }

  if (major > 1 || minor > 4)
    return false;

  return true;
}

static db_IndexRef find_matching_fk_index(std::map<std::string, db_IndexRef> &indexes,
                                          const grt::ListRef<db_Column> &fk_columns) {
  for (std::map<std::string, db_IndexRef>::iterator idx = indexes.begin(); idx != indexes.end(); ++idx) {
    grt::ListRef<db_IndexColumn> icolumns(idx->second->columns());

    if (fk_columns.count() == icolumns.count()) {
      bool ok = true;
      for (size_t c = icolumns.count(), i = 0; i < c; i++) {
        if (fk_columns.get_index(icolumns[i]->referencedColumn()) == grt::BaseListRef::npos) {
          ok = false;
          break;
        }
      }
      if (ok) {
        db_IndexRef the_index(idx->second);
        indexes.erase(idx);
        return the_index;
      }
    }
  }
  return db_IndexRef();
}

// High-Level Upgrade, by manipulation of GRT objects
workbench_DocumentRef ModelFile::attempt_document_upgrade(const workbench_DocumentRef &doc, xmlDocPtr xmldoc,
                                                          const std::string &version) {
  std::vector<std::string> ver = base::split(version, ".");
  int major, minor, revision;

  major = base::atoi<int>(ver[0].c_str(), 0);
  minor = base::atoi<int>(ver[1].c_str(), 0);
  revision = base::atoi<int>(ver[2].c_str(), 0);

  // for all revisions <= 1.2.0
  // fix the defaultValueIsNull to correspond to defaultValue itself
  if ((doc->physicalModels()->count() > 0) && (major == 1) && ((minor < 2) || ((minor == 2) && (revision == 0)))) {
    db_mysql_CatalogRef cat = db_mysql_CatalogRef::cast_from(doc->physicalModels()[0]->catalog());
    if (cat.is_valid()) {
      for (size_t i = 0; i < cat->schemata().count(); i++) {
        db_mysql_SchemaRef schema = cat->schemata().get(i);
        for (size_t j = 0; j < schema->tables().count(); j++) {
          db_mysql_TableRef table = schema->tables().get(j);
          for (size_t k = 0; k < table->columns().count(); k++) {
            db_ColumnRef column = table->columns().get(k);
            if (column->isNotNull()) {
              column->defaultValueIsNull(0);
              // column->defaultValue("");
            } else {
              bool nulldefv = !strcasecmp(column->defaultValue().c_str(), "NULL");
              bool defvisnull = column->defaultValueIsNull() ? true : false;

              if (defvisnull && !nulldefv)
                column->defaultValue("NULL");
              else if (!defvisnull && nulldefv)
                column->defaultValueIsNull(1);
            }
          }
        }
      }
    }
  }

  // Version * -> 1.1.2
  // Performed changes:
  // * Remove all dangling connections caused by bug. (not exactly a file format change)
  if (major <= 1 && minor <= 1 && revision <= 2) {
    if (doc->physicalModels().count() > 0) {
      ListRef<workbench_physical_Diagram> views(doc->physicalModels()[0]->diagrams());

      for (size_t c = views.count(), i = 0; i < c; i++) {
        ListRef<model_Connection> conns(views.get(i)->connections());
        for (ListRef<model_Connection>::const_reverse_iterator conn = conns.rbegin(); conn != conns.rend(); ++conn) {
          if (!(*conn)->startFigure().is_valid() || !(*conn)->endFigure().is_valid())
            conns.remove_value(*conn);
        }
      }
    }
  }

  // Version * -> 1.1.6
  // * model.View::layers does not include rootLayer anymore
  if (major <= 1 && minor <= 1 && revision < 6) {
    ListRef<workbench_physical_Diagram> views(doc->physicalModels()[0]->diagrams());

    for (size_t c = views.count(), i = 0; i < c; i++) {
      model_DiagramRef view(views[i]);

      view->layers().remove_value(view->rootLayer());
    }
  }

  // 1.3.1 added tags, should add default tag categories
  if (major <= 1 && (minor < 3 || (minor == 3 && revision < 1))) {
    if (doc->physicalModels()[0]->tagCategories().count() == 0) {
      GrtObjectRef cat(grt::Initialized);

      cat->owner(doc->physicalModels()[0]);
      cat->name("Business Rule");

      doc->physicalModels()[0]->tagCategories().insert(cat);
    }
  }

  // in 1.4.1 (after 5.2.19) a new index field was added to db.ForeignKey pointing to the
  // matching index created for the FK and the FOREIGN index type was dropped (just uses INDEX now)
  if (major <= 1 && (minor < 4 || (minor == 4 && revision < 2))) {
    ListRef<db_Schema> schemata(doc->physicalModels()[0]->catalog()->schemata());
    for (size_t sc = schemata.count(), si = 0; si < sc; si++) {
      db_SchemaRef schema(schemata[si]);
      ListRef<db_Table> tables = schema->tables();
      for (size_t c = tables.count(), i = 0; i < c; i++) {
        db_TableRef table = tables[i];
        ListRef<db_ForeignKey> fks(table->foreignKeys());
        ListRef<db_Index> indexes(table->indices());

        std::map<std::string, db_IndexRef> fk_indexes;
        // create list of indices by name and change the FOREIGN type indexes to INDEX
        for (size_t ic = indexes.count(), ii = 0; ii < ic; ii++) {
          fk_indexes[indexes[ii]->name()] = indexes[ii];

          if (indexes[ii]->indexType() == "FOREIGN")
            indexes[ii]->indexType("INDEX");
        }

        std::list<db_ForeignKeyRef> pending_fks;

        for (size_t fc = fks.count(), fi = 0; fi < fc; fi++) {
          db_ForeignKeyRef fk(fks[fi]);

          // check by name
          if (fk_indexes.find(fk->name()) != fk_indexes.end()) {
            // assign the index
            fk->index(fk_indexes[fk->name()]);
            // remove from list of indices so it wont match twice
            fk_indexes.erase(fk_indexes.find(fk->name()));
          } else
            // save to try finding later
            pending_fks.push_back(fk);
        }

        // try finding matching index by columns if it cant be found by name
        for (std::list<db_ForeignKeyRef>::iterator iter = pending_fks.begin(); iter != pending_fks.end(); ++iter) {
          db_ForeignKeyRef fk(*iter);
          db_IndexRef found_index = find_matching_fk_index(fk_indexes, fk->columns());
          if (found_index.is_valid()) {
            fk->index(found_index);

            logInfo("Document Upgrade: ForeignKey %s.%s was assigned the index %s by column matching\n",
                    table->name().c_str(), fk->name().c_str(), found_index->name().c_str());
          } else {
            // create the index here
            db_IndexRef new_index = bec::TableHelper::create_index_for_fk(fk);
            fk->index(new_index);
            table->indices().insert(new_index);

            logWarning("ForeignKey %s.%s has no matching index, created one\n", table->name().c_str(),
                       fk->name().c_str());
          }
        }
      }
    }
  }

  if (major <= 1 && (minor < 4 || (minor == 4 && revision < 3))) {
    bool ask_confirmation = true;
    bool aborted = false;
    ListRef<db_Schema> schemata(doc->physicalModels()[0]->catalog()->schemata());
    for (size_t sc = schemata.count(), si = 0; (si < sc) && !aborted; si++) {
      ListRef<db_Table> tables = schemata[si]->tables();
      for (size_t c = tables.count(), i = 0; (i < c) && !aborted; i++) {
        ListRef<db_ForeignKey> fks(tables[i]->foreignKeys());
        ListRef<db_Index> indexes(tables[i]->indices());
        for (size_t ic = indexes.count(), ii = 0; (ii < ic) && !aborted; ii++)
          for (size_t fc = fks.count(), fi = 0; (fi < fc) && !aborted; fi++)
            if (fks[fi]->name() == indexes[ii]->name()) {
              if (ask_confirmation &&
                  mforms::Utilities::show_message("Fix Index Names",
                                                  "Index names identical to FK names were found in the model, which is "
                                                  "not allowed for MySQL 5.5 and later. \r\nWould you like to rename "
                                                  "the indexes?",
                                                  "Rename", "Ignore") != mforms::ResultOk) {
                aborted = true;
                break;
              }
              ask_confirmation = false;
              std::string idx_name = indexes[ii]->name();
              if (idx_name.length() > 59)
                idx_name.resize(59);
              indexes[ii]->name(grt::get_name_suggestion_for_list_object(indexes, idx_name.append("_idx"), false));
              logInfo("Document Upgrade: Index %s.%s was renamed to %s.%s to avoid name conflict with FK\n",
                      tables[i]->name().c_str(), fks[fi]->name().c_str(), tables[i]->name().c_str(),
                      indexes[ii]->name().c_str());
              continue;
            }
      }
    }
    if (!ask_confirmation)
      bec::GRTManager::get()->has_unsaved_changes(true);
  }

  if (major <= 1 && (minor < 4 || (minor == 4 && revision < 4))) {
    ListRef<db_Schema> schemata(doc->physicalModels()[0]->catalog()->schemata());

    for (size_t sc = schemata.count(), si = 0; si < sc; si++) {
      db_SchemaRef schema(schemata[si]);

      db_mgmt_RdbmsRef rdbms = get_rdbms_for_db_object(schema);
      SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms(rdbms);
      Invalid_sql_parser::Ref sql_parser = sql_facade->invalidSqlParser();
      Sql_specifics::Ref sql_specifics = sql_facade->sqlSpecifics();
      std::string non_std_sql_delimiter = sql_specifics->non_std_sql_delimiter();

      ListRef<db_Table> tables = schema->tables();
      for (size_t c = tables.count(), i = 0; i < c; i++) {
        db_TableRef table = tables[i];
        grt::ListRef<db_Trigger> triggers = table->triggers();

        for (size_t i = 0; i < triggers.count(); ++i) {
          db_TriggerRef trigger = triggers.get(i);
          sql_parser->parse_trigger(trigger, trigger->sqlDefinition());
        }
      }

      ListRef<db_View> views = schema->views();
      for (size_t c = views.count(), i = 0; i < c; i++) {
        db_ViewRef view = views[i];
        sql_parser->parse_view(view, view->sqlDefinition());
      }

      ListRef<db_Routine> routines = schema->routines();
      for (size_t c = routines.count(), i = 0; i < c; i++) {
        db_RoutineRef routine = routines[i];
        std::string routine_sql;

        routine_sql.append(strfmt("DELIMITER %s\n\n", non_std_sql_delimiter.c_str()))
          .append("USE `")
          .append(routine->owner()->name())
          .append("`")
          .append(non_std_sql_delimiter.c_str())
          .append("\n\n");
        routine_sql.append(routine->sqlDefinition());
        sql_parser->parse_routine(routine, routine_sql);
      }
    }
  }
  if (major <= 1 && (minor < 4 || (minor == 4 && revision <= 4))) {
    if (!doc->physicalModels()[0]->catalog()->version().is_valid()) {
      GrtVersionRef version(grt::Initialized);
      version->name("Version");
      version->majorNumber(5);
      version->minorNumber(5);
      version->releaseNumber(-1);
      version->buildNumber(-1);
      doc->physicalModels()[0]->catalog()->version(version);
    };
  }
  // Version * -> X.X.X //!
  if (!has_file(get_rel_db_file_path())) {
    add_db_file(_content_dir);

    // finish moving table inserts sql scripts from xml to sqlite db
    // retrieve needed info from cache prepared earlier
    {
      TableInsertsLoader table_inserts_loader;
      ListRef<db_Schema> schemata(doc->physicalModels()[0]->catalog()->schemata());
      for (size_t sc = schemata.count(), si = 0; si < sc; si++) {
        db_SchemaRef schema(schemata[si]);
        ListRef<db_Table> tables = schema->tables();
        for (size_t c = tables.count(), i = 0; i < c; i++) {
          db_TableRef table = tables[i];
          TableInsertsSqlScripts::const_iterator it = table_inserts_sql_scripts.find(table->id());
          if (table_inserts_sql_scripts.end() != it)
            table_inserts_loader.process_table(table, it->second);
        }
      }
    }
  }

  return doc;
}

void ModelFile::cleanup_upgrade_data() {
  table_inserts_sql_scripts = TableInsertsSqlScripts();
}

// ------------------------- Consistency Checks -----------------------------------------

static void check_figure_layers(workbench_physical_DiagramRef view, std::list<std::string> &load_warnings) {
  std::set<std::string> seen_figures;

  // some unedidentified bug is causing figures to have inconsistent figure.layer
  // meaning they're not in figyre.layer.figures

  // go through all layers to find what figures they contain
  for (size_t lc = view->layers().count(), l = 0; l < lc; l++) {
    model_LayerRef layer(view->layers()[l]);

    for (size_t fc = layer->figures().count(), f = 0; f < fc; f++) {
      model_FigureRef figure(layer->figures()[f]);

      if (figure->layer() != layer)
        figure->layer(layer);

      seen_figures.insert(figure.id());
    }
  }
  {
    model_LayerRef layer(view->rootLayer());

    for (size_t fc = layer->figures().count(), f = 0; f < fc; f++) {
      model_FigureRef figure(layer->figures()[f]);

      if (figure->layer() != layer)
        figure->layer(layer);

      seen_figures.insert(figure.id());
    }
  }

  // now go through all figures and add figures that are not in any layer to the root
  for (ssize_t i = view->figures().count() - 1; i >= 0; --i) {
    model_FigureRef figure(view->figures()[i]);

    if (seen_figures.find(figure.id()) == seen_figures.end()) {
      view->rootLayer()->figures().insert(figure);
      figure->layer(view->rootLayer());
    }

    bool orphan = false;
    // check for orphaned figures
    if (figure.is_instance<workbench_physical_TableFigure>()) {
      if (!workbench_physical_TableFigureRef::cast_from(figure)->table().is_valid()) {
        load_warnings.push_back(
          strfmt("Table figure %s is not attached to any database table and was deleted.", figure->name().c_str()));
        orphan = true;
      }
    } else if (figure.is_instance<workbench_physical_ViewFigure>()) {
      if (!workbench_physical_ViewFigureRef::cast_from(figure)->view().is_valid()) {
        load_warnings.push_back(
          strfmt("View figure %s is not attached to any database table and was deleted.", figure->name().c_str()));
        orphan = true;
      }
    } else if (figure.is_instance<workbench_physical_RoutineGroupFigure>()) {
      if (!workbench_physical_RoutineGroupFigureRef::cast_from(figure)->routineGroup().is_valid()) {
        load_warnings.push_back(strfmt("Routine Group figure %s is not attached to any database table and was deleted.",
                                       figure->name().c_str()));
        orphan = true;
      }
    }

    if (orphan) {
      // delete the figure from the view
      view->removeFigure(figure);
    }
  }
}

static void fix_broken_foreign_keys(XMLTraverser &traverser, std::list<std::string> &load_warnings) {
  std::vector<xmlNodePtr> nodes(traverser.scan_objects_of_type("db.mysql.ForeignKey"));

  for (std::vector<xmlNodePtr>::iterator node = nodes.begin(); node != nodes.end(); ++node) {
    xmlNodePtr columns = traverser.get_object_child(*node, "columns");
    xmlNodePtr refcolumns = traverser.get_object_child(*node, "referencedColumns");
    xmlNodePtr fk_name_node = traverser.get_object_child(*node, "name");
    std::string fk_name = "???";
    if (fk_name_node)
      fk_name = (const char *)xmlString(xmlNodeGetContent(fk_name_node));

    xmlNodePtr rnext, rn = first_xml_element(refcolumns->children);
    xmlNodePtr next, n = first_xml_element(columns->children);
    for (; n != NULL && rn != NULL; n = next, rn = rnext) {
      next = first_xml_element(n->next);
      rnext = first_xml_element(rn->next);

      if (xmlStrcmp(n->name, (xmlChar *)"null") == 0 || xmlStrcmp(rn->name, (xmlChar *)"null") == 0) {
        std::string msg = strfmt("An invalid column reference in the Foreign Key '%s' was deleted.", fk_name.c_str());

        load_warnings.push_back(msg);

        xmlUnlinkNode(n);
        xmlUnlinkNode(rn);

        xmlFreeNode(n);
        xmlFreeNode(rn);
      } else {
        assert(xmlStrcmp(n->name, (xmlChar *)"value") == 0 || xmlStrcmp(n->name, (xmlChar *)"link") == 0 ||
               xmlStrcmp(n->name, (xmlChar *)"null") == 0);
        assert(xmlStrcmp(rn->name, (xmlChar *)"value") == 0 || xmlStrcmp(rn->name, (xmlChar *)"link") == 0 ||
               xmlStrcmp(rn->name, (xmlChar *)"null") == 0);
      }
    }

    if (n != NULL || rn != NULL)
      load_warnings.push_back(
        strfmt("Foreign Key %s has an invalid column definition. The invalid values were removed.", fk_name.c_str()));

    // if column lists are not the same size, trim them
    for (; n != NULL; n = next) {
      next = first_xml_element(n->next);

      xmlUnlinkNode(n);
      xmlFreeNode(n);
    }
    for (; rn != NULL; rn = rnext) {
      rnext = first_xml_element(rn->next);

      xmlUnlinkNode(rn);
      xmlFreeNode(rn);
    }
  }
}

static int fix_duplicate_uuid_bug(xmlNodePtr node, std::map<std::string, std::string> &object_types,
                                  std::map<std::string, std::map<std::string, std::string> > &remapped_ids) {
  xmlNodePtr n;
  int fixes = 0;

  for (n = node->children; n; n = n->next) {
    if (n->type == XML_ELEMENT_NODE) {
      if (strcmp((char *)n->name, "value") == 0 && XMLTraverser::node_prop(n, "type") == "object") {
        std::string id = XMLTraverser::node_prop(n, "id");
        std::string sname = XMLTraverser::node_prop(n, "struct-name");
        std::map<std::string, std::string>::iterator it = object_types.find(id);
        if (it != object_types.end()) {
          std::string remapped = strfmt("%s%i", id.c_str(), (int)remapped_ids.size());
          if (remapped_ids.find(id) != remapped_ids.end() || remapped_ids[id].find(sname) != remapped_ids[id].end())
            logWarning(
              "Found more than 1 duplicate entry for same UUID, will be unable to correctly fix file (%s %s)\n",
              sname.c_str(), id.c_str());
          if (remapped_ids[id].size() == 0) {
            // add the 1st found object
            remapped_ids[id][object_types[id]] = id;
          }
          remapped_ids[id][sname] = remapped;
          // g_message("changed duplicate %s %s to %s", id.c_str(), sname.c_str(), remapped.c_str());
          xmlSetProp(n, (const xmlChar *)"id", (const xmlChar *)remapped.c_str());
          fixes++;
        } else {
          object_types[id] = sname;
        }
      }
      fixes += fix_duplicate_uuid_bug(n, object_types, remapped_ids);
    }
  }
  return fixes;
}

static void fix_duplicate_uuid_bug_references(
  xmlNodePtr node, std::map<std::string, std::map<std::string, std::string> > &remapped_ids) {
  xmlNodePtr n;

  for (n = node->children; n; n = n->next) {
    if (n->type == XML_ELEMENT_NODE) {
      if (strcmp((char *)n->name, "link") == 0 && XMLTraverser::node_prop(n, "type") == "object") {
        xmlChar *content = xmlNodeGetContent(n);
        std::string id = (char *)content ? (char *)content : "";
        xmlFree(content);
        std::string sname = XMLTraverser::node_prop(n, "struct-name");
        std::map<std::string, std::map<std::string, std::string> >::iterator it = remapped_ids.find(id);
        if (it != remapped_ids.end()) {
          if (sname.empty()) {
            if (XMLTraverser::node_prop(node, "type") == "list" &&
                XMLTraverser::node_prop(node, "content-type") == "object" &&
                !(sname = XMLTraverser::node_prop(node, "content-struct-name")).empty()) {
              logInfo("link %s is in list of %s\n", id.c_str(), sname.c_str());
            } else {
              logInfo("link %s has no struct-name\n", id.c_str());
              continue;
            }
          }
          //        retry: not used
          if (it->second.find(sname) == it->second.end()) {
            bool ok = false;
            std::map<std::string, std::string> structs = it->second;
            std::string target_key;

            // handle refs to owner, which some times have a generic type like GrtObject so they're very ambiguous
            if ((target_key = XMLTraverser::node_prop(n, "key")) == "owner") {
              if (sname == "GrtObject") {
                xmlNodePtr gparent = node->parent;
                while (gparent) {
                  if (XMLTraverser::node_prop(gparent, "type") == "object") {
                    // g_message("%s %s -> %s (%s)", id.c_str(), sname.c_str(), XMLTraverser::node_prop(gparent,
                    // "struct-name").c_str(),
                    //          XMLTraverser::node_prop(gparent, "id").c_str());
                    sname = XMLTraverser::node_prop(gparent, "struct-name");
                    break;
                  }
                  gparent = gparent->parent;
                }
              }
            }

            // go through the alternatives checking their superclasses
            int count = 0;
            for (std::map<std::string, std::string>::const_iterator opt = structs.begin(); opt != structs.end();
                 ++opt) {
              grt::MetaClass *mc = grt::GRT::get()->get_metaclass(opt->first);
              while (mc) {
                if (mc->name() == sname) {
                  count++;
                  logInfo("found match %i %s for %s:%s (%s -> %s)\n", count, opt->first.c_str(), sname.c_str(),
                          target_key.c_str(), id.c_str(), opt->second.c_str());
                  xmlNodeSetContent(n, (xmlChar *)opt->second.c_str());
                  ok = true;
                  break;
                }
                mc = mc->parent();
              }
            }

            if (!ok)
              logWarning(
                "WARNING: Could not resolve link for %s %s (%i) key='%s'. Report to the developers to get a fix.\n",
                id.c_str(), sname.c_str(), (int)structs.size(), XMLTraverser::node_prop(n, "key").c_str());
          } else
            xmlNodeSetContent(n, (xmlChar *)it->second[sname].c_str());
        }
      }
      fix_duplicate_uuid_bug_references(n, remapped_ids);
    }
  }
}

bool ModelFile::check_and_fix_duplicate_uuid_bug(xmlDocPtr xmldoc) {
  if (XMLTraverser::node_prop(xmlDocGetRootElement(xmldoc), "version") == "1.4.1" ||
      XMLTraverser::node_prop(xmlDocGetRootElement(xmldoc), "version") == "1.4.2") {
    // in 5.2.32, the UUID generator was replaced with a boost implementation.
    // for some reason, that caused duplicate UUIDs to be generated, which screws up with saving/loading
    // of models. So if a model fails because of that, we go through the whole XML,
    // find objects with duplicate ids and tweak these id's to be unique based on their id + class pair.
    // the recovery won't be perfect because if 2 objects that are of the same class have the same id,
    // it will not be possible to accurately match references to them.

    std::map<std::string, std::string> object_types;
    std::map<std::string, std::map<std::string, std::string> > remapped_ids;
    if (fix_duplicate_uuid_bug(xmlDocGetRootElement(xmldoc), object_types, remapped_ids)) {
      fix_duplicate_uuid_bug_references(xmlDocGetRootElement(xmldoc), remapped_ids);
      return true;
    }
  }
  return false;
}

static void check_schema_objects(db_SchemaRef schema, std::list<std::string> &load_warnings) {
  // check for indexes that have a null referencedColumn value

  GRTLIST_FOREACH(db_Table, schema->tables(), table) {
    GRTLIST_FOREACH(db_Index, (*table)->indices(), index) {
      grt::ListRef<db_IndexColumn> icolumns((*index)->columns());
      for (size_t i = icolumns.count(); i > 0; --i) {
        db_IndexColumnRef column(icolumns[i - 1]);
        if (!column->referencedColumn().is_valid()) {
          load_warnings.push_back(
            strfmt("Index %s.%s has an invalid column reference. The reference was removed from the index.",
                   (*table)->name().c_str(), (*index)->name().c_str()));

          icolumns.remove_value(column);
        }
      }
    }

    /* this is not done in the FK check
    GRTLIST_FOREACH(db_ForeignKey, (*table)->foreignKeys(), fk)
    {
      if ((*fk)->columns().count() != (*fk)->referencedColumns().count())
      {
        load_warnings.push_back(strfmt("Foreign Key %s has an invalid column definition. The invalid values were
    removed.",
                                       (*fk)->name().c_str()));

        while ((*fk)->columns().count() > (*fk)->referencedColumns().count())
          (*fk)->columns().remove((*fk)->columns().count()-1);
        while ((*fk)->columns().count() < (*fk)->referencedColumns().count())
          (*fk)->referencedColumns().remove((*fk)->referencedColumns().count()-1);
      }
    }
     */
  }
}

void ModelFile::check_and_fix_inconsistencies(xmlDocPtr xmldoc, const std::string &version) {
  std::vector<std::string> ver = base::split(version, ".");

  int major = base::atoi<int>(ver[0], 0);

  XMLTraverser traverser(xmldoc);

  if (major == 1) {
    // check foreign key definitions for NULL values in the list of columns
    fix_broken_foreign_keys(traverser, _load_warnings);
  }
}

void ModelFile::check_and_fix_inconsistencies(const workbench_DocumentRef &doc, const std::string &version) {
  grt::ListRef<workbench_physical_Model> models(doc->physicalModels());

  for (size_t c = models.count(), i = 0; i < c; i++) {
    workbench_physical_ModelRef model(models[i]);
    for (size_t vc = model->diagrams().count(), v = 0; v < vc; v++) {
      workbench_physical_DiagramRef diagram(model->diagrams()[v]);

      // make sure that all figures are in their correct layer
      // check for orphaned (no db object) figures
      check_figure_layers(diagram, _load_warnings);
    }

    db_CatalogRef catalog(model->catalog());
    for (size_t sc = catalog->schemata().count(), s = 0; s < sc; s++) {
      db_SchemaRef schema(catalog->schemata()[s]);

      // check for orphaned index columns
      check_schema_objects(schema, _load_warnings);
    }
  }
}
