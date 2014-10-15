/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "recordset_text_storage.h"
#include "recordset_be.h"
#include "base/string_utilities.h"
#include "base/file_functions.h"
#include "base/file_utilities.h"
#include "base/config_file.h"

#include <sqlite/query.hpp>
#include <boost/foreach.hpp>
#include <fstream>
#include <memory>
#include <errno.h>

#define WIN32 // required by ctemplate to compile on win

#include <ctemplate/template.h>

using namespace bec;
using namespace grt;
using namespace base;

using ctemplate::Template;
using ctemplate::TemplateDictionary;

typedef std::map<std::string, Recordset_text_storage::TemplateInfo> Templates;
static Templates _templates; // data format name -> template

static const Recordset_text_storage::TemplateInfo &template_info(const std::string &template_name)
{
  if (_templates.find(template_name) == _templates.end())
    throw std::invalid_argument(strfmt("Unknown template type name %s", template_name.c_str()));
  return _templates[template_name];
}



static void process_templates(const std::list<std::string> &files)
{
  for (std::list<std::string>::const_iterator f = files.begin(); f != files.end(); ++f)
  {
    ConfigurationFile cf(AutoCreateNothing);
    if (cf.load(*f))
    {
      Recordset_text_storage::TemplateInfo info;
      info.path = base::strip_extension(*f)+".tpl";
      info.name = base::strip_extension(base::basename(*f));
      info.extension = cf.get_value("extension");
      info.description = cf.get_value("description");
      info.pre_quote_strings = cf.get_bool("pre_quote_strings");
      info.quote = cf.get_value("quote");
      info.include_column_types = cf.get_value("include_column_types");
      info.null_syntax = cf.get_value("null_syntax");
      info.row_separator = cf.get_value("row_separator");
      if (info.include_column_types != "xls")
        info.include_column_types = "";
      std::string args = cf.get_value("arguments");
      std::vector<std::string> arguments = base::split_token_list(args, ';');
      for (std::vector<std::string>::const_iterator arg = arguments.begin(); arg != arguments.end(); ++arg)
      {
        std::string name, symbol;
        if (!arg->empty() && base::partition(*arg, ":", name, symbol))
          info.arguments.push_back(std::make_pair(base::unquote_identifier(name), symbol));
      }
      _templates[info.name] = info;
    }
  }
}

static void scan_templates(GRTManager *grtm)
{
  if (_templates.empty())
  {
    std::string template_dir = bec::make_path(grtm->get_basedir(), "modules/data/sqlide");
    std::list<std::string> files = base::scan_for_files_matching(template_dir+"/*.tpli");
    process_templates(files);
    
    template_dir = bec::make_path(grtm->get_user_datadir(), "recordset_export_templates");
    files = base::scan_for_files_matching(template_dir+"/*.tpli");
    process_templates(files);        
  }
}


#define APPEND(literal)  out->Emit("" literal "", sizeof(literal)-1)

// string escaper for CSV tokens, encloses fields with " if needed, depending on the separator
class CSVTokenQuote : public ctemplate::TemplateModifier {
  void Modify(const char* in, size_t inlen,
              const ctemplate::PerExpandData* per_expand_data,
              ctemplate::ExpandEmitter* out, const std::string& arg) const 
  {
    int ch;
    if (arg == "=comma")
      ch = ',';
    else if (arg == "=tab")
      ch = '\t';
    else if (arg == "=semicolon")
      ch = ';';
    else
      ch = ';';
    // check if quotation needed
    if (memchr(in, ch, inlen) || memchr(in, ' ', inlen) || memchr(in, '"', inlen)
        || memchr(in, '\t', inlen) || memchr(in, '\r', inlen) || memchr(in, '\n', inlen))
    {
      out->Emit(std::string("\""));
      for (size_t i = 0; i < inlen; ++i) {
        if (in[i] == '"') 
          APPEND("\"\"");
        else 
          out->Emit(in[i]);
      } 
      out->Emit(std::string("\""));
    }
    else
      out->Emit(std::string(in, inlen));
  }
};
CSVTokenQuote csv_quote;


Recordset_text_storage::Recordset_text_storage(GRTManager *grtm)
:
Recordset_data_storage(grtm)
{
  static bool registered_csvquote = false;
  if (!registered_csvquote)
  {
    registered_csvquote = true;
    ctemplate::AddModifier("x-csv_quote=", &csv_quote);
    //XXX ctemplate::AddModifier("x-sql_quote", &sql_quote);
  }
}


Recordset_text_storage::~Recordset_text_storage()
{
}


ColumnId Recordset_text_storage::aux_column_count()
{
  throw std::runtime_error("Recordset_text_storage::aux_column_count is not implemented");
}


void Recordset_text_storage::do_apply_changes(const Recordset *recordset, sqlite::connection *data_swap_db, bool skip_commitmig )
{
  throw std::runtime_error("Recordset_text_storage::apply_changes is not implemented");
}

static std::string escape_sql_string_(const std::string &s)
{
  return base::escape_sql_string(s, false);
}

void Recordset_text_storage::do_serialize(const Recordset *recordset, sqlite::connection *data_swap_db)
{
  const TemplateInfo &info(template_info(_data_format));
  std::string template_name(info.name);
  bool strings_are_pre_quoted(info.pre_quote_strings);
  std::string include_column_types(info.include_column_types);
  std::string null_syntax(info.null_syntax);
  std::string tpl_path(info.path);
  Template *pre_tpl = 0;
  Template *post_tpl = 0;
  Template *tpl= Template::GetTemplate(tpl_path, ctemplate::DO_NOT_STRIP);
  if (!tpl)
  {
    Template::ClearCache();
    throw std::runtime_error(strfmt("Failed to open template file: `%s`", tpl_path.c_str()));
  }
  
  // templates can be all in a single file or be divided in 3 files (pre, body and post)
  // to save memory when exporting large resultsets
  std::string pre_tpl_path;
  std::string post_tpl_path;
  if (g_str_has_suffix(tpl_path.c_str(), ".tpl"))
  {
    std::string name = tpl_path.substr(0, tpl_path.size()-4);
    if (g_file_test((name+".pre.tpl").c_str(), G_FILE_TEST_EXISTS))
    {
      pre_tpl_path = name+".pre.tpl";
      pre_tpl= Template::GetTemplate(pre_tpl_path, ctemplate::DO_NOT_STRIP);
      if (!pre_tpl)
        g_warning("Failed to open template file: `%s`", pre_tpl_path.c_str()); 
      else
        pre_tpl->ReloadIfChanged();
    }      
    if (g_file_test((name+".post.tpl").c_str(), G_FILE_TEST_EXISTS))
    {
      post_tpl_path = name+".post.tpl";
      post_tpl= Template::GetTemplate(post_tpl_path, ctemplate::DO_NOT_STRIP);
      if (!post_tpl)
        g_warning("Failed to open template file: `%s`", post_tpl_path.c_str());
      else
        post_tpl->ReloadIfChanged();
    }
  }    
  
  {
    if (!g_file_set_contents(_file_path.c_str(), "", 1, NULL))
      throw std::runtime_error(strfmt("Failed to open output file: `%s`", _file_path.c_str()));
  }

  tpl->ReloadIfChanged();

  std::auto_ptr<TemplateDictionary> dict(new TemplateDictionary("/"));
  BOOST_FOREACH (const Parameters::value_type &param, _parameters)
    dict->SetValue(param.first, param.second);

  const Recordset::Column_names *column_names= recordset->column_names();
  const Recordset::Column_types &column_types= get_column_types(recordset);
  const Recordset::Column_types &real_column_types= get_real_column_types(recordset);
  const Recordset::Column_flags &column_flags= get_column_flags(recordset);
  
  ColumnId visible_col_count= recordset->get_column_count();
  sqlide::QuoteVar qv;
  {
    if (info.quote != "")
      qv.quote = info.quote;
    qv.escape_string= std::ptr_fun(escape_sql_string_);
    // swap db (sqlite) stores unknown values as quoted strings
    qv.store_unknown_as_string= true;
    qv.allow_func_escaping= false;
    qv.blob_to_string= (true) ? sqlide::QuoteVar::Blob_to_string() : std::ptr_fun(sqlide::QuoteVar::blob_to_hex_string);
  }

  // global variables
  TemplateDictionary::SetGlobalValue("INDENT", "\t");
  
  // misc subst variables valid for header/footer
  dict->SetValue("GENERATOR_QUERY", recordset->generator_query());

  // headers
  sqlide::TypeOfVar tv;
  std::vector<std::string> out_column_types;
  for (ColumnId col= 0; col < visible_col_count; ++col)
  {
    TemplateDictionary* col_dict = dict->AddSectionDictionary("COLUMN");
    col_dict->SetValueWithoutCopy("COLUMN_NAME", (*column_names)[col]);

    // Gets the column real data type and maps it to a classification: Numeric or String
    // Right now the data types are needed for the excel format,  if in the future this
    // is needed for another file format maybe the type classification could be associated
    // to the _template being used
    if (include_column_types == "xls")
    {
      std::string real_col_type = boost::apply_visitor(tv, real_column_types[col]);
      std::string out_col_type = "String";
      if( real_col_type == "FLOAT" || real_col_type == "INTEGER" )
        out_col_type = "Number";

      out_column_types.push_back(out_col_type);

      if (out_col_type == "String")
      {
        TemplateDictionary* col_index_dict = dict->AddSectionDictionary("STRING_COLUMN");
        col_index_dict->SetIntValue("STRING_COLUMN_INDEX", (long)col + 1);
      }
    }
  }

  // if at least one of pre or post templates exist, then we process the recordset as
  // 1. dump pre
  // 2. for each row, dump the row
  // 3. dump post
  // otherwise, the whole thing is dumped at once
  if (pre_tpl || post_tpl)
  {
    FILE *outf = base_fopen(_file_path.c_str(), "w+");
    if (!outf)
      throw std::runtime_error(strfmt("Could not create file `%s`: %s", _file_path.c_str(), g_strerror(errno)));

    if (pre_tpl)
    {
      std::string result_text;
      pre_tpl->Expand(&result_text, dict.get());

      fprintf(outf, "%s", result_text.c_str());
    }
    
    // data
    {
      const size_t partition_count= recordset->data_swap_db_partition_count();
      std::list<boost::shared_ptr<sqlite::query> > data_queries(partition_count);
      Recordset::prepare_partition_queries(data_swap_db, "select * from `data%s`", data_queries);
      std::vector<boost::shared_ptr<sqlite::result> > data_results(data_queries.size());
      
      if (Recordset::emit_partition_queries(data_swap_db, data_queries, data_results))
      {
        bool next_row_exists= true;
        sqlite::variant_t v;
        do
        {
          TemplateDictionary row_dict_base("/");
          TemplateDictionary* row_dict = row_dict_base.AddSectionDictionary("ROW");
          
          BOOST_FOREACH (const Parameters::value_type &param, _parameters)
            row_dict_base.SetValue(param.first, param.second);

          // process a single row
          for (size_t partition= 0; partition < partition_count; ++partition)
          {
            boost::shared_ptr<sqlite::result> &data_rs= data_results[partition];
            for (ColumnId col_begin= partition * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT, col= col_begin,
                 col_end= std::min<ColumnId>(visible_col_count, (partition + 1) * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT); col < col_end; ++col)
            {
              TemplateDictionary* field_dict;
              ColumnId partition_column= col - col_begin;
              bool is_null;
              v = data_rs->get_variant((int)partition_column);

              is_null = sqlide::is_var_null(v); // for some reason, the apply_visitor stuff isnt handling NULL

              field_dict = row_dict->AddSectionDictionary("FIELD");
              if (is_null)
                field_dict->AddSectionDictionary("FIELD_is_null");
              else
                field_dict->AddSectionDictionary("FIELD_is_not_null");

              if (!include_column_types.empty())
                field_dict->SetValueWithoutCopy("FIELD_TYPE", out_column_types[col]);

              field_dict->SetValueWithoutCopy("FIELD_NAME", (*column_names)[col]);

              std::string field_value;
              sqlide::VarToStr var_to_str;
              
              if (is_null)
                field_value = null_syntax;
              else if (strings_are_pre_quoted)
                field_value= (column_flags[col] & Recordset::NeedsQuoteFlag) || sqlide::is_var_null(v)?
                     boost::apply_visitor(qv, column_types[col], v):
                     boost::apply_visitor(var_to_str, v);
              else
                field_value= boost::apply_visitor(var_to_str, v);
              field_dict->SetValue("FIELD_VALUE", field_value);
            }
          }

          BOOST_FOREACH (boost::shared_ptr<sqlite::result> &data_rs, data_results)
            next_row_exists= data_rs->next_row();

          if (next_row_exists)
            row_dict->SetValue("ROW_SEPARATOR", info.row_separator);
          else
            row_dict->SetValue("ROW_SEPARATOR", "");

          // expand template & flush row
          {
            std::string result_text;
            tpl->Expand(&result_text, &row_dict_base);
            
            fwrite(result_text.data(), 1, result_text.size(), outf);
          }                  
        }
        while (next_row_exists);
      }
    }

    if (post_tpl)
    {
      std::string result_text;
      post_tpl->Expand(&result_text, dict.get());

      fwrite(result_text.data(), 1, result_text.size(), outf);
    }

    fclose(outf);
  }
  else // no pre/post separation
  {
    // data
    {
      const size_t partition_count= recordset->data_swap_db_partition_count();
      std::list<boost::shared_ptr<sqlite::query> > data_queries(partition_count);
      Recordset::prepare_partition_queries(data_swap_db, "select * from `data%s`", data_queries);
      std::vector<boost::shared_ptr<sqlite::result> > data_results(data_queries.size());
      if (Recordset::emit_partition_queries(data_swap_db, data_queries, data_results))
      {
        bool next_row_exists= true;
        sqlite::variant_t v;
        do
        {
          TemplateDictionary* row_dict = dict->AddSectionDictionary("ROW");
          for (size_t partition= 0; partition < partition_count; ++partition)
          {
            boost::shared_ptr<sqlite::result> &data_rs= data_results[partition];
            for (ColumnId col_begin= partition * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT, col= col_begin,
                 col_end= std::min<ColumnId>(visible_col_count, (partition + 1) * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT); col < col_end; ++col)
            {
              ColumnId partition_column= col - col_begin;
              v = data_rs->get_variant((int)partition_column);
              TemplateDictionary* field_dict = row_dict->AddSectionDictionary("FIELD");
              field_dict->SetValueWithoutCopy("FIELD_NAME", (*column_names)[col]);
              std::string field_value;
              sqlide::VarToStr var_to_str;
              
              if (strings_are_pre_quoted)
                field_value= (column_flags[col] & Recordset::NeedsQuoteFlag) || sqlide::is_var_null(v)?
                    boost::apply_visitor(qv, column_types[col], v):
                    boost::apply_visitor(var_to_str, v);
              else
                field_value= boost::apply_visitor(var_to_str, v);
              field_dict->SetValue("FIELD_VALUE", field_value);                
            }
          }
          BOOST_FOREACH (boost::shared_ptr<sqlite::result> &data_rs, data_results)
          next_row_exists= data_rs->next_row();
        }
        while (next_row_exists);
      }
    }
    
    // expand tempalte & flush result
    {
      std::string result_text;
      tpl->Expand(&result_text, dict.get());
      GError *error = 0;
      
      // use g_file_set because it can handle utf8 filenames
      if (!g_file_set_contents(_file_path.c_str(), result_text.data(), (gssize)result_text.size(), &error))
      {
        std::string message = error->message;
        g_free(error);
        throw std::runtime_error(strfmt("Failed to write to output file `%s`: %s", _file_path.c_str(), message.c_str()));
      }
    }
  }
}


void Recordset_text_storage::do_unserialize(Recordset *recordset, sqlite::connection *data_swap_db)
{
  throw std::runtime_error("Recordset_text_storage::unserialize is not implemented");
}


void Recordset_text_storage::do_fetch_blob_value(Recordset *recordset, sqlite::connection *data_swap_db, RowId rowid, ColumnId column, sqlite::variant_t &blob_value)
{
}


std::string Recordset_text_storage::parameter_value(const std::string &name) const
{
  Parameters::const_iterator i= _parameters.find(name);
  return (_parameters.end() != i) ? i->second : std::string();
}


std::vector<Recordset_storage_info> Recordset_text_storage::storage_types(GRTManager *grtm)
{
  scan_templates(grtm);
  
  std::vector<Recordset_storage_info> types;
  for (std::map<std::string, TemplateInfo>::const_iterator iter = _templates.begin();
       iter != _templates.end(); ++iter)
    types.push_back(iter->second);
  return types;
}

