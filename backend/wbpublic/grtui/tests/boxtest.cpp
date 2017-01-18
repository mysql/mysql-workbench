
#include "mforms/mforms.h"

#include "gtk/lf_mforms.h"

#include <gtkmm.h>

using namespace mforms;

int main(int argc, char **argv) {
  Gtk::Main main(argc, argv);

  ::mforms::gtk::init();

  Form window;
  Box hbox(true);

  window.set_size(400, 400);

  window.set_content(&hbox);

  Button b1;
  Button b2;
  Button b3;

  b1.set_text("Button1");
  b2.set_text("Button2");
  b3.set_text("Button3");

  hbox.add(&b1, true, true);
  hbox.add(&b2, true, false);
  hbox.add(&b3, false, false);

  hbox.set_spacing(8);
  hbox.set_padding(12);

  window.show();

  main.run();
  return 0;
}
