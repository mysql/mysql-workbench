/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include <gtk/gtk.h>
#include <gtk/gtkicontheme.h>
#include <iostream>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <libgen.h>
#include <fstream>
GtkImage *image1 = nullptr;
GtkWidget *window = nullptr;
GtkWidget *image1_eventbox = nullptr;
GtkMenu *popup_menu = nullptr;
GtkWidget *treeView = nullptr;
GtkWidget *button1 = nullptr;
GtkWidget *button3 = nullptr;
GtkWidget *button4 = nullptr;
GtkWidget *entry2 = nullptr;
GtkDialog *dialog1 = nullptr;
GtkWidget *iconView = nullptr;
GtkWidget *gridView = nullptr;
GtkWidget *checkbutton2 = nullptr;
GtkWidget *radiobutton2 = nullptr;
GtkWidget *radiobutton4 = nullptr;

static std::vector<const char *> iconList({
  "application-exit",
  "appointment-new",
  "call-start",
  "call-stop",
  "contact-new",
  "document-new",
  "document-open",
  "document-open-recent",
  "document-page-setup",
  "document-print",
  "document-print-preview",
  "document-properties",
  "document-revert",
  "document-save",
  "document-save-as",
  "document-send",
  "edit-clear",
  "edit-copy",
  "edit-cut",
  "edit-delete",
  "edit-find",
  "edit-find-replace",
  "edit-paste",
  "edit-redo",
  "edit-select-all",
  "edit-undo",
  "folder-new",
  "format-indent-less",
  "format-indent-more",
  "format-justify-center",
  "format-justify-fill",
  "format-justify-left",
  "format-justify-right",
  "format-text-direction-ltr",
  "format-text-direction-rtl",
  "format-text-bold",
  "format-text-italic",
  "format-text-underline",
  "format-text-strikethrough",
  "go-bottom",
  "go-down",
  "go-first",
  "go-home",
  "go-jump",
  "go-last",
  "go-next",
  "go-previous",
  "go-top",
  "go-up",
  "help-about",
  "help-contents",
  "help-faq",
  "insert-image",
  "insert-link",
  "insert-object",
  "insert-text",
  "list-add",
  "list-remove",
  "mail-forward",
  "mail-mark-important",
  "mail-mark-junk",
  "mail-mark-notjunk",
  "mail-mark-read",
  "mail-mark-unread",
  "mail-message-new",
  "mail-reply-all",
  "mail-reply-sender",
  "mail-send",
  "mail-send-receive",
  "media-eject",
  "media-playback-pause",
  "media-playback-start",
  "media-playback-stop",
  "media-record",
  "media-seek-backward",
  "media-seek-forward",
  "media-skip-backward",
  "media-skip-forward",
  "object-flip-horizontal",
  "object-flip-vertical",
  "object-rotate-left",
  "object-rotate-right",
  "process-stop",
  "system-lock-screen",
  "system-log-out",
  "system-run",
  "system-search",
  "system-reboot",
  "system-shutdown",
  "tools-check-spelling",
  "view-fullscreen",
  "view-refresh",
  "view-restore",
  "view-sort-ascending",
  "view-sort-descending",
  "window-close",
  "window-new",
  "zoom-fit-best",
  "zoom-in",
  "zoom-original",
  "zoom-out",
  "accessories-character-map",
  "accessories-dictionary",
  "accessories-text-editor",
  "help-browser",
  "multimedia-volume-control",
  "preferences-desktop-accessibility",
  "preferences-desktop-font",
  "preferences-desktop-keyboard",
  "preferences-desktop-locale",
  "preferences-desktop-multimedia",
  "preferences-desktop-screensaver",
  "preferences-desktop-theme",
  "preferences-desktop-wallpaper",
  "system-file-manager",
  "system-software-install",
  "system-software-update",
  "utilities-system-monitor",
  "utilities-terminal",
  "applications-development",
  "applications-engineering",
  "applications-games",
  "applications-graphics",
  "applications-internet",
  "applications-multimedia",
  "applications-office",
  "applications-other",
  "applications-science",
  "applications-system",
  "applications-utilities",
  "preferences-desktop",
  "preferences-desktop-peripherals",
  "preferences-desktop-personal",
  "preferences-other",
  "preferences-system",
  "preferences-system-network",
  "system-help",
  "audio-input-microphone",
  "battery",
  "camera-photo",
  "camera-video",
  "camera-web",
  "computer",
  "drive-harddisk",
  "drive-optical",
  "drive-removable-media",
  "input-gaming",
  "input-keyboard",
  "input-mouse",
  "input-tablet",
  "media-flash",
  "media-floppy",
  "media-optical",
  "media-tape",
  "modem",
  "multimedia-player",
  "network-wired",
  "network-wireless",
  "pda",
  "phone",
  "printer",
  "scanner",
  "video-display",
  "emblem-documents",
  "emblem-downloads",
  "emblem-favorite",
  "emblem-important",
  "emblem-mail",
  "emblem-photos",
  "emblem-readonly",
  "emblem-shared",
  "emblem-symbolic-link",
  "emblem-synchronized",
  "emblem-system",
  "emblem-unreadable",
  "face-angry",
  "face-cool",
  "face-crying",
  "face-devilish",
  "face-embarrassed",
  "face-kiss",
  "face-laugh",
  "face-monkey",
  "face-plain",
  "face-raspberry",
  "face-sad",
  "face-sick",
  "face-smile",
  "face-smile-big",
  "face-smirk",
  "face-surprise",
  "face-tired",
  "face-uncertain",
  "face-wink",
  "face-worried",
  "audio-x-generic",
  "font-x-generic",
  "image-x-generic",
  "package-x-generic",
  "text-html",
  "text-x-generic",
  "text-x-generic-template",
  "text-x-script",
  "video-x-generic",
  "x-office-address-book",
  "x-office-calendar",
  "x-office-document",
  "x-office-presentation",
  "x-office-spreadsheet",
  "folder-remote",
  "network-server",
  "network-workgroup",
  "start-here",
  "user-bookmarks",
  "user-desktop",
  "user-home",
  "user-trash",
  "appointment-soon",
  "audio-volume-high",
  "audio-volume-low",
  "audio-volume-medium",
  "audio-volume-muted",
  "battery-caution",
  "battery-low",
  "dialog-error",
  "dialog-information",
  "dialog-password",
  "dialog-question",
  "dialog-warning",
  "folder-drag-accept",
  "folder-open",
  "folder-visiting",
  "image-loading",
  "image-missing",
  "mail-attachment",
  "mail-unread",
  "mail-read",
  "mail-replied",
  "mail-signed",
  "mail-signed-verified",
  "media-playlist-repeat",
  "media-playlist-shuffle",
  "network-error",
  "network-idle",
  "network-offline",
  "network-receive",
  "network-transmit",
  "network-transmit-receive",
  "printer-error",
  "printer-printing",
  "security-high",
  "security-medium",
  "security-low",
  "software-update-available",
  "software-update-urgent",
  "sync-error",
  "sync-synchronizing",
  "task-due",
  "task-past-due",
  "user-available",
  "user-away",
  "user-idle",
  "user-offline",
  "user-trash-full",
  "weather-clear",
  "weather-clear-night",
  "weather-few-clouds",
  "weather-few-clouds-night",
  "weather-fog",
  "weather-overcast",
  "weather-severe-alert",
  "weather-showers",
  "weather-showers-scattered",
  "weather-snow",
  "weather-storm"
});

void on_window_main_destroy(){
    gtk_main_quit();
}

gboolean dialog1_delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  return false;
}


void dialog1_response_event_cb(GtkDialog *dialog, gint response_id, gpointer user_data) {
  if (response_id == GTK_RESPONSE_DELETE_EVENT) {
    gtk_widget_hide((GtkWidget*)dialog1);
    gtk_widget_set_sensitive((GtkWidget*)dialog1, 1);
  }
}


gboolean button1_button_release_event_cb(GtkWidget *widget, GdkEvent  *event, gpointer user_data) {
  GtkFileChooser *fileChooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new("This is a title", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN,
                                                "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, nullptr));
                                       
  gint res = gtk_dialog_run(GTK_DIALOG(fileChooser));
  
  if (res == GTK_RESPONSE_ACCEPT) {
    char *filename = gtk_file_chooser_get_filename (fileChooser);
    gtk_entry_set_text(GTK_ENTRY(entry2), filename);
    std::cout << filename << std::endl;
  }
  
  gtk_widget_destroy (GTK_WIDGET(fileChooser));
  return false;
}

gboolean button3_button_release_event_cb(GtkWidget *widget, GdkEvent  *event, gpointer user_data) {
  gint res = gtk_dialog_run(dialog1);
  return false;
}
void button3_button_clicked_cb(GtkButton *button, gpointer user_data) {
 gint res = gtk_dialog_run(dialog1);
}

gboolean button4_button_release_event_cb(GtkWidget *widget, GdkEvent  *event, gpointer user_data) {
  gtk_window_close(GTK_WINDOW(dialog1));
  return false;
}

gboolean image1_button_release_event_cb(GtkWidget *widget, GdkEvent  *event, gpointer user_data) {
  GtkBuilder *builder = gtk_builder_new();
  GdkEventButton *eventButton = (GdkEventButton *) event;

#if GTK_CHECK_VERSION(3,22,0)
  gtk_menu_popup_at_pointer(popup_menu, nullptr);
#else
  gtk_menu_popup(popup_menu, nullptr, nullptr, nullptr, nullptr, eventButton->button, gtk_get_current_event_time());
#endif
  
  return false;
}

enum ButtonState {
  Indeterminate,
  Checked,
  Unchecked
};

ButtonState checkbox2LastState = Indeterminate;

void checkbox_clicked_event_cb(GtkWidget* widget, gpointer *data) {
  GtkToggleButton *btn = reinterpret_cast<GtkToggleButton*>(widget);
  if (btn == nullptr)
    return;
  
  switch(checkbox2LastState) {
    case Indeterminate:
      gtk_toggle_button_set_active(btn, true);
      gtk_toggle_button_set_inconsistent(btn, false);
      checkbox2LastState = Checked;
      break;
    case Checked:
      gtk_toggle_button_set_active(btn, false);
      gtk_toggle_button_set_inconsistent(btn, false);
      checkbox2LastState = Unchecked;
      break;
    case Unchecked:
      gtk_toggle_button_set_active(btn, false);
      gtk_toggle_button_set_inconsistent(btn, true);
      checkbox2LastState = Indeterminate;
      break;
    default:
      break;
  }
  
  
}

void radiobutton_clicked_event_cb(GtkWidget* widget, gpointer *data) {
  GtkToggleButton *btn = reinterpret_cast<GtkToggleButton*>(widget);
  if (btn == nullptr)
    return;
  
  gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON(radiobutton2), false);
  gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON(radiobutton4), false);
}

void tableFillLevel(GtkTreeStore *store, GtkTreeIter *parent, int level) {
  
  for (unsigned int index = 0; index < 5; ++index){
    char col0[50];
    char col1[50];
    sprintf(col0, "Entry %d.%d", level, index);
    sprintf(col1, "Data %d.%d", level, index);
    
    GtkTreeIter entry;
    gtk_tree_store_append(store, &entry, parent);
    gtk_tree_store_set(store, &entry, 0, col0, 1, col1, -1);
    
    if (level < 5)
      tableFillLevel(store, &entry, level + 1);
  }
}

void fillTable() {
  
  GtkTreeStore *store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

  tableFillLevel(store, nullptr, 0);
  
  gtk_tree_view_set_model(GTK_TREE_VIEW(treeView), GTK_TREE_MODEL(store));
}

void attachToGrid(GtkGrid *grid, gint x, gint y, const char *text) {
  GtkWidget *label = gtk_label_new(text);
  AtkObject *atkLabel = gtk_widget_get_accessible(label);
  atk_object_set_role(atkLabel, ATK_ROLE_TABLE_CELL);
  gtk_grid_attach(GTK_GRID(gridView), label, x, y, 1, 1);
  gtk_widget_show(label);
}

void fillNumericGrid() {
  static char gridStr[20][4][4];

  attachToGrid(GTK_GRID(gridView), 0, 0, "#1");
  attachToGrid(GTK_GRID(gridView), 1, 0, "#2");
  attachToGrid(GTK_GRID(gridView), 2, 0, "#3");
  attachToGrid(GTK_GRID(gridView), 3, 0, "#4");

  for (int row = 0; row < 20; ++row) {
    for (int col = 0; col < 4; ++col) {
      int number = row + col + 1;
      sprintf(gridStr[row][col], "%d", number);
      attachToGrid(GTK_GRID(gridView), col, row + 1, gridStr[row][col]);
    }
  }
}



void fillIconGrid() {
  GtkListStore *store = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
  GtkIconTheme *theme = gtk_icon_theme_get_default();

  gtk_icon_view_set_model(GTK_ICON_VIEW(iconView), GTK_TREE_MODEL(store));
  gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(iconView), 0);
  gtk_icon_view_set_text_column(GTK_ICON_VIEW(iconView), 1);
  
  for (auto name : iconList) {
    GdkPixbuf *icon = gtk_icon_theme_load_icon (theme, name, 64, GTK_ICON_LOOKUP_USE_BUILTIN, nullptr);
    if (icon == nullptr) {
//       std::cout << "Found an empty icon [" << name << "]" << std::endl;
      continue;
    }
    
    GtkTreeIter entry;
    gtk_list_store_append(store, &entry);
    gtk_list_store_set(store, &entry, 0, icon, 1, name, -1);
    g_object_unref(icon);
  }

}

std::string resolver(std::vector<std::string> &searchDirs, const std::string &filename) {
  for (std::vector<std::string>::iterator iter = searchDirs.begin(); iter != searchDirs.end(); ++iter) {
    std::string fullPath = *iter + "/" + filename;
    struct stat buff;
    
    if (stat(fullPath.c_str(), &buff) == 0) {
      return fullPath;
    }
  }
  return "";
}


class pidFile {
  std::ofstream fileHandle;
  std::string _fileName;
  public:
    pidFile(const std::string fileName, int pid) {
      _fileName = fileName;
      fileHandle.open(_fileName.c_str(), std::ios::out|std::ios::trunc);
      fileHandle << pid;
      fileHandle.close();
    }

    ~pidFile() {
     if (remove(_fileName.c_str()) != 0)
      std::cerr << "Unable to remove pid file." << std::endl;
    }
};

void sigHandlerCb(int s) {
  gtk_main_quit();
}

int main(int argc, char *argv[]) {
  // Disable the global menu on Ubuntu
  setenv("UBUNTU_MENUPROXY","0",1);
  struct sigaction sigHandler;
  sigHandler.sa_handler = sigHandlerCb;
  sigemptyset(&sigHandler.sa_mask);
  sigHandler.sa_flags = 0;
  sigaction(SIGINT, &sigHandler, nullptr);
  sigaction(SIGTERM, &sigHandler, nullptr);

  std::vector<char> buff;
  buff.resize(PATH_MAX, '\0');
  unsigned int len;
  if (readlink("/proc/self/exe", buff.data(), buff.size() - 1) == -1) {
    std::cerr << "Can't get proc name, exiting." << std::endl;
    return 1;
  }

  std::string procName(basename(buff.data()));
  std::cout << "Proc name: " << procName << std::endl;
  std::string pidFileName(g_get_tmp_dir());
  pidFileName.append("/");
  pidFileName.append(procName + ".pid");


  std::vector<std::string> searchPaths;
  searchPaths.push_back(".");
  searchPaths.push_back("..");
  std::string arg0(argv[0]);
  std::vector<char> tmp(arg0.begin(), arg0.end());
  tmp.push_back('\0');

  searchPaths.push_back(dirname(&tmp[0]));
  searchPaths.push_back("../images");
  searchPaths.push_back("../../images");
  searchPaths.push_back("../apps/test-gui/images/");
  

  bool forceStartup = false;
  for (int index = 1; index < argc; ++index) {
    std::string arg = argv[index];
    if (arg.compare(0, 10,  "--pidfile=") == 0) {
      pidFileName = arg.substr(10, std::string::npos);
      if (pidFileName.empty()) {
        std::cerr << "Empty pid filename." << std::endl;
      }
      continue;
    }

    if (arg.compare(0, 7, "--force") == 0) {
      forceStartup = true;
      continue;
    }
    searchPaths.push_back(argv[index]);
  }

  {
    std::ofstream checkPidFile(pidFileName.c_str(), std::ios::in);
    if (!forceStartup && checkPidFile.good()) {
      std::cerr << "The pid file [" << pidFileName <<"] already exists, please remove the file or specify different pid filename/location."  << std::endl;
      return 1;
    }
  }

  pidFile pid(pidFileName, ::getpid());
  std::cout << "Pid file: " << pidFileName << std::endl;
 
  std::string gladeFile = resolver(searchPaths, "gui_test_application.glade");
  std::string imageFile = resolver(searchPaths, "shield.png");
  if (gladeFile == "") {
    std::cerr << "Unable to find file: gui_test_application.glade" << std::endl;
    return 1;
  }
  
  gtk_init(&argc, &argv);

  GtkBuilder *builder = gtk_builder_new();
  gtk_builder_add_from_file (builder, gladeFile.c_str(), nullptr);

  //  Get the needed widgets
  window = GTK_WIDGET(gtk_builder_get_object(builder, "mainwindow"));
  gtk_window_set_title((GtkWindow*)window, "GUI Test App");
  image1 = GTK_IMAGE(gtk_builder_get_object(builder, "image1"));
  image1_eventbox = GTK_WIDGET(gtk_builder_get_object(builder, "image1_eventbox"));
  popup_menu = GTK_MENU(gtk_builder_get_object(builder, "image1_popup_menu"));
  treeView = GTK_WIDGET(gtk_builder_get_object(builder, "treeview1"));
  gridView = GTK_WIDGET(gtk_builder_get_object(builder, "grid1"));
  iconView = GTK_WIDGET(gtk_builder_get_object(builder, "iconview1"));
  button1 = GTK_WIDGET(gtk_builder_get_object(builder, "button1"));
  button3 = GTK_WIDGET(gtk_builder_get_object(builder, "button3"));
  button4 = GTK_WIDGET(gtk_builder_get_object(builder, "button4"));
  entry2 = GTK_WIDGET(gtk_builder_get_object(builder, "entry2"));
  dialog1 = GTK_DIALOG(gtk_builder_get_object(builder, "dialog1"));
  gtk_window_set_transient_for((GtkWindow*)dialog1, (GtkWindow*)window);
  checkbutton2 = GTK_WIDGET(gtk_builder_get_object(builder, "checkbutton2"));
  radiobutton2 = GTK_WIDGET(gtk_builder_get_object(builder, "radiobutton2"));
  radiobutton4 = GTK_WIDGET(gtk_builder_get_object(builder, "radiobutton4"));
  gtk_image_set_from_file(image1, imageFile.c_str());
  
  //  Connect the signals to the proper callbacks
  g_signal_connect (window, "destroy", G_CALLBACK (on_window_main_destroy), nullptr);
  g_signal_connect (image1_eventbox, "button-press-event", G_CALLBACK (image1_button_release_event_cb), nullptr);
  g_signal_connect (button1, "button-release-event", G_CALLBACK (button1_button_release_event_cb), nullptr);
  g_signal_connect (button3, "button-release-event", G_CALLBACK (button3_button_release_event_cb), nullptr);
  g_signal_connect (button3, "clicked", G_CALLBACK (button3_button_clicked_cb), nullptr);

  g_signal_connect (button4, "button-release-event", G_CALLBACK (button4_button_release_event_cb), nullptr);
  g_signal_connect_after (dialog1, "delete-event", G_CALLBACK(dialog1_delete_event_cb), nullptr);
  g_signal_connect (dialog1, "response", G_CALLBACK(dialog1_response_event_cb), nullptr);
  g_signal_connect (checkbutton2, "clicked", G_CALLBACK(checkbox_clicked_event_cb), nullptr);
  g_signal_connect (radiobutton2, "clicked", G_CALLBACK(radiobutton_clicked_event_cb), nullptr);
  g_signal_connect (radiobutton4, "clicked", G_CALLBACK(radiobutton_clicked_event_cb), nullptr);

  gtk_window_set_default_size(GTK_WINDOW(window), 1400, 900);
  
//   gtk_builder_connect_signals(builder, nullptr);
  fillTable();

  fillNumericGrid();
  fillIconGrid();
  
  GtkBox *box = GTK_BOX(gtk_builder_get_object(builder, "box4"));
  gtk_box_set_homogeneous(box, true);
  
  g_object_unref(builder);
  
  gtk_widget_show(window);
  gtk_main();

  gtk_widget_destroy (GTK_WIDGET(dialog1));

     

  return 0;
}




