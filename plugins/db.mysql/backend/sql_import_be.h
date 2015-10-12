#ifndef _SQL_IMPORT_BE_H_
#define _SQL_IMPORT_BE_H_

#include "grts/structs.db.h"
#include "grts/structs.workbench.h"

#include "grt/grt_manager.h"
#include "grtpp_undo_manager.h"
#include "db_mysql_public_interface.h"
#include "grtsqlparser/mysql_parser_services.h"
#include "grtdb/db_helpers.h"


class WBPLUGINDBMYSQLBE_PUBLIC_FUNC Sql_import
{
public:
  virtual ~Sql_import() {};
  void grtm(bec::GRTManager *grtm);

  boost::function<grt::ValueRef (grt::GRT*)> get_task_slot();

  boost::function<grt::ValueRef (grt::GRT*)> get_autoplace_task_slot();
  
private:
  grt::StringRef parse_sql_script(grt::GRT *grt, db_CatalogRef catalog, const std::string &sql_script);
  virtual void parse_sql_script(parser::MySQLParserServices::Ref sql_parser, parser::ParserContext::Ref context, db_CatalogRef &catalog, const std::string &sql_scrtipt, grt::DictRef &options);
  virtual db_CatalogRef target_catalog();
  virtual GrtVersionRef getVersion(grt::GRT *grt);

public:
  virtual std::string sql_script() { return _sql_script; }
  virtual void sql_script(const std::string &sql_script) { _sql_script= sql_script; }
  virtual void sql_script_codeset(const std::string &value) { _sql_script_codeset= value; }

  grt::ListRef<GrtObject> get_created_objects();
protected:
  
  grt::ValueRef autoplace_grt(grt::GRT *grt);
  
  grt::DictRef _options;
  
  workbench_DocumentRef _doc;
  std::string _sql_script;
  std::string _sql_script_codeset;
};

#endif /* _SQL_IMPORT_BE_H_ */
