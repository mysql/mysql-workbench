/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_model.h"
#include "reporting.h"

#include "interfaces/sqlgenerator.h"
#include "grts/structs.workbench.h"
#include "grtdb/db_helpers.h"

#include "reporting_template_variables.h"

#include "base/util_functions.h"
#include "base/string_utilities.h"
#include "base/geometry.h"
#include "base/log.h"
#include "base/util_functions.h"
#include "base/symbol-info.h"

#include "mforms/code_editor.h"

#include "mysql/MySQLRecognizerCommon.h"
#include "SymbolTable.h"

#include "UniConversion.h"

// Support for syntax highlighting in SQL output.
#ifdef _MSC_VER
  #include "win32/ScintillaWR.h"
  #define SCI_WRAPPER_NS ScintillaWrapper::
#else
  #include "Scintilla.h"
  #include "WordList.h"
  #include "LexerModule.h"
  #include "LexAccessor.h"
  #include "Accessor.h"
  #include "Catalogue.h"
  #include "PropSetSimple.h"
  #define SCI_WRAPPER_NS Scintilla::
#endif
#include "SciLexer.h"

#include "mtemplate/template.h"

using namespace base;
using namespace Scintilla;

DEFAULT_LOG_DOMAIN("Model.Reporting")

//----------------- LexerDocument --------------------------------------------------------------------------------------

LexerDocument::LexerDocument(const std::string &text) : _text(text), _styling_mask('\0') {
  _style_position = 0;
  _styling_mask = 127;
  _style_buffer = new char[_text.size()];

  // Split the text into lines and store start and length of each.
  std::vector<std::string> lines = base::split(text, "\n");
  std::size_t start = 0;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    _lines.push_back(std::make_pair(start, lines[i].size() + 1));
    start += lines[i].size() + 1;
  }
}

//----------------------------------------------------------------------------------------------------------------------

LexerDocument::~LexerDocument() {
  delete[] _style_buffer;
}

//----------------------------------------------------------------------------------------------------------------------

// IDocument implementation.
int LexerDocument::Version() const {
  return 0; // Indicates old style lexer document.
}

//----------------------------------------------------------------------------------------------------------------------

void LexerDocument::SetErrorStatus(int status) {
  throw std::logic_error(std::string("Internal error. Unexpected use of unimplemented function ")
                           .append(__FUNCTION__)
                           .append(" in LexerDocument (")
                           .append(__FILE__)
                           .append(")."));
}

//----------------------------------------------------------------------------------------------------------------------

Sci_Position LexerDocument::Length() const {
  return static_cast<Sci_Position>(_text.size());
}

//----------------------------------------------------------------------------------------------------------------------

void LexerDocument::GetCharRange(char *buffer, Sci_Position position, Sci_Position lengthRetrieve) const {
  _text.copy(buffer, lengthRetrieve, position);
}

//----------------------------------------------------------------------------------------------------------------------

char LexerDocument::StyleAt(Sci_Position position) const {
  return _style_buffer[position];
}

//----------------------------------------------------------------------------------------------------------------------

Sci_Position LexerDocument::LineFromPosition(Sci_Position position) const {
  std::size_t i = 0;
  while (i < _lines.size()) {
    if ((std::size_t)position < _lines[i].first + _lines[i].second)
      break;
    ++i;
  }

  if (i < _lines.size())
    return (int)i;
  return (int)_lines.size();
}

//----------------------------------------------------------------------------------------------------------------------

Sci_Position LexerDocument::LineStart(Sci_Position line) const {
  if (_lines.empty())
    return 1;

  if (line >= (int)_lines.size())
    return (int)(_lines.back().first + _lines.back().second); // A position after the last one.

  return (int)_lines[line].first;
}

//----------------------------------------------------------------------------------------------------------------------

int LexerDocument::GetLevel(Sci_Position line) const {
  if (line < 0 || line >= (int)_level_cache.size())
    return SC_FOLDLEVELBASE;
  return _level_cache[line];
}

//----------------------------------------------------------------------------------------------------------------------

int LexerDocument::SetLevel(Sci_Position line, int level) {
  if (line >= 0) {
    // Check if we need to make more room in our cache.
    if (line >= (int)_level_cache.size()) {
      std::size_t last_size = _level_cache.size();
      _level_cache.resize(line + 1);

      // Initialize newly added entries.
      for (std::size_t i = last_size - 1; i < _level_cache.size() - 1; i++)
        _level_cache[i] = SC_FOLDLEVELBASE;
    }
    _level_cache[line] = level;
    return level;
  }
  return SC_FOLDLEVELBASE;
}

//----------------------------------------------------------------------------------------------------------------------

int LexerDocument::GetLineState(Sci_Position line) const {
  throw std::logic_error(std::string("Internal error. Unexpected use of unimplemented function ")
                           .append(__FUNCTION__)
                           .append(" in LexerDocument (")
                           .append(__FILE__)
                           .append(")."));
}

//----------------------------------------------------------------------------------------------------------------------

int LexerDocument::SetLineState(Sci_Position line, int state) {
  throw std::logic_error(std::string("Internal error. Unexpected use of unimplemented function ")
                           .append(__FUNCTION__)
                           .append(" in LexerDocument (")
                           .append(__FILE__)
                           .append(")."));
}

//----------------------------------------------------------------------------------------------------------------------

void LexerDocument::StartStyling(Sci_Position position) {
  _style_position = position;
}

//----------------------------------------------------------------------------------------------------------------------

bool LexerDocument::SetStyleFor(Sci_Position length, char style) {
  // Style buffer and text have the same length so we can use the text to get the size (which is faster).
  if (_style_position + length >= (int)_text.size())
    return false;

  Sci_Position i = _style_position;
  style &= _styling_mask;
  for (; length > 0; i++, length--)
    _style_buffer[i] = style;
  _style_position = i;

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

bool LexerDocument::SetStyles(Sci_Position length, const char *styles) {
  if (_style_position + length > (int)_text.size())
    return false;

  Sci_Position i = _style_position;
  for (int j = 0; length > 0; i++, j++, length--)
    _style_buffer[i] = styles[j] & _styling_mask;
  _style_position = i;

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

void LexerDocument::DecorationSetCurrentIndicator(int indicator) {
  throw std::logic_error(std::string("Internal error. Unexpected use of unimplemented function ")
                           .append(__FUNCTION__)
                           .append(" in LexerDocument (")
                           .append(__FILE__)
                           .append(")."));
}

//----------------------------------------------------------------------------------------------------------------------

void LexerDocument::DecorationFillRange(Sci_Position position, int value, Sci_Position fillLength) {
  throw std::logic_error(std::string("Internal error. Unexpected use of unimplemented function ")
                           .append(__FUNCTION__)
                           .append(" in LexerDocument (")
                           .append(__FILE__)
                           .append(")."));
}

//----------------------------------------------------------------------------------------------------------------------

void LexerDocument::ChangeLexerState(Sci_Position start, Sci_Position end) {
  throw std::logic_error(std::string("Internal error. Unexpected use of unimplemented function ")
                           .append(__FUNCTION__)
                           .append(" in LexerDocument (")
                           .append(__FILE__)
                           .append(")."));
}

//----------------------------------------------------------------------------------------------------------------------

int LexerDocument::CodePage() const {
  return SC_CP_UTF8;
}

//----------------------------------------------------------------------------------------------------------------------

bool LexerDocument::IsDBCSLeadByte(char ch) const {
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

const char *LexerDocument::BufferPointer() {
  throw std::logic_error(std::string("Internal error. Unexpected use of unimplemented function ")
                           .append(__FUNCTION__)
                           .append(" in LexerDocument (")
                           .append(__FILE__)
                           .append(")."));
}

//----------------------------------------------------------------------------------------------------------------------

int LexerDocument::GetLineIndentation(Sci_Position line) {
  throw std::logic_error(std::string("Internal error. Unexpected use of unimplemented function ")
                           .append(__FUNCTION__)
                           .append(" in LexerDocument (")
                           .append(__FILE__)
                           .append(")."));
}

//----------------------------------------------------------------------------------------------------------------------

Sci_Position LexerDocument::LineEnd(Sci_Position line) const {
  throw std::logic_error(std::string("Internal error. Unexpected use of unimplemented function ")
                         .append(__FUNCTION__)
                         .append(" in LexerDocument (")
                         .append(__FILE__)
                         .append(")."));
}

//----------------------------------------------------------------------------------------------------------------------

Sci_Position LexerDocument::GetRelativePosition(Sci_Position positionStart, Sci_Position characterOffset) const {
  Sci_Position pos = positionStart;
  pos = positionStart + characterOffset;
  if ((pos < 0) || (pos > Length()))
    return INVALID_POSITION;

  return pos;
}

//----------------------------------------------------------------------------------------------------------------------

int LexerDocument::GetCharacterAndWidth(Sci_Position position, Sci_Position *pWidth) const {
  int character;
  const unsigned char leadByte = _text[position];
  if (UTF8IsAscii(leadByte)) {
    // Single byte character or invalid
    character =  leadByte;
  } else {
    const int widthCharBytes = UTF8BytesOfLead[leadByte];
    unsigned char charBytes[UTF8MaxBytes] = { leadByte, 0, 0, 0 };
    for (int b = 1; b < widthCharBytes; b++)
      charBytes[b] = _text[position + b];
    const int utf8status = UTF8Classify(charBytes, widthCharBytes);
    if (utf8status & UTF8MaskInvalid) {
      // Report as singleton surrogate values which are invalid Unicode
      character =  0xDC80 + leadByte;
    } else {
      character = UnicodeFromUTF8(charBytes);
    }
  }
  return character;
}

//----------------- WbModelImpl ----------------------------------------------------------------------------------------

/**
 * Initializes the template engine with our templates. They are registered so we can get error
 * reports for them if something is wrong and also can reload them if they have been changed while
 * the application is running.
 */
void WbModelImpl::initializeReporting() {
  // Enumerate reporting folder for all stored report types.
}

//----------------------------------------------------------------------------------------------------------------------

/*
 * @brief returns the list of available template file directories
 *
 * @param model - the workbench_physical_ModelRef to process
 * @param templates - a GRT List the available templates will be added to
 * @return 1 on success, 0 on error
 */
ssize_t WbModelImpl::getAvailableReportingTemplates(grt::StringListRef templates) {
  // get pointer to the GRT
  std::string basedir = bec::GRTManager::get()->get_basedir();
  std::string template_base_dir = base::makePath(basedir, "modules/data/wb_model_reporting");
  GDir *dir;
  const char *entry;

  dir = g_dir_open(template_base_dir.c_str(), 0, NULL);
  if (dir) {
    while ((entry = g_dir_read_name(dir)) != NULL) {
      char *path = g_build_filename(template_base_dir.c_str(), entry, NULL);

      if (g_file_test(path, (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) && g_str_has_suffix(entry, ".tpl")) {
        // reformat the template name, replace _ with spaces
        char *temp = g_strdup(entry);
        char *ptr = temp;
        while ((ptr = strchr(ptr, '_')))
          *ptr = ' ';

        // remove .tpl suffix
        ptr = strrchr(temp, '.');
        *ptr = 0;

        templates.insert(temp);

        g_free(temp);
      }

      g_free(path);
    }

    g_dir_close(dir);
  }

  return 1;
}

//----------------------------------------------------------------------------------------------------------------------

/*
 * @brief returns the template info for the given template
 *
 * @param model - the workbench_physical_ModelRef to process
 * @param template_name - the name of the template
 * @return the template info object
 */
workbench_model_reporting_TemplateInfoRef WbModelImpl::getReportingTemplateInfo(const std::string &template_name) {
  std::string template_dir = getTemplateDirFromName(template_name);

  std::string template_info_path = base::makePath(template_dir, "info.xml");
  if (g_file_test(template_info_path.c_str(), (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)))
    return workbench_model_reporting_TemplateInfoRef::cast_from(grt::GRT::get()->unserialize(template_info_path));
  else
    return workbench_model_reporting_TemplateInfoRef();
}

//----------------------------------------------------------------------------------------------------------------------

workbench_model_reporting_TemplateStyleInfoRef WbModelImpl::get_template_style_from_name(std::string template_name,
  std::string template_style_name) {
  if (template_style_name == "")
    return workbench_model_reporting_TemplateStyleInfoRef();

  std::string template_dir = getTemplateDirFromName(template_name);

  std::string template_info_path = base::makePath(template_dir, "info.xml");
  if (g_file_test(template_info_path.c_str(), (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))) {
    workbench_model_reporting_TemplateInfoRef info =
      workbench_model_reporting_TemplateInfoRef::cast_from(grt::GRT::get()->unserialize(template_info_path));

    for (std::size_t i = 0; i < info->styles().count(); i++) {
      workbench_model_reporting_TemplateStyleInfoRef styleInfo = info->styles().get(i);

      if (template_style_name == (std::string)styleInfo->name())
        return styleInfo;
    }
  }

  return workbench_model_reporting_TemplateStyleInfoRef();
}

//----------------------------------------------------------------------------------------------------------------------

void read_option(bool &var, const char *field, const grt::DictRef &dict) {
  if (dict.has_key(field))
    var = dict.get_int(field) != 0;
}

//----------------------------------------------------------------------------------------------------------------------

void read_option(std::string &var, const char *field, const grt::DictRef &dict) {
  if (dict.has_key(field))
    var = dict.get_string(field);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Assigns the given value to the dictionary if it is not empty. Otherwise the text "n/a" is added.
 **/
void assignValueOrNA(mtemplate::DictionaryInterface *dict, const char *key, const std::string &value) {
  if (value.size() == 0)
    dict->setValue(key, "<span class=\"report_na_entry\">n/a</span>");
  else
    dict->setValue(key, value);
}

//----------------------------------------------------------------------------------------------------------------------

void fillTablePropertyDict(const db_mysql_TableRef &table, mtemplate::DictionaryInterface *table_dict) {
  assignValueOrNA(table_dict, REPORT_TABLE_AVG_ROW_LENGTH, *table->avgRowLength());
  table_dict->setValue(REPORT_TABLE_USE_CHECKSUM, (table->checksum() == 1) ? "yes" : "no");
  assignValueOrNA(table_dict, REPORT_TABLE_CONNECTION_STRING, *table->connectionString());
  assignValueOrNA(table_dict, REPORT_TABLE_CHARSET, *table->defaultCharacterSetName());
  assignValueOrNA(table_dict, REPORT_TABLE_COLLATION, *table->defaultCollationName());
  table_dict->setValue(REPORT_TABLE_DELAY_KEY_UPDATES, (table->delayKeyWrite() == 1) ? "yes" : "no");
  assignValueOrNA(table_dict, REPORT_TABLE_MAX_ROW_COUNT, *table->maxRows());
  assignValueOrNA(table_dict, REPORT_TABLE_MIN_ROW_COUNT, *table->minRows());
  assignValueOrNA(table_dict, REPORT_TABLE_UNION_TABLES, *table->mergeUnion());
  assignValueOrNA(table_dict, REPORT_TABLE_MERGE_METHOD, *table->mergeInsert());
  assignValueOrNA(table_dict, REPORT_TABLE_AUTO_INCREMENT, *table->nextAutoInc());
  assignValueOrNA(table_dict, REPORT_TABLE_PACK_KEYS, *table->packKeys());
  table_dict->setValue(REPORT_TABLE_HAS_PASSWORD, ((*table->password()).size() == 0) ? "no" : "yes");
  assignValueOrNA(table_dict, REPORT_TABLE_ROW_FORMAT, *table->rowFormat());
  assignValueOrNA(table_dict, REPORT_TABLE_KEY_BLOCK_SIZE, *table->keyBlockSize());
  assignValueOrNA(table_dict, REPORT_TABLE_DATA_DIR, *table->tableDataDir());
  assignValueOrNA(table_dict, REPORT_TABLE_INDEX_DIR, *table->tableIndexDir());
  assignValueOrNA(table_dict, REPORT_TABLE_ENGINE, *table->tableEngine());

  // Partitions.
  if (table->partitionCount() > 0) {
    mtemplate::DictionaryInterface *partitions_dict = table_dict->addSectionDictionary(REPORT_PARTITION_LISTING);
    partitions_dict->setIntValue(REPORT_PARTITION_COUNT, table->partitionCount());
    partitions_dict->setValue(REPORT_PARTITION_TYPE, *table->partitionType());
    partitions_dict->setValue(REPORT_PARTITION_EXPRESSION, *table->partitionExpression());

    partitions_dict->setIntValue(REPORT_PARTITION_SUB_COUNT, table->subpartitionCount());
    partitions_dict->setValue(REPORT_PARTITION_SUB_TYPE, *table->subpartitionType());
    partitions_dict->setValue(REPORT_PARTITION_SUB_EXPRESSION, *table->subpartitionExpression());

    for (int i = 0; i < table->partitionCount(); i++) {
      db_mysql_PartitionDefinitionRef partition = table->partitionDefinitions().get(i);

      mtemplate::DictionaryInterface *partition_dict = table_dict->addSectionDictionary(REPORT_PARTITIONS);

      partition_dict->setValue(REPORT_PARTITION_NAME, *partition->name());
      assignValueOrNA(partition_dict, REPORT_PARTITION_VALUE, *partition->value());
      partition_dict->setValue(REPORT_PARTITION_COMMENT, *partition->comment());
      assignValueOrNA(partition_dict, REPORT_PARTITION_MAX_ROW_COUNT, *partition->maxRows());
      assignValueOrNA(partition_dict, REPORT_PARTITION_MIN_ROW_COUNT, *partition->minRows());
      assignValueOrNA(partition_dict, REPORT_PARTITION_DATA_DIR, *partition->indexDirectory());
      assignValueOrNA(partition_dict, REPORT_PARTITION_INDEX_DIR, *partition->dataDirectory());

      // One possible iteration to sub partitions.
      for (int j = 0; j < table->subpartitionCount(); j++) {
        db_mysql_PartitionDefinitionRef sub_partition = partition->subpartitionDefinitions().get(j);

        mtemplate::DictionaryInterface *sub_partition_dict =
          partition_dict->addSectionDictionary(REPORT_PARTITION_SUB_PARTITIONS);

        sub_partition_dict->setValue(REPORT_PARTITION_SUB_NAME, *sub_partition->name());
        assignValueOrNA(sub_partition_dict, REPORT_PARTITION_SUB_VALUE, *sub_partition->value());
        sub_partition_dict->setValue(REPORT_PARTITION_SUB_COMMENT, *sub_partition->comment());
        assignValueOrNA(sub_partition_dict, REPORT_PARTITION_SUB_MAX_ROW_COUNT, *sub_partition->maxRows());
        assignValueOrNA(sub_partition_dict, REPORT_PARTITION_SUB_MIN_ROW_COUNT, *sub_partition->minRows());
        assignValueOrNA(sub_partition_dict, REPORT_PARTITION_SUB_DATA_DIR, *sub_partition->indexDirectory());
        assignValueOrNA(sub_partition_dict, REPORT_PARTITION_SUB_INDEX_DIR, *sub_partition->dataDirectory());
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void fillColumnDict(const db_mysql_ColumnRef &col, const db_mysql_TableRef &table,
                    mtemplate::DictionaryInterface *col_dict, bool detailed) {
  if (*table->isPrimaryKeyColumn(col)) {
    if (*table->isForeignKeyColumn(col))
      col_dict->setValue(REPORT_COLUMN_KEY, "FK");
    else
      col_dict->setValue(REPORT_COLUMN_KEY, "PK");
  }

  col_dict->setValue(REPORT_COLUMN_NAME, *col->name());

  col_dict->setValue(REPORT_COLUMN_NOTNULL, (col->isNotNull() == 1) ? "Yes" : "No");
  col_dict->setValue(REPORT_COLUMN_DEFAULTVALUE, (col->defaultValueIsNull() == 1) ? "NULL" : *col->defaultValue());
  col_dict->setValue(REPORT_COLUMN_COMMENT, *col->comment());
  col_dict->setValue(REPORT_COLUMN_DATATYPE, *col->formattedRawType());

  if (detailed) {
    col_dict->setValue(REPORT_TABLE_NAME, *table->name());

    std::string key_part = "";
    if (table->isPrimaryKeyColumn(col))
      key_part += "Primary key, ";
    if (table->isForeignKeyColumn(col))
      key_part += "Foreign key, ";

    col_dict->setValue(REPORT_COLUMN_KEY_PART, key_part.substr(0, key_part.size() - 2));
    col_dict->setValue(REPORT_COLUMN_NULLABLE, (col->isNotNull() == 1) ? "No" : "Yes");
    col_dict->setValue(REPORT_COLUMN_AUTO_INC, (col->autoIncrement() == 1) ? "Yes" : "No");
    if (!col->characterSetName().empty())
      col_dict->setValue(REPORT_COLUMN_CHARSET, *col->characterSetName());
    else
      col_dict->setValue(REPORT_COLUMN_CHARSET, "Schema Default");

    if (!col->collationName().empty())
      col_dict->setValue(REPORT_COLUMN_COLLATION, *col->collationName());
    else
      col_dict->setValue(REPORT_COLUMN_COLLATION, "Schema Default");

    if (col->userType().is_valid())
      col_dict->setValue(REPORT_COLUMN_IS_USERTYPE, "Yes");
    else
      col_dict->setValue(REPORT_COLUMN_IS_USERTYPE, "No");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void fillIndexDict(const db_mysql_IndexRef &idx, const db_mysql_TableRef &table,
                   mtemplate::DictionaryInterface *idx_dict, bool detailed) {
  idx_dict->setValue(REPORT_INDEX_NAME, *idx->name());

  idx_dict->setValue(REPORT_INDEX_PRIMARY, (idx->isPrimary() == 1) ? "Yes" : "No");
  idx_dict->setValue(REPORT_INDEX_UNIQUE, (idx->unique() == 1) ? "Yes" : "No");
  idx_dict->setValue(REPORT_INDEX_TYPE, *idx->indexType());
  idx_dict->setValue(REPORT_INDEX_KIND, *idx->indexKind());
  idx_dict->setValue(REPORT_INDEX_COMMENT, *idx->comment());

  for (std::size_t l = 0; l < idx->columns().count(); l++) {
    db_mysql_IndexColumnRef idx_col = idx->columns().get(l);

    mtemplate::DictionaryInterface *idx_col_dict = idx_dict->addSectionDictionary(REPORT_INDEX_COLUMNS);
    idx_col_dict->setValue(REPORT_INDEX_COLUMN_NAME, *idx_col->referencedColumn()->name());
    idx_col_dict->setValue(REPORT_INDEX_COLUMN_ORDER, (idx_col->descend() == 1) ? "Descending" : "Ascending");
    if (idx_col->comment().empty())
      idx_col_dict->setValue(REPORT_INDEX_COLUMN_COMMENT, "no comment");
    else
      idx_col_dict->setValue(REPORT_INDEX_COLUMN_COMMENT, *idx_col->comment());
  }

  if (detailed) {
    idx_dict->setValue(REPORT_TABLE_NAME, *table->name());
    idx_dict->setIntValue(REPORT_INDEX_KEY_BLOCK_SIZE, idx->keyBlockSize());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void fillForeignKeyDict(const db_mysql_ForeignKeyRef &fk, const db_mysql_TableRef &table,
                        mtemplate::DictionaryInterface *fk_dict, bool detailed) {
  fk_dict->setValue(REPORT_REL_NAME, *fk->name());
  fk_dict->setValue(REPORT_REL_TYPE,
                    bec::TableHelper::is_identifying_foreign_key(table, fk) ? "Identifying" : "Non-Identifying");
  if (fk->referencedTable().is_valid())
    fk_dict->setValue(REPORT_REL_PARENTTABLE, *fk->referencedTable()->name());
  fk_dict->setValue(REPORT_REL_CHILDTABLE, *table->name());
  fk_dict->setValue(REPORT_REL_CARD, (fk->many() == 1) ? "1:n" : "1:1");

  if (detailed) {
    fk_dict->setValue(REPORT_TABLE_NAME, *table->name());
    fk_dict->setValue(REPORT_FK_DELETE_RULE, *fk->deleteRule());
    fk_dict->setValue(REPORT_FK_UPDATE_RULE, *fk->updateRule());
    fk_dict->setValue(REPORT_FK_MANDATORY, fk->mandatory() ? "Yes" : "No");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void fillTriggerDict(const db_mysql_TriggerRef &trigger, const db_mysql_TableRef &table,
                     mtemplate::DictionaryInterface *trigger_dict) {
  trigger_dict->setValue(REPORT_TRIGGER_NAME, *trigger->name());
  trigger_dict->setValue(REPORT_TRIGGER_TIMING, *trigger->timing());
  trigger_dict->setValue(REPORT_TRIGGER_ENABLED, (trigger->enabled() == 1) ? "yes" : "no");

  trigger_dict->setValue(REPORT_TABLE_NAME, table->name().c_str());
  trigger_dict->setValue(REPORT_TRIGGER_DEFINER, *trigger->definer());
  trigger_dict->setValue(REPORT_TRIGGER_EVENT, *trigger->event());
  trigger_dict->setValue(REPORT_TRIGGER_ORDER, *trigger->ordering());
  trigger_dict->setValue(REPORT_TRIGGER_OTHER_TRIGGER, *trigger->otherTrigger());
  trigger_dict->setValue(REPORT_TRIGGER_TIMING, *trigger->timing());
}

//----------------------------------------------------------------------------------------------------------------------

void fillViewDict(const db_mysql_ViewRef &view, mtemplate::DictionaryInterface *view_dict) {
  view_dict->setValue(REPORT_VIEW_NAME, *view->name());
  view_dict->setValueAndShowSection(REPORT_VIEW_COMMENT, *view->comment(), REPORT_VIEW_COMMENT_LISTING);

  view_dict->setValue(REPORT_VIEW_COLUMNS, *view->name());
  view_dict->setValue(REPORT_VIEW_READ_ONLY, view->isReadOnly() ? "read only" : "writable");
  view_dict->setValue(REPORT_VIEW_WITH_CHECK, view->withCheckCondition() ? "yes" : "no");

  std::string columns = "";
  for (grt::StringListRef::const_iterator iterator = view->columns().begin(); iterator != view->columns().end();
       iterator++) {
    columns += *iterator;
    columns += ", ";
  }
  assignValueOrNA(view_dict, REPORT_VIEW_COLUMNS, columns);
}

//----------------------------------------------------------------------------------------------------------------------

void fillRoutineDict(const db_mysql_RoutineRef &routine, mtemplate::DictionaryInterface *routine_dict) {
  std::string value;

  routine_dict->setValue(REPORT_ROUTINE_NAME, *routine->name());
  routine_dict->setValue(REPORT_ROUTINE_TYPE, *routine->routineType());

  assignValueOrNA(routine_dict, REPORT_ROUTINE_RETURN_TYPE, routine->returnDatatype());
  assignValueOrNA(routine_dict, REPORT_ROUTINE_SECURITY, value = routine->security());

  routine_dict->setIntValue(REPORT_ROUTINE_PARAMETER_COUNT, (long)routine->params().count());
  for (std::size_t j = 0; j < routine->params().count(); j++) {
    db_mysql_RoutineParamRef parameter = routine->params().get(j);

    mtemplate::DictionaryInterface *parameter_dict = routine_dict->addSectionDictionary(REPORT_ROUTINE_PARAMETERS);

    parameter_dict->setValue(REPORT_ROUTINE_PARAMETER_NAME, *parameter->name());
    parameter_dict->setValue(REPORT_ROUTINE_PARAMETER_TYPE, *parameter->paramType());
    parameter_dict->setValue(REPORT_ROUTINE_PARAMETER_DATA_TYPE, *parameter->datatype());
  }
}

//----------------------------------------------------------------------------------------------------------------------

static Scintilla::WordList *keywordLists[KEYWORDSET_MAX + 2];

/**
 * Initialization of the syntax highlighter classes that are used to colorize SQL code in HTML reports.
 */
const Scintilla::LexerModule *setup_syntax_highlighter(db_mgmt_RdbmsRef rdbms) {
  const Scintilla::LexerModule *result = SCI_WRAPPER_NS Catalogue::Find("mysql");

  if (result != NULL) {
    mforms::CodeEditorConfig config(mforms::LanguageMySQL);
    std::map<std::string, std::string> keywordMap = config.get_keywords();

    // Create the keyword lists used for lexing.
    for (int i = 0; i <= KEYWORDSET_MAX; i++)
      keywordLists[i] = new SCI_WRAPPER_NS WordList();
    keywordLists[KEYWORDSET_MAX + 1] = NULL;

    // There are no predefined constants for the indices below, but the occupancy of the list array
    // can be seen in LexMySQL.cxx.
    auto version = rdbms->version();
    if (!version.is_valid())
      version = bec::parse_version("8.0.16");

    auto &functions = MySQLSymbolInfo::systemFunctionsForVersion(bec::versionToEnum(version));
    std::string list;
    for (auto &name : functions)
      list += base::tolower(name) + " ";
    ((SCI_WRAPPER_NS WordList *)keywordLists[3])->Set(list.c_str());

    auto &keywords = MySQLSymbolInfo::keywordsForVersion(bec::versionToEnum(version));
    list = "";
    for (auto &name : keywords)
      list += base::tolower(name) + " ";
    ((SCI_WRAPPER_NS WordList *)keywordLists[1])->Set(list.c_str());

    ((SCI_WRAPPER_NS WordList *)keywordLists[5])->Set(keywordMap["Procedure keywords"].c_str());
    ((SCI_WRAPPER_NS WordList *)keywordLists[6])->Set(keywordMap["User Keywords 1"].c_str());

    // TODO: adjust CSS files so the color values correspond to what the user has set currently.
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Cleanup after we are done with report creation.
 */
void cleanup_syntax_highlighter() {
  for (int i = 0; i <= KEYWORDSET_MAX; i++)
    delete (SCI_WRAPPER_NS WordList *)keywordLists[i];
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the HTML markup for the given style.
 */
const std::string markupFromStyle(int style) {
  switch (style) {
    case SCE_MYSQL_DEFAULT:
      return "<span class=\"syntax_default\">%s</span>";
      break;
    case SCE_MYSQL_COMMENT:
      return "<span class=\"syntax_comment\">%s</span>";
      break;
    case SCE_MYSQL_COMMENTLINE:
      return "<span class=\"syntax_comment_line\">%s</span>";
      break;
    case SCE_MYSQL_VARIABLE:
      return "<span class=\"syntax_variable\">%s</span>";
      break;
    case SCE_MYSQL_SYSTEMVARIABLE:
      return "<span class=\"syntax_system_variable\">%s</span>";
      break;
    case SCE_MYSQL_KNOWNSYSTEMVARIABLE:
      return "<span class=\"syntax_known_system_variable\">%s</span>";
      break;
    case SCE_MYSQL_NUMBER:
      return "<span class=\"syntax_number\">%s</span>";
      break;
    case SCE_MYSQL_MAJORKEYWORD:
      return "<span class=\"syntax_major_keyword\">%s</span>";
      break;
    case SCE_MYSQL_KEYWORD:
      return "<span class=\"syntax_keyword\">%s</span>";
      break;
    case SCE_MYSQL_DATABASEOBJECT:
      return "<span class=\"syntax_database_object\">%s</span>";
      break;
    case SCE_MYSQL_PROCEDUREKEYWORD:
      return "<span class=\"syntax_procedure_keyword\">%s</span>";
      break;
    case SCE_MYSQL_STRING:
      return "<span class=\"syntax_string\">%s</span>";
      break;
    case SCE_MYSQL_SQSTRING:
      return "<span class=\"syntax_single_quoted_string\">%s</span>";
      break;
    case SCE_MYSQL_DQSTRING:
      return "<span class=\"syntax_double_quoted_string\">%s</span>";
      break;
    case SCE_MYSQL_OPERATOR:
      return "<span class=\"syntax_operator\">%s</span>";
      break;
    case SCE_MYSQL_FUNCTION:
      return "<span class=\"syntax_function\">%s</span>";
      break;
    case SCE_MYSQL_IDENTIFIER:
      return "<span class=\"syntax_identifier\">%s</span>";
      break;
    case SCE_MYSQL_QUOTEDIDENTIFIER:
      return "<span class=\"syntax_quoted_identifier\">%s</span>";
      break;
    case SCE_MYSQL_USER1:
      return "<span class=\"syntax_user1\">%s</span>";
      break;
    case SCE_MYSQL_USER2:
      return "<span class=\"syntax_user2\">%s</span>";
      break;
    case SCE_MYSQL_USER3:
      return "<span class=\"syntax_user3\">%s</span>";
      break;
    case SCE_MYSQL_HIDDENCOMMAND:
      return "<span class=\"syntax_hidden_command\">%s</span>";
      break;
    default:
      return "%s";
  }
}

//----------------------------------------------------------------------------------------------------------------------

void set_ddl(mtemplate::DictionaryInterface *target, SQLGeneratorInterfaceImpl *sqlgenModule,
             const GrtNamedObjectRef &object, const Scintilla::LexerModule *lexer, bool ddl_enabled) {
  if (ddl_enabled && sqlgenModule != NULL) {
    std::string sql = sqlgenModule->makeCreateScriptForObject(object);

    if (lexer != NULL) {
      // Add syntax highlighter markup.
      LexerDocument *document = new LexerDocument(sql);
      SCI_WRAPPER_NS PropSetSimple property_set;
      SCI_WRAPPER_NS Accessor *accessor = new SCI_WRAPPER_NS Accessor(document, &property_set);

      lexer->Lex(0, (int)sql.size(), 0, keywordLists, *accessor);

      int currentStyle = SCE_MYSQL_DEFAULT;
      int tokenStart = 0;
      std::string markup = "";
      int i;
      for (i = 0; i < (int)sql.size(); i++)
        if (currentStyle != accessor->StyleAt(i)) {
          markup += base::replaceString(markupFromStyle(currentStyle), "%s", sql.substr(tokenStart, i - tokenStart));
          tokenStart = i;
          currentStyle = accessor->StyleAt(i);
        }

      markup += base::replaceString(markupFromStyle(currentStyle), "%s", sql.substr(tokenStart, i - tokenStart));

      delete accessor;
      delete document;

      sql = markup;
    };

    std::string fixed_line_breaks = base::replaceString(sql, "\n", "<br />");

    // The DDL script is wrapped in an own section dir to allow switching it off entirely (including
    // the surrounding HTML code).
    target->setValueAndShowSection(REPORT_DDL_SCRIPT, fixed_line_breaks, REPORT_DDL_LISTING);
  }
}

//----------------------------------------------------------------------------------------------------------------------

static int count_template_files(const std::string template_dir) {
  // loop over all files in the template dir
  const char *entry;
  int count = 0;
  GDir *dir = g_dir_open(template_dir.c_str(), 0, NULL);
  if (dir) {
    while ((entry = g_dir_read_name(dir)) != NULL) {
      // skip the info.xml file and preview pngs
      if (strcmp(entry, "info.xml") == 0 || (g_str_has_prefix(entry, "preview_") && g_str_has_suffix(entry, ".png")))
        continue;

      char *path = g_build_filename(template_dir.c_str(), entry, NULL);
      if (g_file_test(path, (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))) {
        if (g_str_has_suffix(entry, ".tpl"))
          count++;
      }
      g_free(path);
    }
  }
  g_dir_close(dir);
  return count;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Generates a schema report for the model passed in workbench_physical_Model.
 *
 * @param model - the workbench_physical_ModelRef to process
 * @param options - various options that customize the output, including output template, output path etc.
 * @return 1 on success, 0 on error
 */
ssize_t WbModelImpl::generateReport(workbench_physical_ModelRef model, const grt::DictRef &options) {
  // get pointer to the GRT
  std::string basedir = bec::GRTManager::get()->get_basedir();
  std::string template_base_dir = base::makePath(basedir, "modules/data/wb_model_reporting");

  db_mysql_CatalogRef catalog = db_mysql_CatalogRef::cast_from(model->catalog());

  // helper variables
  std::map<std::string, std::vector<db_mysql_ForeignKeyRef> > tbl_fk_map;

  // Process options
  std::string template_name = "HTML Basic Frames";
  std::string template_style_name = "";
  std::string title = "MySQL Model Report";
  std::string output_path = "";
  bool columns_show = true;
  bool indices_show = true;
  bool fks_show = true;
  bool fks_show_referred_fks = true;
  bool show_ddl = true;
  bool use_highlighting = false;
  std::unique_ptr<mtemplate::TemplateOutputFile> single_file_output;

  if (options.is_valid()) {
    read_option(template_name, "template_name", options);
    read_option(template_style_name, "template_style_name", options);
    read_option(title, "title", options);
    read_option(output_path, "output_path", options);
    read_option(columns_show, "columns_show", options);
    read_option(indices_show, "indices_show", options);
    read_option(fks_show, "fks_show", options);
    read_option(fks_show_referred_fks, "fks_show_referred_fks", options);
    read_option(show_ddl, "show_ddl", options);
    read_option(use_highlighting, "use_highlighting", options);
  }

  bool single_file_report = count_template_files(getTemplateDirFromName(template_name)) == 1;

  const Scintilla::LexerModule *lexer = NULL;
  if (use_highlighting)
    lexer = setup_syntax_highlighter(model->rdbms());

  // Ensure the output dir exists.
  {
    int r;

    if (!g_file_test(output_path.c_str(), G_FILE_TEST_EXISTS)) {
      r = g_mkdir_with_parents(output_path.c_str(), 0700);
      if (r < 0) {
        logError("Could not create report directory %s: %s", output_path.c_str(), g_strerror(errno));
        return 0;
      }
    }
  }

  // Start report generation
  grt::GRT::get()->send_info("Generating schema report...");

  // Build FK dictionary if required
  if (fks_show && fks_show_referred_fks) {
    // build schema_dict by loop over all schemata, add it to the main_dict
    for (std::size_t i = 0; i < catalog->schemata().count(); i++) {
      db_mysql_SchemaRef schema = catalog->schemata().get(i);

      // loop over all tables
      for (std::size_t j = 0; j < schema->tables().count(); j++) {
        db_mysql_TableRef table = schema->tables().get(j);

        // loop over all foreign keys
        for (std::size_t k = 0; k < table->foreignKeys().count(); k++) {
          db_mysql_ForeignKeyRef fk = table->foreignKeys().get(k);
          db_mysql_TableRef ref_tbl = fk->referencedTable();

          if (!ref_tbl.is_valid())
            continue;

          // look for table in tbl_fk_map
          std::map<std::string, std::vector<db_mysql_ForeignKeyRef> >::iterator tbl_fk_map_it = tbl_fk_map.find(ref_tbl.id());
          if (tbl_fk_map_it != tbl_fk_map.end()) {
            // if found, add fk reference to table
            tbl_fk_map_it->second.push_back(fk);
          } else {
            // if table is not found, create entry
            std::vector<db_mysql_ForeignKeyRef> new_tbl_fk_map_v;

            // add fk reference to table
            new_tbl_fk_map_v.push_back(fk);

            tbl_fk_map.insert(make_pair(ref_tbl.id(), new_tbl_fk_map_v));
          }
        }
      }
    }
  }

  // create main dictionary that will be used to expand the templates
  mtemplate::DictionaryInterface *main_dictionary = mtemplate::CreateMainDictionary();

  // Set some global project info.
  main_dictionary->setValue(REPORT_TITLE, title);

  std::string time = base::fmttime(0, DATETIME_FMT);
  main_dictionary->setValue(REPORT_GENERATED, time);

  workbench_DocumentRef document = workbench_DocumentRef::cast_from(model->owner());
  main_dictionary->setValue(REPORT_PROJECT_NAME, (std::string)document->info()->project());
  main_dictionary->setValue(REPORT_PROJECT_AUTHOR, (std::string)document->info()->author());
  main_dictionary->setValue(REPORT_PROJECT_TITLE, (std::string)document->info()->caption());
  main_dictionary->setValue(REPORT_PROJECT_CHANGED, (std::string)document->info()->dateChanged());
  main_dictionary->setValue(REPORT_PROJECT_CREATED, (std::string)document->info()->dateCreated());
  main_dictionary->setValue(REPORT_PROJECT_DESCRIPTION, (std::string)document->info()->description());
  main_dictionary->setValue(REPORT_PROJECT_VERSION, (std::string)document->info()->version());

  main_dictionary->dump();

  workbench_model_reporting_TemplateStyleInfoRef styleInfo =
    get_template_style_from_name(template_name, template_style_name);
  if (styleInfo.is_valid())
    main_dictionary->setValue(REPORT_STYLE_NAME, (std::string)styleInfo->styleTagValue());

  main_dictionary->setIntValue(REPORT_SCHEMA_COUNT, (long int)catalog->schemata().count());

  int total_column_count = 0;
  int total_index_count = 0;
  int total_fk_count = 0;
  int total_table_count = 0;
  int total_view_count = 0;
  int total_sp_count = 0;
  int total_trigger_count = 0;

  SQLGeneratorInterfaceImpl *sqlgenModule = NULL;

  if (show_ddl) {
    sqlgenModule = dynamic_cast<SQLGeneratorInterfaceImpl *>(grt::GRT::get()->get_module("DbMySQL"));
    if (!sqlgenModule)
      throw std::logic_error("could not find SQL generation module for mysql");
  }

  // Build schema_dict by looping over all schemata, add it to the main_dict.
  for (int i = 0; i < (int)catalog->schemata().count(); i++) {
    db_mysql_SchemaRef schema = catalog->schemata().get(i);

    mtemplate::DictionaryInterface *schema_dictionary = main_dictionary->addSectionDictionary(REPORT_SCHEMATA);
    schema_dictionary->setIntValue(REPORT_SCHEMA_ID, i);
    schema_dictionary->setIntValue(REPORT_SCHEMA_NUMBER, i + 1);
    schema_dictionary->setValue(REPORT_SCHEMA_NAME, *schema->name());

    set_ddl(schema_dictionary, sqlgenModule, schema, lexer, show_ddl);

    schema_dictionary->setIntValue(REPORT_TABLE_COUNT, (int)schema->tables().count());

    // Loop over all tables. Build the nested tables sub groups and at the same time the
    // full collection of all columns, indices and foreign keys.
    for (int j = 0; j < (int)schema->tables().count(); j++) {
      db_mysql_TableRef table = schema->tables().get(j);

      mtemplate::DictionaryInterface *table_dictionary = schema_dictionary->addSectionDictionary(REPORT_TABLES);

      // The table id is used as unique id, e.g. in HTML anchors.
      table_dictionary->setIntValue(REPORT_TABLE_ID, total_table_count++);

      // The table number is used in visible counts like "Table 1 of 20".
      table_dictionary->setIntValue(REPORT_TABLE_NUMBER, j + 1);

      table_dictionary->setValue(REPORT_TABLE_NAME, *table->name());
      table_dictionary->setValueAndShowSection(REPORT_TABLE_COMMENT, *table->comment(), REPORT_TABLE_COMMENT_LISTING);

      fillTablePropertyDict(table, table_dictionary);
      set_ddl(table_dictionary, sqlgenModule, table, lexer, show_ddl);

      if (columns_show) {
        mtemplate::DictionaryInterface *columns_list_dictionary = NULL;

        schema_dictionary->setIntValue(REPORT_COLUMN_COUNT, (long)table->columns().count());

        for (int k = 0; k < (int)table->columns().count(); k++) {
          // Create the dict for the outer section (including header)
          if (k == 0)
            columns_list_dictionary = table_dictionary->addSectionDictionary(REPORT_COLUMNS_LISTING);

          db_mysql_ColumnRef col = table->columns().get(k);

          // Fill data for table details.
          mtemplate::DictionaryInterface *col_dictionary =
            columns_list_dictionary->addSectionDictionary(REPORT_COLUMNS);

          fillColumnDict(col, table, col_dictionary, false);

          // Fill data for full details.
          col_dictionary = schema_dictionary->addSectionDictionary(REPORT_COLUMNS);

          fillColumnDict(col, table, col_dictionary, true);

          col_dictionary->setIntValue(REPORT_COLUMN_ID, total_column_count++);
          col_dictionary->setIntValue(REPORT_COLUMN_NUMBER, k + 1);
        }
      }

      if (indices_show) {
        mtemplate::DictionaryInterface *idx_list_dictionary = NULL;

        schema_dictionary->setIntValue(REPORT_INDEX_COUNT, (long)table->indices().count());

        for (int k = 0; k < (int)table->indices().count(); k++) {
          // Create the dict for the outer section (including header)
          if (k == 0)
            idx_list_dictionary = table_dictionary->addSectionDictionary(REPORT_INDICES_LISTING);

          db_mysql_IndexRef idx = table->indices().get(k);

          mtemplate::DictionaryInterface *idx_dictionary = idx_list_dictionary->addSectionDictionary(REPORT_INDICES);

          fillIndexDict(idx, table, idx_dictionary, false);

          idx_dictionary = schema_dictionary->addSectionDictionary(REPORT_INDICES);
          fillIndexDict(idx, table, idx_dictionary, true);

          idx_dictionary->setIntValue(REPORT_INDEX_ID, total_index_count++);
          idx_dictionary->setIntValue(REPORT_INDEX_NUMBER, k + 1);
        }
      }

      if (fks_show) {
        mtemplate::DictionaryInterface *fk_list_dictionary = NULL;

        schema_dictionary->setIntValue(REPORT_FOREIGN_KEY_COUNT, (long)table->foreignKeys().count());

        for (int k = 0; k < (int)table->foreignKeys().count(); k++) {
          // Create the dict for the outer section (inluding header)
          if (k == 0)
            fk_list_dictionary = table_dictionary->addSectionDictionary(REPORT_REL_LISTING);

          db_mysql_ForeignKeyRef fk = table->foreignKeys().get(k);

          mtemplate::DictionaryInterface *fk_dictionary = fk_list_dictionary->addSectionDictionary(REPORT_REL);
          fillForeignKeyDict(fk, table, fk_dictionary, false);

          fk_dictionary = schema_dictionary->addSectionDictionary(REPORT_FOREIGN_KEYS);
          fillForeignKeyDict(fk, table, fk_dictionary, true);

          fk_dictionary->setIntValue(REPORT_FOREIGN_KEY_ID, total_fk_count++);
          fk_dictionary->setIntValue(REPORT_FOREIGN_KEY_NUMBER, k + 1);
        }

        if (fks_show_referred_fks) {
          std::map<std::string, std::vector<db_mysql_ForeignKeyRef> >::iterator tbl_fk_map_it = tbl_fk_map.find(table->id());
          if (tbl_fk_map_it != tbl_fk_map.end()) {
            std::vector<db_mysql_ForeignKeyRef>::iterator fk_it = tbl_fk_map_it->second.begin();
            for (; fk_it != tbl_fk_map_it->second.end(); fk_it++) {
              if (fk_list_dictionary == NULL)
                fk_list_dictionary = table_dictionary->addSectionDictionary(REPORT_REL_LISTING);

              db_mysql_ForeignKeyRef fk = *fk_it;

              mtemplate::DictionaryInterface *fk_dictionary = fk_list_dictionary->addSectionDictionary(REPORT_REL);
              fk_dictionary->setValue(REPORT_REL_NAME, *fk->name());
              fk_dictionary->setValue(REPORT_REL_TYPE, bec::TableHelper::is_identifying_foreign_key(table, fk)
                                                         ? "Identifying"
                                                         : "Non-Identifying");
              fk_dictionary->setValue(REPORT_REL_PARENTTABLE, *table->name());
              fk_dictionary->setValue(REPORT_REL_CHILDTABLE, *fk->owner()->name());
              fk_dictionary->setValue(REPORT_REL_CARD, (fk->many() == 1) ? "1:n" : "1:1");
            }
          }
        }

        // Triggers.
        schema_dictionary->setIntValue(REPORT_TRIGGER_COUNT, (long)table->triggers().count());

        for (int k = 0; k < (int)table->triggers().count(); k++) {
          db_mysql_TriggerRef trigger = table->triggers().get(k);

          mtemplate::DictionaryInterface *trigger_dictionary = schema_dictionary->addSectionDictionary(REPORT_TRIGGERS);
          fillTriggerDict(trigger, table, trigger_dictionary);
          set_ddl(trigger_dictionary, sqlgenModule, trigger, lexer, show_ddl);

          trigger_dictionary->setIntValue(REPORT_TRIGGER_ID, total_trigger_count++);
          trigger_dictionary->setIntValue(REPORT_TRIGGER_NUMBER, k + 1);
        }
      }
    }

    // View section.
    schema_dictionary->setIntValue(REPORT_VIEW_COUNT, (long)schema->views().count());
    for (int j = 0; j < (int)schema->views().count(); j++) {
      db_mysql_ViewRef view = schema->views().get(j);

      mtemplate::DictionaryInterface *view_dictionary = schema_dictionary->addSectionDictionary(REPORT_VIEWS);
      view_dictionary->setIntValue(REPORT_VIEW_ID, total_view_count++);
      view_dictionary->setIntValue(REPORT_VIEW_NUMBER, j + 1);
      set_ddl(view_dictionary, sqlgenModule, view, lexer, show_ddl);

      fillViewDict(view, view_dictionary);
    }

    // Routine section.
    schema_dictionary->setIntValue(REPORT_ROUTINE_COUNT, (long)schema->routines().count());
    for (int j = 0; j < (int)schema->routines().count(); j++) {
      db_mysql_RoutineRef routine = schema->routines().get(j);

      mtemplate::DictionaryInterface *routine_dictionary = schema_dictionary->addSectionDictionary(REPORT_ROUTINES);
      routine_dictionary->setIntValue(REPORT_ROUTINE_ID, total_sp_count++);
      routine_dictionary->setIntValue(REPORT_ROUTINE_NUMBER, j + 1);
      set_ddl(routine_dictionary, sqlgenModule, routine, lexer, show_ddl);

      fillRoutineDict(routine, routine_dictionary);
    }
  }

  main_dictionary->setIntValue(REPORT_TOTAL_COLUMN_COUNT, total_column_count);
  main_dictionary->setIntValue(REPORT_TOTAL_INDEX_COUNT, total_index_count);
  main_dictionary->setIntValue(REPORT_TOTAL_FK_COUNT, total_fk_count);
  main_dictionary->setIntValue(REPORT_TOTAL_TABLE_COUNT, total_table_count);
  main_dictionary->setIntValue(REPORT_TOTAL_VIEW_COUNT, total_view_count);
  main_dictionary->setIntValue(REPORT_TOTAL_TRIGGER_COUNT, total_trigger_count);
  main_dictionary->setIntValue(REPORT_TOTAL_ROUTINE_COUNT, total_sp_count);

  // Process template files

  std::string template_dir = getTemplateDirFromName(template_name);

  // loop over all files in the template dir
  const char *entry;
  GDir *dir = g_dir_open(template_dir.c_str(), 0, NULL);
  if (dir) {
    while ((entry = g_dir_read_name(dir)) != NULL) {
      char *path = g_build_filename(template_dir.c_str(), entry, NULL);

      // skip the info.xml file and preview pngs
      if (strcmp(entry, "info.xml") == 0 || (g_str_has_prefix(entry, "preview_") && g_str_has_suffix(entry, ".png"))) {
        g_free(path);
        continue;
      }
      if (g_file_test(path, (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))) {
        if (g_str_has_suffix(entry, ".tpl")) {
          // load template file
          mtemplate::Template *template_index = mtemplate::GetTemplate(path, mtemplate::DO_NOT_STRIP);
          if (template_index == 0) {
            grt::GRT::get()->send_error(
              "Error while loading template files. Please check the log for more information.");
            grt::GRT::get()->send_error(path);
            g_free(path);
            return 0;
          }

          // expand the template based on the dictionary
          mtemplate::TemplateOutputString string_output;
          template_index->expand(main_dictionary, &string_output);

          // build output file name
          std::string output_filename;

          if (single_file_report) {
            // For single file reports the target file name is constructed from the report title.
            output_filename = base::makePath(output_path, title);
            std::string template_filename(entry);

            // Remove the .tpl suffix.
            std::string name = template_filename.substr(0, template_filename.size() - 4);

            // Find the file's target suffix. If there is one use this for the target file too.
            std::string::size_type p = name.rfind('.');
            if (p != std::string::npos)
              output_filename += name.substr(p);

            if (single_file_output == nullptr)
              single_file_output =
                std::unique_ptr<mtemplate::TemplateOutputFile>(new mtemplate::TemplateOutputFile(output_filename));

            template_index->expand(main_dictionary, single_file_output.get());
          } else {
            std::string template_filename(entry);
            output_filename = base::makePath(output_path, template_filename.substr(0, template_filename.size() - 4));
            mtemplate::TemplateOutputFile output(output_filename);
            template_index->expand(main_dictionary, &output);
          }
        } else {
          // Copy files/folders.
          std::string target = base::makePath(output_path, entry);
          if (g_file_test(path, G_FILE_TEST_IS_DIR))
            copy_folder(path, target.c_str());
          else
            base::copyFile(path, target.c_str());
        }
      }

      g_free(path);
    }

    g_dir_close(dir);
  }

  if (use_highlighting)
    cleanup_syntax_highlighter();

  grt::GRT::get()->send_info(
    strfmt("Schema report written to %s %s", single_file_report ? "file" : "folder", output_path.c_str()));

  return 1;
}

//----------------------------------------------------------------------------------------------------------------------
