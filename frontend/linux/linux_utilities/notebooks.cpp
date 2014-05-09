#include "notebooks.h"

#if GTK_VERSION_LT(2,20)

//------------------------------------------------------------------------------
ActionAreaNotebook::ActionAreaNotebook()
{
  book_cont.set_show_tabs(false);
  book_tab.set_scrollable(true);
  book_tab.set_show_border(false);

  tab_box.pack_start(book_tab, true, true);
  tab_box.pack_start(action_bin, false, false, 10);

  pack_start(tab_box, false, true);
  pack_start(book_cont, true, true);

  book_tab.signal_switch_page().connect(sigc::mem_fun(this, &ActionAreaNotebook::switch_cont_page));
  action_bin.property_yscale() = 0.5;

  book_cont.show();
  book_tab.show();

  book_cont.set_show_border(false);
  book_tab.set_show_border(false);

  book_tab.signal_page_reordered().connect(sigc::mem_fun(this, &ActionAreaNotebook::reorder_cont_page));
}

//------------------------------------------------------------------------------
int ActionAreaNotebook::append_page(Widget& child, Widget& tab_label)
{
  book_cont.append_page(child);
  tab_label.show_all();
  child.show_all();
  Gtk::Alignment* al = new Gtk::Alignment();
  al->show();
  tab_label.set_data("content", (void*)&child);
  tab_label.set_data("dummy", (void*)al);
  al->set_data("label", (void*)&tab_label);
  child.set_data("label", (void*)&tab_label);
  return book_tab.append_page(*Gtk::manage(al), tab_label);
}

//------------------------------------------------------------------------------
int ActionAreaNotebook::append_page(Widget& child, const Glib::ustring& tab_label)
{
  return append_page(child, *Gtk::manage(new Gtk::Label(tab_label)));
}

//------------------------------------------------------------------------------
void ActionAreaNotebook::set_action_widget(Gtk::Widget* widget, Gtk::PackType pack_type)
{
  action_bin.remove();
  action_bin.add(*widget);
}

//------------------------------------------------------------------------------
void ActionAreaNotebook::remove_page(Gtk::Widget& child)
{
  const int pnum = book_cont.page_num(child);
  book_tab.remove_page(pnum);
  book_cont.remove_page(pnum);
}

//------------------------------------------------------------------------------
Gtk::Widget* ActionAreaNotebook::get_tab_label (Gtk::Widget& child)
{
  Gtk::Widget* r = 0;
  const int page_num = book_cont.page_num(child);
  if (page_num >= 0)
  {
    r = book_tab.pages()[page_num].get_tab_label();
  }
  return r;
}

//------------------------------------------------------------------------------
void ActionAreaNotebook::reorder_cont_page(Gtk::Widget* dummy_cont, guint pos)
{
  // dummy_cont is an empty Gtk::Alignment from book_tab
  Gtk::Widget* tab_label = (Gtk::Widget*)dummy_cont->get_data("label");
  if (tab_label)
  {
    Gtk::Widget* child = (Gtk::Widget*)tab_label->get_data("content");
    if (child)
      book_cont.reorder_child(*child, pos);
  }
}

//------------------------------------------------------------------------------
void ActionAreaNotebook::set_tab_reorderable(Widget& w, bool reorderable)
{
  Gtk::Widget* label = (Gtk::Widget*)w.get_data("label");
  if (label)
  {
    Gtk::Widget* dummy_cont = (Gtk::Widget*)label->get_data("dummy");

    if (dummy_cont)
      book_tab.set_tab_reorderable(*dummy_cont, reorderable);
  }
}

#endif
