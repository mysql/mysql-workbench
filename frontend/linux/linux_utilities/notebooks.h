#ifndef __NOTEBOOKS_H__
#define __NOTEBOOKS_H__

#include <gtkmm/box.h>
#include <gtkmm/notebook.h>
#include <gtkmm/alignment.h>

#if GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 20
typedef Gtk::Notebook   ActionAreaNotebook;
#else
//==============================================================================
//
//==============================================================================
class ActionAreaNotebook : public Gtk::VBox
{
  public:
    ActionAreaNotebook();

    int append_page(Widget& child, Widget& tab_label);
    int append_page(Widget& child, const Glib::ustring& tab_label);

    void set_action_widget(Gtk::Widget* widget, Gtk::PackType pack_type = Gtk::PACK_START);

    Glib::SignalProxy2<void,GtkNotebookPage*, guint>  signal_switch_page()
    {
      return book_cont.signal_switch_page();
    }

    Glib::SignalProxy2<void,Gtk::Widget*, guint> signal_page_reordered()
    {
      return book_tab.signal_page_reordered();
    }

    int get_n_pages() const
    {
      return book_cont.get_n_pages();
    }

    int page_num(Gtk::Widget &w)
    {
      return book_cont.page_num(w);
    }

    const Gtk::Widget* get_nth_page(const int pnum) const
    {
      return book_cont.get_nth_page(pnum);
    }

    Gtk::Widget* get_nth_page(const int pnum)
    {
      return book_cont.get_nth_page(pnum);
    }

    const Gtk::Widget* get_nth_label(const int pnum) const
    {
      return book_tab.pages()[pnum].get_tab_label();
    }

    Gtk::Widget* get_nth_label(const int pnum)
    {
      return book_tab.pages()[pnum].get_tab_label();
    }

    void remove_page(Gtk::Widget& child);

    int get_current_page() const
    {
      return book_cont.get_current_page();
    }

    void set_current_page(const int page)
    {
      book_tab.set_current_page(page);
      book_cont.set_current_page(page);
    }

    void set_tab_pos(Gtk::PositionType pos)
    {
      book_tab.set_tab_pos(pos);
      book_cont.set_tab_pos(pos);
      if (pos == Gtk::POS_BOTTOM)
        reorder_child(tab_box, 1);
      else if (pos == Gtk::POS_TOP)
        reorder_child(tab_box, 0);
    }

    Widget* get_tab_label (Widget& child);
    void set_tab_reorderable(Widget& w, bool reorderable = true);

  private:
    void reorder_cont_page(Gtk::Widget* w, guint pos);
    void switch_cont_page(GtkNotebookPage*, guint page_num)
    {
      book_cont.set_current_page(page_num);
    }

    Gtk::HBox       tab_box;
    Gtk::Alignment  action_bin;
    Gtk::Notebook   book_tab;
    Gtk::Notebook   book_cont;
};
#endif //#if GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 20

#endif

