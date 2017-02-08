
#pragma once

#include "db_mysql_diff_reporting_public_interface.h"
#include "grt/grt_manager.h"
#include "grts/structs.db.mysql.h"
#include "grt/grt_string_list_model.h"

class DBMYSQLDIFFREPORTINGWBPBE_PUBLIC_FUNC DbMySQLDiffReportingException : public std::logic_error {
public:
  DbMySQLDiffReportingException(const std::string& message) : std::logic_error(message) {
  }
};

class DBMYSQLDIFFREPORTINGWBPBE_PUBLIC_FUNC DbMySQLDiffReporting {
public:
  inline db_mysql_CatalogRef get_model_catalog() {
    return db_mysql_CatalogRef::cast_from(grt::GRT::get()->get("/wb/doc/physicalModels/0/catalog"));
  }

public:
  DbMySQLDiffReporting();
  virtual ~DbMySQLDiffReporting();

  std::string generate_report(const db_mysql_CatalogRef& left_cat, const db_mysql_CatalogRef& right_cat);
};
