#ifndef _RELATIONSHIP_CANVAS_FLOATER_H_
#define _RELATIONSHIP_CANVAS_FLOATER_H_

#include "canvas_floater.h"

namespace wb {
  class ModelDiagramForm;

  class RelationshipFloater : public Floater {
  public:
    RelationshipFloater(ModelDiagramForm *view);
    virtual ~RelationshipFloater();

    void add_column(const std::string &name);

    boost::signals2::signal<void()> *signal_done_clicked() {
      return _button.signal_activate();
    }

    void setup_pick_target();

    void pick_next_target();

  private:
    mdc::Box _columns_box;
    mdc::TextFigure _text;
    Button _button;
    std::vector<mdc::TextFigure *> _columns;
    unsigned int _current_column;

    void setup_pick_source();
  };
};

#endif
