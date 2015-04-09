#include <gtkmm/scrolledwindow.h>
#include "documentation_box.h"
#include "base/string_utilities.h"
#include "grt/common.h"
#include "grtpp_util.h"
#include <glibmm/main.h>

#define TIMER_INTERVAL 700


DocumentationBox::DocumentationBox(wb::WBContextUI *wbui)
  : Gtk::VBox(false, 0), _wbui(wbui), _multiple_items(false)
{
  pack_start(_combo, false, false);
  
  Gtk::ScrolledWindow *swin= Gtk::manage(new Gtk::ScrolledWindow());
  swin->add(_text);
  swin->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  swin->set_shadow_type(Gtk::SHADOW_IN);

  _text.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
  _text.signal_key_press_event().connect_notify(sigc::mem_fun(this, &DocumentationBox::text_key_press));
  _text.signal_button_press_event().connect_notify(sigc::mem_fun(this, &DocumentationBox::text_button_press));
  _text.get_buffer()->signal_changed().connect(sigc::mem_fun(this, &DocumentationBox::text_changed));
  _combo.signal_changed().connect(sigc::mem_fun(this, &DocumentationBox::combo_changed));

  pack_start(*swin, true, true);
  
  show_all();
}

DocumentationBox::~DocumentationBox()
{
  if(_timer)
    _timer.disconnect();
}

void DocumentationBox::update_for_form(bec::UIForm *form)
{
  if (_timer)
    commit();

  _initializing= true;

  std::vector<std::string> items;
  grt::ListRef<GrtObject> new_object_list;
  std::string description;

  _selected_form= form;

  if (form)
    description = _wbui->get_description_for_selection(form, new_object_list, items);
  else
    description = _wbui->get_description_for_selection(new_object_list, items);
  
  // update only if selection was changed
  if (!grt::compare_list_contents(_object_list, new_object_list))
  {
    _combo.clear();

    // Set description text
    _object_list= new_object_list;

    // Set properties
    _multiple_items= items.size() > 1;

    // handle different number of selected items
    if (!items.empty())
    {
      std::vector<std::string>::iterator it;
      for(it = items.begin(); it != items.end(); ++it)
        _combo.append(*it);

      _combo.set_active(0);

      // lock on multi selection
      if (_multiple_items)
      {
        _text.get_buffer()->set_text("<double-click to overwrite multiple objects>");
        _text.set_editable(false);
      }
      else
      {
        _text.get_buffer()->set_text(description);
        _text.set_editable(true);
      }
    }
    else
    {
      _combo.clear();
      _combo.append(_("No Selection"));
      _combo.set_active(0);

      _text.get_buffer()->set_text("");
      _text.set_editable(false);
    }
  }

  _initializing= false;
}



void DocumentationBox::commit()
{
  puts("COMMIT");
  _timer.disconnect();

  _wbui->set_description_for_selection(_object_list, _text.get_buffer()->get_text());
}


void DocumentationBox::text_changed()
{
  if (!_initializing)
  {
    _timer.disconnect();
    _timer= Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(this, &DocumentationBox::commit),false), TIMER_INTERVAL);
  }
}


void DocumentationBox::text_button_press(GdkEventButton *ev)
{
  if (ev->type == GDK_2BUTTON_PRESS && _multiple_items && !_text.get_editable())
  {
    _initializing= true;
    
    _text.set_editable(true);
    _text.get_buffer()->set_text("");

    _initializing= false;
  }
}


void DocumentationBox::combo_changed()
{
  if (_combo.get_model()->children().size())
    _combo.set_active(0);
}


void DocumentationBox::text_key_press(GdkEventKey *key)
{
  if ((key->state & GDK_CONTROL_MASK) && key->keyval == GDK_KEY_Return && _text.get_editable())
  {
    commit();
  }
}
