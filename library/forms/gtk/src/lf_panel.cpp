#include "../lf_mforms.h"
#include "../lf_panel.h"
#include "gtk_helpers.h"
#include "base/drawing.h"

mforms::gtk::PanelImpl::PanelImpl(::mforms::Panel *self, ::mforms::PanelType type)
  : ViewImpl(self), BinImpl(this), _frame(0), _evbox(0), _radio_group_set(false) {
  // static Gdk::Color sel_color;
  // static bool initialized = false;
  // if (!initialized)
  //{
  //  sel_color = Gtk::RC::get_style(Gtk::TreeView())->get_bg(Gtk::STATE_SELECTED);
  //  initialized = true;
  //}

  _title_check = 0;

  switch (type) {
    case TransparentPanel: // just a container with no background
      _frame = new Gtk::Frame();
      _frame->set_shadow_type(Gtk::SHADOW_NONE);
      break;
    case StyledHeaderPanel: // just a container with color filled background
      _evbox = new Gtk::EventBox();
      _evbox->signal_draw().connect(sigc::bind(sigc::mem_fun(this, &PanelImpl::on_draw_event), _evbox));

      break;
    case FilledHeaderPanel: {
      mforms::App *app = mforms::App::get();
      if (app) {
        base::Color sclr = base::Color::getSystemColor(base::SystemColor::HighlightColor);
        _evbox->override_background_color(color_to_rgba(Gdk::Color(sclr.to_html())), Gtk::STATE_FLAG_NORMAL);
      }
    }
    case FilledPanel: // just a container with color filled background
      _evbox = new Gtk::EventBox();
      break;
    case BorderedPanel: // container with native border
      _frame = new Gtk::Frame();
      _frame->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
      break;
    case LineBorderPanel: // container with a solid line border
      _frame = new Gtk::Frame();
      _frame->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
      break;
    case TitledBoxPanel: // native grouping box with a title with border
      _frame = new Gtk::Frame();
      _frame->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
      break;
    case TitledGroupPanel: // native grouping container with a title (may have no border)
      _frame = new Gtk::Frame();
      _frame->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
      break;
  }

  if (_frame)
    _frame->show();
  if (_evbox)
    _evbox->show();
  setup();
}

mforms::gtk::PanelImpl::~PanelImpl() {
  if (_frame)
    delete (_frame);

  if (_evbox)
    delete (_evbox);
}

bool mforms::gtk::PanelImpl::create(::mforms::Panel *self, ::mforms::PanelType type) {
  return new PanelImpl(self, type);
}

void mforms::gtk::PanelImpl::set_title(::mforms::Panel *self, const std::string &title) {
  PanelImpl *panel = self->get_data<PanelImpl>();

  if (panel->_title_check)
    panel->_title_check->set_label(title);
  else if (panel->_frame)
    panel->_frame->set_label(title);
}

void mforms::gtk::PanelImpl::set_active(::mforms::Panel *self, bool flag) {
  PanelImpl *panel = self->get_data<PanelImpl>();

  if (panel->_title_check)
    panel->_title_check->set_active(flag);
}

bool mforms::gtk::PanelImpl::get_active(::mforms::Panel *self) {
  PanelImpl *panel = self->get_data<PanelImpl>();

  if (panel->_title_check)
    return panel->_title_check->get_active();
  return false;
}

void mforms::gtk::PanelImpl::set_back_color(::mforms::Panel *self, const std::string &color) {
  PanelImpl *panel = self->get_data<PanelImpl>();

  if (panel->_evbox)
    panel->_evbox->override_background_color(color_to_rgba(Gdk::Color(color)), Gtk::STATE_FLAG_NORMAL);
}

void mforms::gtk::PanelImpl::add(::mforms::Panel *self, ::mforms::View *child) {
  PanelImpl *panel = self->get_data<PanelImpl>();

  Gtk::Widget *outer_child = child->get_data<ViewImpl>()->get_outer();

  if (panel->_evbox)
    panel->_evbox->add(*outer_child);
  else if (panel->_frame)
    panel->_frame->add(*outer_child);
  child->show();
}

void mforms::gtk::PanelImpl::remove(::mforms::Panel *self, ::mforms::View *child) {
  PanelImpl *panel = self->get_data<PanelImpl>();

  if (panel->_evbox)
    panel->_evbox->remove();
  else if (panel->_frame)
    panel->_frame->remove();
}

void mforms::gtk::PanelImpl::set_padding_impl(int left, int top, int right, int bottom) {
  if (_evbox)
    _evbox->set_border_width(left);
  else if (_frame)
    _frame->set_border_width(left);
}

void mforms::gtk::PanelImpl::init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_panel_impl.create = &PanelImpl::create;
  f->_panel_impl.set_title = &PanelImpl::set_title;
  f->_panel_impl.set_back_color = &PanelImpl::set_back_color;

  f->_panel_impl.add = &PanelImpl::add;
  f->_panel_impl.remove = &PanelImpl::remove;

  f->_panel_impl.set_active = &PanelImpl::set_active;
  f->_panel_impl.get_active = &PanelImpl::get_active;
}

// called by radiobutton to add itself to a radio group
void mforms::gtk::PanelImpl::add_to_radio_group(Gtk::RadioButton *radio) {
  if (!_radio_group_set) {
    _radio_group_set = true;
    _radio_group = radio->get_group();
  } else
    radio->set_group(_radio_group);
}
