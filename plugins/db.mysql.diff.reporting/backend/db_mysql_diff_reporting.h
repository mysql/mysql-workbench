#ifndef _DB_MYSQL_DIFF_REPORTING_H_
#define _DB_MYSQL_DIFF_REPORTING_H_

#include "db_mysql_diff_reporting_public_interface.h"
#include "grt/grt_manager.h"
#include "grts/structs.db.mysql.h"
#include "grt/grt_string_list_model.h"

class DBMYSQLDIFFREPORTINGWBPBE_PUBLIC_FUNC DbMySQLDiffReportingException : public std::logic_error
{
public:
  DbMySQLDiffReportingException(const std::string& message)
    : std::logic_error(message)
  {}
};


class DBMYSQLDIFFREPORTINGWBPBE_PUBLIC_FUNC DbMySQLDiffReporting
{
  bec::GRTManager *manager_;

public:
  inline db_mysql_CatalogRef get_model_catalog()
  {
    return db_mysql_CatalogRef::cast_from(
      manager_->get_grt()->get("/wb/doc/physicalModels/0/catalog"));
  }

public:
  DbMySQLDiffReporting(bec::GRTManager *m);
  virtual ~DbMySQLDiffReporting();

  bec::GRTManager *get_grt_manager() { return manager_; }

  std::string generate_report(const db_mysql_CatalogRef& left_cat,
                              const db_mysql_CatalogRef& right_cat);
};

#endif // _DB_MYSQL_DIFF_REPORTING_H_

