
#ifndef _WB_DIAGRAM_OPTIONS_H_
#define _WB_DIAGRAM_OPTIONS_H_

#include "mdc.h"

#include "workbench/wb_backend_public_interface.h"

#include "grts/structs.model.h"

#include "base/trackable.h"

namespace wb 
{
  class WBContext;

  class MYSQLWBBACKEND_PUBLIC_FUNC DiagramOptionsBE: public base::trackable
  {
    friend class SizerFigure;
    
    mdc::CanvasView *_view;
    model_DiagramRef _target_view;
    class SizerFigure *_sizer;
    WBContext *_wbcontext;
    std::string _name;

    boost::signals2::signal<void ()> _changed_signal;

    void get_min_size_in_pages(int &xc, int &yc);
    
  public:
    DiagramOptionsBE(mdc::CanvasView *view, model_DiagramRef target_view, WBContext *wb);
    ~DiagramOptionsBE();

    void update_size();
    
    std::string get_name();
    void set_name(const std::string &name);

    int get_xpages();
    int get_ypages();
    void set_xpages(int c);
    void set_ypages(int c);

    void get_max_page_counts(int &max_xpages, int &max_ypages);
    
    void commit();
    
    boost::signals2::signal<void ()>* signal_changed() { return &_changed_signal; }
  };
};

#endif //  _WB_DIAGRAM_OPTIONS_H_
