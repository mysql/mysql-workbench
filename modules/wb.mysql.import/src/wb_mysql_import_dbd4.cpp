/* 
 * Copyright (c) 2004, 2014, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef _WIN32
#include <cctype>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#endif

#include <glib/gstdio.h>

#include "wb_mysql_import_dbd4.h"
#include "grt/grt_manager.h"
#include <grtpp_util.h>
#include "tinyxml.h"
#include "base/log.h"
#include "grtdb/db_object_helpers.h"
#include "stream_conv.h"
#include "grtsqlparser/module_utils.h"
#include "sqlide/table_inserts_loader_be.h"
#include "grts/structs.workbench.h"
#include "base/string_utilities.h"

DEFAULT_LOG_DOMAIN("dbd4import");

using namespace base;

const float XSCALE_FACTOR= 1.0F;
const float YSCALE_FACTOR= 1.33F;
const int DEFAULT_MODEL_BASE_WIDTH= 10000;
const int DEFAULT_MODEL_BASE_HEIGHT= 7000;

#define NEUTRAL_STATE_KEEPER Neutral_state_keeper _nsk(this);

typedef std::map<int, model_LayerRef> Layers;


/*
*********************************************************************************
* @brief Converts a DBDesigner4 string to a utf8 string.
*
* Performs the following conversion for a string loaded from a DBD4 xml file:
*  - converts \n to newline
*  - converts \t to tab
*  - converts \a to '
*  - converts \\ to \
*  - converts \nnn from latin1 to UTF8 character
*
*  @param str
*
*  @return UTF8 string
*********************************************************************************
*/
static std::string dbd_string_to_utf8(const char *str)
{
  if (!str)
    return "";

  std::string res;
  gchar *tmp= (gchar*)g_malloc((int)strlen(str)*4+1);
  const char *ptr;
  gchar *tmpstr;

  tmpstr= tmp;
  ptr= str;
  while (*ptr)
  {
    if (*ptr == '\\')
    {
      ++ptr;
      switch (*ptr)
      {
      case 0:
        --ptr;
        break;
      case '\\':
        *tmpstr= '\\';
        break;
      case 'n':
      case 'N':
        *tmpstr= '\n';
        break;
      case 't':
      case 'T':
        *tmpstr= '\t';
        break;
      case 'r':
      case 'R':
        *tmpstr= '\r';
        break;
      case 'a':
      case 'A':
        *tmpstr= '\'';
        break;
      default:
        if (isdigit(ptr[0]) && isdigit(ptr[1]) && isdigit(ptr[2]))
        {
          char num[4];
          num[0]= ptr[0];
          num[1]= ptr[1];
          num[2]= ptr[2];
          num[3]= 0;
          *tmpstr= (gchar)base::atoi<int>(num, 0);
          ptr+= 2;
        }
        else
          *tmpstr= *ptr;
        break;
      }
    }
    else
      *tmpstr= *ptr;
    ++tmpstr;
    ++ptr;
  }
  *tmpstr= 0;

  tmpstr= g_convert(tmp, (gssize)strlen(tmp), "UTF-8", "ISO-8859-1", NULL, NULL, NULL);
  g_free(tmp);
  if (!tmpstr)
    return str;
  res= tmpstr;
  g_free(tmpstr);
  return res;
}


struct Rect
{
  double left;
  double top;
  double width;
  double height;
};


void replace_terminated_symbol(std::string &str, const std::string &term, const std::string &replace_string)
{
  char term_prefix= term[0];
  char term_symbol= term[1];
  bool terminated= false;
  for (size_t n= 0; n < str.length(); ++n)
  {
    char chr= str[n];

    if (terminated && chr == term_symbol)
    {
      --n;
      str= str.replace(n, term.length(), replace_string);
      terminated= false;
    }
    else if (chr == term_prefix)
      terminated= !terminated;
    else
      terminated= false;
  }
}


void split_string(const std::string &str, const std::string &delim, std::vector<std::string> &result)
{
  std::string::const_iterator i= str.begin();

  while (true)
  {
    std::string::const_iterator i2= search(i, str.end(), delim.begin(), delim.end());
    size_t substr_length= std::distance(i, i2);
    if (str.end() == i2 && !substr_length)
      break;
    std::string substr;
    substr.resize(substr_length);
    std::copy(i, i2, substr.begin());
    result.push_back(substr);
    i= i2;
    if (str.end() != i2)
      i+= delim.size();
  }
}


void parse_table_options(db_mysql_TableRef &table, const std::string &optionsstr)
{
  std::vector<std::string> options;
  split_string(optionsstr, "\\n", options);
  for (std::vector<std::string>::iterator i= options.begin(); i != options.end(); ++i)
  {
    std::vector<std::string> option_pair;
    split_string(*i, "=", option_pair);
    const std::string &option_name= option_pair[0];
    const char *option_val= option_pair[1].c_str();

    if (0 == option_name.compare("DelayKeyTblUpdates"))
      table->delayKeyWrite(base::atoi<int>(option_val, 0));
    else if (0 == option_name.compare("PackKeys"))
      table->packKeys(option_val);
    else if (0 == option_name.compare("RowChecksum"))
      table->checksum(base::atoi<int>(option_val, 0));
    else if (0 == option_name.compare("RowFormat"))
    {
      // ROW_FORMAT [=] {DEFAULT|DYNAMIC|FIXED|COMPRESSED|REDUNDANT|COMPACT}
      // REDUNDANT and COMPACT values are not supported by DBD4
      int n= 0;
      {
        std::istringstream iss(option_val);
        iss >> n;
      }
      switch (n)
      {
      case 1: option_val= "DYNAMIC"; break;
      case 2: option_val= "FIXED"; break;
      case 3: option_val= "COMPRESSED"; break;
      default: option_val= "DEFAULT"; break;
      }
      table->rowFormat(option_val);
    }
    else if (0 == option_name.compare("AverageRowLength"))
      table->avgRowLength(option_val);
    else if (0 == option_name.compare("MaxRowNumber"))
      table->maxRows(option_val);
    else if (0 == option_name.compare("MinRowNumber"))
      table->minRows(option_val);
    else if (0 == option_name.compare("NextAutoIncVal"))
      table->nextAutoInc(option_val);
    else if (0 == option_name.compare("TblPassword"))
      table->password(option_val);
    else if (0 == option_name.compare("TblDataDir"))
      table->tableDataDir(option_val);
    else if (0 == option_name.compare("TblIndexDir"))
      table->tableIndexDir(option_val);
  }
}


void add_figure_on_layer(model_LayerRef layer, model_FigureRef figure)
{
  figure->visible(1);
  layer->figures().insert(figure);
  layer->owner()->figures().insert(figure); // add to view->figures()
}


model_LayerRef find_containing_layer(Rect &rect, Layers &layers)
{
  for (Layers::iterator i= layers.begin(); i != layers.end(); ++i)
  {
    model_LayerRef &layer= i->second;
    if (rect.left > layer->left()
      && (rect.left < layer->left() + layer->width())
      && (rect.top > layer->top())
      && (rect.top < layer->top() + layer->height())
      )
      return layer;
  }
  return model_LayerRef();
}


static bool calculate_view_size(const app_PageSettingsRef &page, double &width, double &height)
{
  if (page->paperType().is_valid())
  {
    width= page->paperType()->width();
    height= page->paperType()->height();

    width-= page->marginLeft() + page->marginRight();
    height-= page->marginTop() + page->marginBottom();

    width*= page->scale();
    height*= page->scale();

    if (page->orientation() == "landscape")
      std::swap(width, height);

    return true;
  }
  else
  {
    width= 1000;
    height= 1000;
    return false;
  }
}


Wb_mysql_import_DBD4::Wb_mysql_import_DBD4()
: _grt(NULL)
{
  NEUTRAL_STATE_KEEPER
}

int Wb_mysql_import_DBD4::import_DBD4(workbench_physical_ModelRef model, const char *file_name, grt::DictRef options)
{
  NEUTRAL_STATE_KEEPER

  overwrite_default_option<IntegerRef>(_gen_fk_names_when_empty, "gen_fk_names_when_empty", options);

  TiXmlDocument doc(file_name);
  bool loaded = doc.LoadFile();
  if (!loaded || !doc.FirstChildElement("DBMODEL"))
    throw std::runtime_error("Wrong file format.");

  _grt= model.get_grt();
  _catalog= db_mysql_CatalogRef::cast_from(model->catalog());

  log_info("Started import DBD4 model.\n");

  bec::GRTManager *grtm= bec::GRTManager::get_instance_for(_grt);

  _created_schemata= ListRef<db_mysql_Schema>(_grt);
  ensure_schema_created(0,
    (0 < _catalog->schemata().count()) ? _catalog->schemata().get(0)->name().c_str() : "mydb");

  const TiXmlElement *dbmodel_el= doc.FirstChildElement("DBMODEL");
  if (dbmodel_el)
  {
    int id= 0;

    DictRef wb_options= DictRef::cast_from(_grt->get("/wb/options/options"));
    std::string table_figure_color= wb_options.get_string(std::string(workbench_physical_TableFigure::static_class_name()).append(":Color"));
    std::string note_figure_color=  wb_options.get_string(std::string(workbench_model_NoteFigure::static_class_name()).append(":Color"));
    std::string image_color_color= wb_options.get_string(std::string(workbench_model_ImageFigure::static_class_name()).append(":Color"));

    // view initialization
    workbench_physical_DiagramRef view;
    {
      workbench_WorkbenchRef wb(workbench_WorkbenchRef::cast_from(model->owner()->owner()));
      double viewWidth;
      double viewHeight;

      view= workbench_physical_DiagramRef::cast_from(model->addNewDiagram(1));

      calculate_view_size(wb->doc()->pageSettings(), viewWidth, viewHeight);

      const TiXmlElement *settings_el= dbmodel_el->FirstChildElement("SETTINGS");
      if (settings_el)
      {
        const TiXmlElement *global_settings_el= settings_el->FirstChildElement("GLOBALSETTINGS");
        if (global_settings_el)
        {
          int w= 0;
          int h= 0;
          global_settings_el->QueryIntAttribute("CanvasWidth", &w);
          global_settings_el->QueryIntAttribute("CanvasHeight", &h);

          if (!w) w = DEFAULT_MODEL_BASE_WIDTH;
          if (!h) h = DEFAULT_MODEL_BASE_HEIGHT;

          // align with page size (view size should be a whole number of pages)
          div_t divres= div((int)(w * XSCALE_FACTOR), (int)viewWidth);
          viewWidth= divres.quot * viewWidth + (divres.rem ? viewWidth : 0);
          divres= div((int)(h * YSCALE_FACTOR), (int)viewHeight);
          viewHeight= divres.quot * viewHeight + (divres.rem ? viewHeight : 0);
        }
      }

      std::string name_prefix= "DBD4 Model";
      /// std::string view_class_name= base::replaceString(model.class_name(), ".Model", ".View");
      std::string name= grt::get_name_suggestion_for_list_object(
        grt::ObjectListRef::cast_from(model->diagrams()),
        name_prefix, false);

      ///view= _grt->create_object<workbench_physical_View>(view_class_name);
      ///view->owner(model);
      view->name(grt::StringRef(name));
      view->width(grt::DoubleRef(viewWidth));
      view->height(grt::DoubleRef(viewHeight));
      ///view->zoom(grt::DoubleRef(1));

      ///do_transactable_list_insert(_undo_man, model->views(), view);
    }

    db_mgmt_RdbmsRef rdbms= model->rdbms();

    // settings
    std::map<int, std::string> region_colors;
    const TiXmlElement *settings_el= dbmodel_el->FirstChildElement("SETTINGS");
    if (settings_el)
    {
      // DBD4 model datatypes mapping
      const TiXmlElement *datatypes_el= settings_el->FirstChildElement("DATATYPES");
      {
        if (datatypes_el)
        {
          const TiXmlElement *datatype_el= datatypes_el->FirstChildElement("DATATYPE");
          while (datatype_el)
          {
            datatype_el->QueryIntAttribute("PhysicalMapping", &id);
            int is_datatype_synonym= id;
            int datatype_id;
            datatype_el->QueryIntAttribute("ID", &datatype_id);
            const char *attr_name= (is_datatype_synonym ? "PhysicalTypeName" : "TypeName");
            const char *type_name= datatype_el->Attribute(attr_name);

            _datatypes[datatype_id]= type_name;
            if (!is_datatype_synonym)
            {
              _datatypes_revind[type_name]= datatype_id;

              const TiXmlElement *options_el= datatype_el->FirstChildElement("OPTIONS");
              if (options_el)
              {
                std::list<Simple_type_flag> flags;

                const TiXmlElement *option_el= options_el->FirstChildElement("OPTION");
                while (option_el)
                {
                  Simple_type_flag flag;
                  flag.name= option_el->Attribute("Name");
                  option_el->QueryIntAttribute("Default", &id);
                  flag.default_val= id;
                  flags.push_back(flag);

                  option_el= option_el->NextSiblingElement();
                }

                _datatypes_flags[datatype_id]= flags;
              }
            }
            else
              _datatypes_flags[datatype_id]= _datatypes_flags[_datatypes_revind[type_name]];

            datatype_el= datatype_el->NextSiblingElement();
          }
        }
      }

      // cache schemata
      const TiXmlElement *table_prefixes_el= settings_el->FirstChildElement("TABLEPREFIXES");
      {
        if (table_prefixes_el)
        {
          const TiXmlElement *table_prefix_el= table_prefixes_el->FirstChildElement("TABLEPREFIX");

          // skip default schema (use mydb instead)
          if (table_prefix_el)
            table_prefix_el= table_prefix_el->NextSiblingElement();

          log_info("Schemata:\n");
          while (table_prefix_el)
          {
            std::string schema_name= dbd_string_to_utf8(table_prefix_el->Attribute("Name"));
            log_info("...%s\n", schema_name.c_str());

            ensure_schema_created((int)_schemata.size(), schema_name.c_str());

            table_prefix_el= table_prefix_el->NextSiblingElement();
          }
        }
      }

      // colors
      const TiXmlElement *regions_colors_el= settings_el->FirstChildElement("REGIONCOLORS");
      {
        if (regions_colors_el)
        {
          int id= 0;
          const TiXmlElement *regions_color_el= regions_colors_el->FirstChildElement("REGIONCOLOR");
          while (regions_color_el)
          {
            std::string color= regions_color_el->Attribute("Color");
            std::vector<std::string> color_pair;
            split_string(color, "=", color_pair);
            region_colors[id]= (2 == color_pair.size()) ? color_pair[1] : std::string();

            ++id;
            regions_color_el= regions_color_el->NextSiblingElement();
          }
        }
      }
    }

    // metadata
    const TiXmlElement *metadata_el= dbmodel_el->FirstChildElement("METADATA");
    if (metadata_el)
    {
      // regions
      Layers layers;
      const TiXmlElement *regions_el= metadata_el->FirstChildElement("REGIONS");
      {
        if (regions_el)
        {
          log_info("Layers:\n");

          const TiXmlElement *region_el= regions_el->FirstChildElement("REGION");
          while (region_el)
          {
            std::string layer_name= dbd_string_to_utf8(region_el->Attribute("RegionName"));
            log_info("...%s\n", layer_name.c_str());

            model_LayerRef layer= workbench_physical_LayerRef(_grt);
            layer->owner(view);
            layer->name(layer_name);

            region_el->QueryIntAttribute("XPos", &id);
            layer->left(id*XSCALE_FACTOR);
            region_el->QueryIntAttribute("YPos", &id);
            layer->top(id*YSCALE_FACTOR);
            region_el->QueryIntAttribute("Width", &id);
            layer->width(id*XSCALE_FACTOR);
            region_el->QueryIntAttribute("Height", &id);
            layer->height(id*YSCALE_FACTOR);
            region_el->QueryIntAttribute("RegionColor", &id);
            layer->color(region_colors[id]);
            layer->description(dbd_string_to_utf8(region_el->Attribute("Comments")));

            view->layers().insert(layer);

            region_el->QueryIntAttribute("ID", &id);
            layers[id]= layer;

            region_el= region_el->NextSiblingElement();
          }
        }
      }

      // tables
      const TiXmlElement *tables_el= metadata_el->FirstChildElement("TABLES");
      {
        if (tables_el)
        {
          log_info("Tables:\n");

          TableInsertsLoader table_inserts_loader(grtm);

          const TiXmlElement *table_el= tables_el->FirstChildElement("TABLE");
          while (table_el)
          {
            std::string table_name= dbd_string_to_utf8(table_el->Attribute("Tablename"));
            log_info("...%s\n", table_name.c_str());

            // table
            table_el->QueryIntAttribute("TablePrefix", &id);
            db_mysql_SchemaRef schema= _schemata[id];

            db_mysql_TableRef table(_grt);
            table->owner(schema);

            table->name(table_name);
            table_el->QueryIntAttribute("Temporary", &id);
            table->isTemporary(id);
            table->comment(dbd_string_to_utf8(table_el->Attribute("Comments")));
            parse_table_options(table, table_el->Attribute("TableOptions"));

            table_el->QueryIntAttribute("ID", &id);
            _tables[id]= table;

            // columns
            const TiXmlElement *columns_el= table_el->FirstChildElement("COLUMNS");
            if (columns_el)
            {
              ListRef<db_mysql_Column> columns= table->columns();
              const TiXmlElement *column_el= columns_el->FirstChildElement("COLUMN");
              while(column_el)
              {
                db_mysql_ColumnRef column(_grt);
                column->owner(table);

                column->name(dbd_string_to_utf8(column_el->Attribute("ColName")));
                column_el->QueryIntAttribute("NotNull", &id);
                column->isNotNull(id);
                column_el->QueryIntAttribute("AutoInc", &id);
                column->autoIncrement(id);
                column->comment(dbd_string_to_utf8(column_el->Attribute("Comments")));
                bec::ColumnHelper::set_default_value(column, 
                  dbd_string_to_utf8(column_el->Attribute("DefaultValue")));
                column->datatypeExplicitParams(column_el->Attribute("DatatypeParams"));

                column_el->QueryIntAttribute("idDatatype", &id);
                std::string typestr= _datatypes[id];
                // Fixes a glitch in the DBD4 Model tab, that doesn't recognize
                // "INTEGER" as an alias for "INT"
                if (typestr == "INTEGER")
                  typestr= "INT(11)";
                std::list<Simple_type_flag> flags= _datatypes_flags[id];
                // append datatype params to type name
                typestr.append(dbd_string_to_utf8(column_el->Attribute("DatatypeParams")));
                if (!column->setParseType(typestr, rdbms->simpleDatatypes()))
                  _grt->send_warning(strfmt("Error parsing type for column '%s' (%s)", column->name().c_str(), typestr.c_str()));

                // flags
                std::list<int> flags2;
                const TiXmlElement *optionselected_el= column_el->FirstChildElement("OPTIONSELECTED");
                if (optionselected_el)
                {
                  const TiXmlElement *optionselect_el= optionselected_el->FirstChildElement("OPTIONSELECT");
                  while (optionselect_el)
                  {
                    optionselect_el->QueryIntAttribute("Value", &id);
                    flags2.push_back(id);

                    optionselect_el= optionselect_el->NextSiblingElement();
                  }
                }
                {
                  std::list<Simple_type_flag>::const_iterator i= flags.begin();
                  std::list<int>::const_iterator i2= flags2.begin();
                  for (; i != flags.end(); ++i)
                  {
                    int flag_value= (flags2.empty() ? i->default_val : *i2);
                    if (flag_value)
                      column->flags().insert(i->name);
                    if (!flags2.empty())
                      ++i2;
                  }
                }

                columns.insert(column);
                column_el->QueryIntAttribute("ID", &id);
                _columns[id]= column;

                column_el= column_el->NextSiblingElement();
              }
            }

            // indices
            const TiXmlElement *indices_el= table_el->FirstChildElement("INDICES");
            if (indices_el)
            {
              ListRef<db_mysql_Index> indices= table->indices();
              const TiXmlElement *index_el= indices_el->FirstChildElement("INDEX");
              while(index_el)
              {
                db_mysql_IndexRef index(_grt);
                index->owner(table);

                index->name(dbd_string_to_utf8(index_el->Attribute("IndexName")));

                // index->indexType
                index_el->QueryIntAttribute("IndexKind", &id);
                if (0 == id || (*index->name()).compare("PRIMARY") == 0)
                {
                  index->indexType("PRIMARY");
                  index->isPrimary(1);
                  index->unique(1);
                  table->primaryKey(index);
                }
                else if (2 == id)
                  index->indexType("UNIQUE");
                else if (3 == id)
                  index->indexType("FULLTEXT");
                else
                  index->indexType("INDEX");

                // index columns
                const TiXmlElement *index_columns_el= index_el->FirstChildElement("INDEXCOLUMNS");
                if (index_columns_el)
                {
                  ListRef<db_mysql_IndexColumn> index_columns= index->columns();
                  const TiXmlElement *index_column_el= index_columns_el->FirstChildElement("INDEXCOLUMN");
                  while(index_column_el)
                  {
                    db_mysql_IndexColumnRef index_column(_grt);
                    index_column->owner(index);

                    index_column_el->QueryIntAttribute("idColumn", &id);
                    index_column->referencedColumn(_columns[id]);
                    index_column_el->QueryIntAttribute("LengthParam", &id);
                    index_column->columnLength(id);

                    index_columns.insert(index_column);

                    index_column_el= index_column_el->NextSiblingElement();
                  }
                }

                indices.insert(index);

                index_el= index_el->NextSiblingElement();
              }
            }

            // table inserts
            {
              std::string inserts_script= dbd_string_to_utf8(table_el->Attribute("StandardInserts"));
              try
              {
                table_inserts_loader.process_table(table, inserts_script);
              }
              catch (const std::exception &exc)
              {
                _grt->send_error("Import DBD4 Model", base::strfmt("Error processing inserts importing DBD4 model %s: %s", exc.what(), inserts_script.c_str()));
                throw;
              }
            }
            schema->tables().insert(table);

            // table figure
            Rect rect;
            table_el->QueryIntAttribute("XPos", &id);
            rect.left= id*XSCALE_FACTOR;
            table_el->QueryIntAttribute("YPos", &id);
            rect.top= id*YSCALE_FACTOR;

            model_LayerRef layer= find_containing_layer(rect, layers);
            if (!layer.is_valid())
              layer= view->rootLayer();
            else
            {
              rect.left-= layer->left();
              rect.top-= layer->top();
            }

            workbench_physical_TableFigureRef table_figure= workbench_physical_TableFigureRef(_grt);
            table_figure->owner(view);
            table_figure->layer(layer);
            table_figure->left(rect.left);
            table_figure->top(rect.top);
            table_figure->table(table);
            table_figure->color(table_figure_color);

            table_el->QueryIntAttribute("Collapsed", &id);
            table_figure->expanded(abs(id-1));

            add_figure_on_layer(layer, table_figure);

            table_el->QueryIntAttribute("ID", &id);
            _table_figures[id]= table_figure;

            table_el= table_el->NextSiblingElement();
          }
        }
      }

      // relations / fkeys
      const TiXmlElement *relations_el= metadata_el->FirstChildElement("RELATIONS");
      {
        if (relations_el)
        {
          log_info("Connections:\n");

          const TiXmlElement *relation_el= relations_el->FirstChildElement("RELATION");
          while (relation_el)
          {
            std::string relation_name= dbd_string_to_utf8(relation_el->Attribute("RelationName"));
            log_info("...%s\n", relation_name.c_str());

            relation_el->QueryIntAttribute("DestTable", &id);
            db_mysql_TableRef srcTable= _tables[id];
            relation_el->QueryIntAttribute("SrcTable", &id);
            db_mysql_TableRef destTable= _tables[id];

            db_mysql_ForeignKeyRef fkey(_grt);
            fkey->owner(srcTable);

            /*
            real insert of fkey is occured after relation figure is added to model.
            that's needed to disable autocreation of relation.
            */
            //srctable->foreignKeys().insert(fkey);

            // connection figure
            workbench_physical_ConnectionRef conn_figure(_grt);
            conn_figure->owner(view);

            relation_el->QueryIntAttribute("DestTable", &id);
            workbench_physical_TableFigureRef start_table_figure= _table_figures[id];
            relation_el->QueryIntAttribute("SrcTable", &id);
            workbench_physical_TableFigureRef end_table_figure= _table_figures[id];

            conn_figure->startFigure(start_table_figure);
            conn_figure->endFigure(end_table_figure);

            conn_figure->foreignKey(fkey);

            conn_figure->name(dbd_string_to_utf8(relation_el->Attribute("RelationName")));
            conn_figure->caption(dbd_string_to_utf8(relation_el->Attribute("RelationName")));
            conn_figure->comment(dbd_string_to_utf8(relation_el->Attribute("Comments")));
            relation_el->QueryIntAttribute("CaptionOffsetX", &id);
            conn_figure->captionXOffs(id);
            relation_el->QueryIntAttribute("CaptionOffsetY", &id);
            conn_figure->captionYOffs(id);
            relation_el->QueryIntAttribute("StartIntervalOffsetX", &id);
            conn_figure->startCaptionXOffs(id);
            relation_el->QueryIntAttribute("StartIntervalOffsetY", &id);
            conn_figure->startCaptionYOffs(id);
            relation_el->QueryIntAttribute("EndIntervalOffsetX", &id);
            conn_figure->endCaptionXOffs(id);
            relation_el->QueryIntAttribute("EndIntervalOffsetY", &id);
            conn_figure->endCaptionYOffs(id);

            relation_el->QueryIntAttribute("Invisible", &id);
            conn_figure->visible(abs(id-1));

            relation_el->QueryIntAttribute("Splitted", &id);
            conn_figure->drawSplit(id);

            view->connections().insert(conn_figure);

            /*
            now connection figure auto-creation is disabled.
            continue with fkey.
            */
            fkey->referencedTable(destTable);
            if (_gen_fk_names_when_empty)
            {
              std::string name= bec::TableHelper::generate_foreign_key_name();
              fkey->name(name);
            }
            else
              fkey->name(relation_name); // for tests only (to keep names constant, hence comparable)
            fkey->comment(dbd_string_to_utf8(relation_el->Attribute("Comments")));

            std::string fk_fields= relation_el->Attribute("FKFields");
            std::vector<std::string> field_pairs;
            split_string(fk_fields, "\\n", field_pairs);
            for (std::vector<std::string>::iterator i= field_pairs.begin(); i != field_pairs.end(); ++i)
            {
              std::vector<std::string> field_pair;
              split_string(*i, "=", field_pair);
              {
                db_mysql_ColumnRef column= find_named_object_in_list(destTable->columns(), field_pair[0].c_str(), false);
                if (!column.is_valid())
                {
                  column= db_mysql_ColumnRef(_grt);
                  column->owner(destTable);
                  column->name(field_pair[0]);
                  destTable->columns().insert(column);
                }
                fkey->referencedColumns().insert(column);
              }
              {
                db_mysql_ColumnRef column= find_named_object_in_list(srcTable->columns(), field_pair[1].c_str(), false);
                if (!column.is_valid())
                {
                  column= db_mysql_ColumnRef(_grt);
                  column->owner(srcTable);
                  column->name(field_pair[1]);
                  srcTable->columns().insert(column);
                }
                fkey->columns().insert(column);
              }
            }

            relation_el->QueryIntAttribute("CreateRefDef", &id);
            if (1 == id)
            {
              std::string ref_def= relation_el->Attribute("RefDef");
              std::vector<std::string> ref_defs;
              split_string(ref_def, "\\n", ref_defs);
              for (std::vector<std::string>::iterator i= ref_defs.begin(); i != ref_defs.end(); ++i)
              {
                std::vector<std::string> ref_def_parts;
                split_string(*i, "=", ref_def_parts);
                {
                  if (ref_def_parts.size() != 2)
                    continue;

                  if (("OnUpdate" == ref_def_parts[0]) || ("OnDelete" == ref_def_parts[0]))
                  {
                    /*
                    on delete/update rule:
                    0 - restrict
                    1 - cascade
                    2 - set null
                    3 - no action
                    4 - set default
                    */
                    std::string rule;
                    int val;
                    {
                      std::stringstream ss;
                      ss << ref_def_parts[1];
                      ss >> val;
                    }
                    switch (val)
                    {
                    case 0: rule= "RESTRICT"; break;
                    case 1: rule= "CASCADE"; break;
                    case 2: rule= "SET NULL"; break;
                    case 3: rule= "NO ACTION"; break;
                    default: break;
                    }
                    if ("OnDelete" == ref_def_parts[0])
                      fkey->deleteRule(rule);
                    else if ("OnUpdate" == ref_def_parts[0])
                      fkey->updateRule(rule);
                  }
                }
              }
            }

            /*
            relation kind:
            0 - one_to_one
            1 - one_to_many
            2 - one_to_many non-identifying
            4 - arrow
            5 - one_to_one non-identifying
            */
            relation_el->QueryIntAttribute("Kind", &id);
            fkey->many((int)(1 == id || 2 == id));
            relation_el->QueryIntAttribute("OptionalEnd", &id);
            fkey->mandatory(abs(id-1));
            relation_el->QueryIntAttribute("OptionalStart", &id);
            fkey->referencedMandatory(abs(id-1));

            srcTable->foreignKeys().insert(fkey);

            relation_el= relation_el->NextSiblingElement();
          }
        }
      }

      // notes
      const TiXmlElement *notes_el= metadata_el->FirstChildElement("NOTES");
      {
        if (notes_el)
        {
          log_info("Notes:\n");

          const TiXmlElement *note_el= notes_el->FirstChildElement("NOTE");
          while (note_el)
          {
            std::string note_name= dbd_string_to_utf8(note_el->Attribute("NoteName"));
            log_info("...%s\n", note_name.c_str());

            workbench_model_NoteFigureRef note_figure(_grt);
            note_figure->owner(view);
            note_figure->layer(view->rootLayer());
            note_figure->name(note_name);

            note_el->QueryIntAttribute("XPos", &id);
            note_figure->left(id*XSCALE_FACTOR);
            note_el->QueryIntAttribute("YPos", &id);
            note_figure->top(id*YSCALE_FACTOR);
            note_figure->color(note_figure_color);
            note_figure->text(dbd_string_to_utf8(note_el->Attribute("NoteText")));

            add_figure_on_layer(view->rootLayer(), note_figure);

            note_el= note_el->NextSiblingElement();
          }
        }
      }

      // images
      const TiXmlElement *images_el= metadata_el->FirstChildElement("IMAGES");
      {
        if (images_el)
        {
          log_info("Images:\n");

          const std::string tmp_dir= grtm->get_unique_tmp_subdir();
          // crete temp dir
          {
            g_mkdir(grtm->get_tmp_dir().c_str(), 0700);
            g_mkdir(tmp_dir.c_str(), 0700);
          }

          const TiXmlElement *image_el= images_el->FirstChildElement("IMAGE");
          while (image_el)
          {
            image_el->QueryIntAttribute("IsLinkedObject", &id);
            std::string img_format= image_el->Attribute("ImgFormat");
            if ((img_format.compare("PNG") == 0) && (id == 0))
            {
              std::string image_name= dbd_string_to_utf8(image_el->Attribute("ImageName"));
              log_info("...%s\n", image_name.c_str());

              std::string filename;
              filename
                .append(tmp_dir)
                .append("/")
                .append(image_name)
                .append("(")
                .append(image_el->Attribute("ID"))
                .append(").png");

              {
                std::istringstream iss(image_el->Attribute("ImgData"));
                std::ofstream ofs(filename.c_str(), std::ios_base::out|std::ios_base::binary);
                stream_conv(iss, ofs, &unhex<2,char>);
              }

              workbench_model_ImageFigureRef image_figure(_grt);
              image_figure->owner(view);
              image_figure->layer(view->rootLayer());
              image_figure->name(image_name);

              image_el->QueryIntAttribute("XPos", &id);
              image_figure->left(id*XSCALE_FACTOR);
              image_el->QueryIntAttribute("YPos", &id);
              image_figure->top(id*YSCALE_FACTOR);

              image_el->QueryIntAttribute("Width", &id);
              image_figure->width(id);
              image_el->QueryIntAttribute("Height", &id);
              image_figure->height(id);

              image_figure->expanded(1);
              image_figure->color(image_color_color);

              add_figure_on_layer(view->rootLayer(), image_figure);

              image_figure->setImageFile(filename);
            }

            image_el= image_el->NextSiblingElement();
          }
        }
      }
    }
  }

  remove_unused_schemata();

  log_info("Finished import DBD4 model.\n");

  return 1; // success
}


db_mysql_SchemaRef Wb_mysql_import_DBD4::ensure_schema_created(int index, const char *name)
{
  ListRef<db_mysql_Schema> schemata= _catalog->schemata();

  db_mysql_SchemaRef schema= find_named_object_in_list(schemata, name, false);

  if (!schema.is_valid())
  {
    schema= db_mysql_SchemaRef(_grt);
    schema->owner(_catalog);
    schema->name(name);
    schemata.insert(schema);
    _created_schemata.insert(schema);
  }
  _schemata[index]= schema;

  return schema;
}


/*
removes schemata that were created during import but don't contain any object inside.
*/
void Wb_mysql_import_DBD4::remove_unused_schemata()
{
  for (size_t n= 0, count= _created_schemata.count(); n < count; ++n)
  {
    db_mysql_SchemaRef schema= _created_schemata.get(n);
    if (schema->tables().count() == 0
      && schema->views().count() == 0
      && schema->routines().count() == 0)
      _catalog->schemata().insert(schema);
  }
}
