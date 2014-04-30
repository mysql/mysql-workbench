// Sort columns according the the indexes they belong to.
// First go the columns that belong to the primary key, next
// go the columns that belong to indexes under the foreign keys,
// and go the rest of columns

#include "stdafx.h"

#include "grtpp_module_cpp.h"
#include "grts/structs.db.mysql.h"
#include "grtdb/catalog_templates.h"

#define WbUtilsNative_VERSION "1.0.0"

namespace {

struct Table_iterator
{
  typedef std::list<db_mysql_ColumnRef> Column_list;
  typedef std::vector<db_mysql_IndexRef> Index_vector;
  typedef std::map<grt::ValueRef, int> Column_rank_map;

  Index_vector ixs;
  Column_list cols;
  int next_rank;
  Column_rank_map crm;

  void iteration_cleanup()
  {
    ixs.clear();
    cols.clear();
    next_rank= 0;
    crm.clear();
  }

  static void add_column_to_table(db_mysql_ColumnRef c, db_mysql_TableRef t)
  {
    t->columns().insert(c);
  }

  void add_column_to_list(db_mysql_ColumnRef c)
  {
    cols.push_back(c);
  }

  void add_index_to_vector(db_mysql_IndexRef ix)
  {
    ixs.push_back(ix);
  }

  void rank_column(db_mysql_IndexColumnRef col)
  {
    crm[col->referencedColumn()]= next_rank++;
  }

  void rank_nonindex_column(db_mysql_ColumnRef col)
  {
    if (crm.find(col) != crm.end())
      return;
    crm[col]= next_rank++;
  }

  void rank_index_columns(db_mysql_IndexRef ix)
  {
    ct::for_each<ct::Columns>(ix,
      boost::bind(&Table_iterator::rank_column, this, _1));
  }

  bool compare_indexes(db_mysql_IndexRef ix1, db_mysql_IndexRef ix2)
  {
    bool ix1pk= ix1->isPrimary() != 0;
    bool ix2pk= ix2->isPrimary() != 0;

    if (ix1pk)
      return true;
    if (ix2pk)
      return false;

    bool ix1fk= ix1->indexType() == "FOREIGN";
    bool ix2fk= ix2->indexType() == "FOREIGN";

    if (ix1fk && ix2fk)
      return strcmp(ix1->name().c_str(), ix2->name().c_str()) < 0;

    if (ix1fk)
      return true;
    if (ix2fk)
      return false;

    return strcmp(ix1->name().c_str(), ix2->name().c_str()) < 0;
  }

  bool compare_columns(db_mysql_ColumnRef c1, db_mysql_ColumnRef c2)
  {
    int r1= crm.find(c1)->second;
    int r2= crm.find(c2)->second;
    return r1 < r2;
  }

public:
  void operator() (db_mysql_TableRef t)
  {
    ct::for_each<ct::Indices>(t, 
      boost::bind(&Table_iterator::add_index_to_vector, this, _1));

    ct::for_each<ct::Columns>(t, 
      boost::bind(&Table_iterator::add_column_to_list, this, _1));
    
    std::sort(ixs.begin(), ixs.end(), 
      boost::bind(&Table_iterator::compare_indexes, this, _1, _2));

    next_rank= 0;

    std::for_each(ixs.begin(), ixs.end(),
      boost::bind(&Table_iterator::rank_index_columns, this, _1));
    std::for_each(cols.begin(), cols.end(), 
      boost::bind(&Table_iterator::rank_nonindex_column, this, _1));

    cols.sort(boost::bind(&Table_iterator::compare_columns, this, _1, _2));

    t->columns().remove_all();

    std::for_each(cols.begin(), cols.end(), 
      boost::bind(&Table_iterator::add_column_to_table, _1, t));

    iteration_cleanup();
  }
};

struct Schema_iterator
{
  void operator() (db_mysql_SchemaRef schema) const
  {
    ct::for_each<ct::Tables>(schema, Table_iterator());
  }
};

} // namespace

class WbUtilsNativeImpl : public grt::ModuleImplBase
{
public:
  WbUtilsNativeImpl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr) {}

  DEFINE_INIT_MODULE(WbUtilsNative_VERSION, "MySQL", grt::ModuleImplBase,
                  DECLARE_MODULE_FUNCTION(WbUtilsNativeImpl::sortIndexes));

  int sortIndexes(db_mysql_CatalogRef cat)
  {
    ct::for_each<ct::Schemata>(cat, Schema_iterator());
    return 0;
  }
};

GRT_MODULE_ENTRY_POINT(WbUtilsNativeImpl);
