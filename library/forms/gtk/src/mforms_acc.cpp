/*
 * Copyright (c) 2017, 2018 Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "../mforms_acc.h"
#include <gtk/gtk-a11y.h>
#include <atk/atk.h>
#include <atkmm.h>
#pragma GCC diagnostic push
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
namespace mforms {
  namespace gtk {

    static AtkRole convertAccessibleRole(base::Accessible::Role be_role) {
      AtkRole role = ATK_ROLE_INVALID;

      switch (be_role) {
        case base::Accessible::RoleNone:
          role = ATK_ROLE_INVALID;
          break;

        case base::Accessible::Window:
          role = ATK_ROLE_WINDOW;
          break;

        case base::Accessible::Pane:
          role = ATK_ROLE_PANEL;
          break;

        case base::Accessible::Link:
          role = ATK_ROLE_UNKNOWN;
          break;

        case base::Accessible::List:
          role = ATK_ROLE_LIST;
          break;

        case base::Accessible::ListItem:
          role = ATK_ROLE_LIST_ITEM;
          break;

        case base::Accessible::PushButton:
          role = ATK_ROLE_PUSH_BUTTON;
          break;

        case base::Accessible::StaticText:
          role = ATK_ROLE_LABEL;
          break;

        case base::Accessible::Text:
          role = ATK_ROLE_TEXT;
          break;

        case base::Accessible::Outline:
          role = ATK_ROLE_TREE_TABLE;
          break;

        case base::Accessible::OutlineButton:
          role = ATK_ROLE_UNKNOWN;
          break;

        case base::Accessible::OutlineItem:
        default:
          role = ATK_ROLE_UNKNOWN;

      }
      return role;
    }

    static gpointer mforms_object_accessible_parent_class = nullptr;

    static void mforms_object_accessible_init(mformsObjectAccessible *accessible) {
      mformsObjectAccessiblePrivate *priv = MFORMS_OBJECT_ACCESSIBLE_GET_PRIVATE(accessible);
      priv->mfoacc = nullptr;
    }

    static AtkStateSet *mforms_object_accessible_ref_state_set(AtkObject *accessible) {
      AtkStateSet *state_set = ATK_OBJECT_CLASS(mforms_object_accessible_parent_class)->ref_state_set(accessible);

      GtkWidget *widget = gtk_accessible_get_widget(GTK_ACCESSIBLE(accessible));
      if (widget == NULL)
        atk_state_set_add_state(state_set, ATK_STATE_DEFUNCT);
      else {

        atk_state_set_add_state(state_set, ATK_STATE_DEFAULT);
        atk_state_set_add_state(state_set, ATK_STATE_VISIBLE);
        // This is special situation, we paint our widgets inside drawbox
        // hence if parent widget to the drawbox is mapped, then drawbox
        // should be in the same state.
        auto parent = gtk_widget_get_parent(widget);
        if (parent != nullptr && gtk_widget_get_mapped(parent))
          atk_state_set_add_state(state_set, ATK_STATE_SHOWING);
      }

      return state_set;
    }

    static void mforms_object_accessible_initialize(AtkObject *obj, gpointer data) {
      if (data != nullptr)
        ATK_OBJECT_CLASS(mforms_object_accessible_parent_class)->initialize(obj, data);
    }

    static void mforms_object_accessible_finalize(GObject *object) {
      mformsObjectAccessiblePrivate *priv = MFORMS_OBJECT_ACCESSIBLE_GET_PRIVATE(object);

      if (priv->mfoacc != nullptr) {
        delete priv->mfoacc;
        priv->mfoacc = 0;
      }

      G_OBJECT_CLASS(mforms_object_accessible_parent_class)->finalize(object);
    }

    static void mforms_object_accessible_widget_set(GtkAccessible *accessible) {
      GtkWidget *widget = gtk_accessible_get_widget(accessible);
      if (widget == NULL)
        return;

      mformsObjectAccessiblePrivate *priv = MFORMS_OBJECT_ACCESSIBLE_GET_PRIVATE(accessible);
      if (priv->mfoacc != nullptr)
        delete priv->mfoacc;
      priv->mfoacc = new mformsGTKAccessible(accessible, mformsGTK::FromWidget(widget)->getmformsAcc());
    }

    static void mforms_object_accessible_widget_unset(GtkAccessible *accessible) {
      GtkWidget *widget = gtk_accessible_get_widget(accessible);
      if (widget == NULL)
        return;

      mformsObjectAccessiblePrivate *priv = MFORMS_OBJECT_ACCESSIBLE_GET_PRIVATE(accessible);
      delete priv->mfoacc;
      priv->mfoacc = nullptr;
    }

    static void mforms_object_accessible_class_init(mformsObjectAccessibleClass *klass) {
      GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
      AtkObjectClass *object_class = ATK_OBJECT_CLASS(klass);

      object_class->ref_state_set = mforms_object_accessible_ref_state_set;
      object_class->initialize = mforms_object_accessible_initialize;
      gobject_class->finalize = mforms_object_accessible_finalize;

      GtkAccessibleClass *accessible_class = GTK_ACCESSIBLE_CLASS(klass);
      accessible_class->widget_set = mforms_object_accessible_widget_set;
      accessible_class->widget_unset = mforms_object_accessible_widget_unset;

      object_class->get_name = mformsGTKAccessible::getName;
      object_class->get_description = mformsGTKAccessible::getDescription;
      object_class->get_role = mformsGTKAccessible::getRole;
      object_class->get_n_children = mformsGTKAccessible::getNChildren;
      object_class->ref_child = mformsGTKAccessible::refChild;

      mforms_object_accessible_parent_class = g_type_class_peek_parent(klass);

      g_type_class_add_private(klass, sizeof(mformsObjectAccessiblePrivate));
    }

    // @p parent_type is only required on GTK 3.2 to 3.6, and only on the first call
    GType mforms_object_accessible_get_type(GType parent_type G_GNUC_UNUSED) {
      static volatile gsize type_id_result = 0;

      if (g_once_init_enter(&type_id_result)) {
        GTypeInfo tinfo = { 0, /* class size */
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) mforms_object_accessible_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        0, /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) mforms_object_accessible_init, /* instance init */
        NULL /* value table */
        };

        // good, we have gtk-a11y.h, we can use that
        GType derived_atk_type = GTK_TYPE_CONTAINER_ACCESSIBLE;
        tinfo.class_size = sizeof(GtkContainerAccessibleClass);
        tinfo.instance_size = sizeof(GtkContainerAccessible);

        GType type_id = g_type_register_static(derived_atk_type, "mformsObjectAccessible", &tinfo, (GTypeFlags) 0);

        const GInterfaceInfo atk_action_info = { (GInterfaceInitFunc) mformsGTKAccessible::AtkActionIface::init,
            (GInterfaceFinalizeFunc) NULL,
            NULL };

        const GInterfaceInfo atk_component_info = { (GInterfaceInitFunc) mformsGTKAccessible::AtkComponentIface::init,
            (GInterfaceFinalizeFunc) NULL, NULL };

        g_type_add_interface_static(type_id, ATK_TYPE_ACTION, &atk_action_info);
        g_type_add_interface_static(type_id, ATK_TYPE_COMPONENT, &atk_component_info);

        g_once_init_leave(&type_id_result, type_id);
      }

      return type_id_result;
    }

    mformsGTKAccessible::mformsGTKAccessible(GtkAccessible *accessible, base::Accessible *acc)
        : _accessible(accessible), _mformsAcc(acc) {
    }

    static std::map<base::Accessible*, AtkObject*> childMapping;

    mformsGTKAccessible::~mformsGTKAccessible() {
      for (const auto &it : _children) {
        auto childIt = childMapping.find(it);
        if (childIt != childMapping.end()) {
          auto *widget = gtk_accessible_get_widget(GTK_ACCESSIBLE(childIt->second));
          if (widget != nullptr)
            g_object_ref_sink(widget);  // This is only a floating ref, we remove it.

          g_object_unref(childIt->second);  // AtkObject is real, we have to unref it.
        }
      }
      _children.clear();
    }

    base::Accessible* mformsGTKAccessible::getmformsAccessible(AtkObject *accessible) {
      GtkWidget *widget = gtk_accessible_get_widget(GTK_ACCESSIBLE(accessible));
      if (widget == NULL)
        return nullptr;

      MFormsObject *mfo = MFORMSOBJECT(widget);
      if (mfo != nullptr && mfo->pmforms != nullptr)
        return dynamic_cast<base::Accessible*>(mfo->pmforms->_owner);

      return nullptr;
    }

    void mformsGTKAccessible::AtkActionIface::init(::AtkActionIface *iface) {
      iface->do_action = mformsGTKAccessible::AtkActionIface::doAction;
      iface->get_n_actions = mformsGTKAccessible::AtkActionIface::getNActions;
      iface->get_name = mformsGTKAccessible::AtkActionIface::getName;
    }

    gboolean mformsGTKAccessible::AtkActionIface::doAction(AtkAction *action, gint i) {
      auto *thisAccessible = FromAccessible(reinterpret_cast<GtkAccessible*>(action));
      if (thisAccessible != nullptr && thisAccessible->_mformsAcc != nullptr) {
        thisAccessible->_mformsAcc->accessibilityDoDefaultAction();
        return true;
      }

      return false;
    }

    gint mformsGTKAccessible::AtkActionIface::getNActions(AtkAction *action) {
      auto *thisAccessible = FromAccessible(reinterpret_cast<GtkAccessible*>(action));
      if (thisAccessible != nullptr && thisAccessible->_mformsAcc != nullptr)
        return 1;

      return 0;
    }

    const gchar* mformsGTKAccessible::AtkActionIface::getName(AtkAction *action, gint i) {
      if (i == 0) {
        auto *thisAccessible = FromAccessible(reinterpret_cast<GtkAccessible*>(action));
        if (thisAccessible != nullptr) {
          if (!thisAccessible->_mformsAcc->getAccessibilityDefaultAction().empty() && thisAccessible->_accActionName.empty())
            thisAccessible->_accActionName = thisAccessible->_mformsAcc->getAccessibilityDefaultAction();
          return thisAccessible->_accActionName.c_str();
        }
      }

      return nullptr;
    }

    void mformsGTKAccessible::AtkComponentIface::init(::AtkComponentIface *iface) {
      iface->get_position = mformsGTKAccessible::AtkComponentIface::getPosition;
      iface->get_size = mformsGTKAccessible::AtkComponentIface::getSize;
      iface->get_extents = mformsGTKAccessible::AtkComponentIface::getExtents;
    }

    void mformsGTKAccessible::AtkComponentIface::getPosition(AtkComponent *component, gint *x, gint *y, AtkCoordType coord_type) {
      gint width, height;
      getExtents(component, x, y, &width, &height, coord_type);
    }

    void mformsGTKAccessible::AtkComponentIface::getSize(AtkComponent *component, gint *width, gint *height) {
      gint x, y;
      getExtents(component, &x, &y, width, height, ATK_XY_SCREEN);
    }

    void mformsGTKAccessible::AtkComponentIface::getExtents(AtkComponent *component, gint *x, gint *y, gint *width, gint *height, AtkCoordType coord_type) {
      auto *thisAccessible = FromAccessible(reinterpret_cast<GtkAccessible*>(component));
      if (thisAccessible != nullptr) {
        auto mGtk = mformsGTK::FromWidget(gtk_accessible_get_widget(GTK_ACCESSIBLE(component)));
        auto bounds = thisAccessible->_mformsAcc->getAccessibilityBounds();
        *width = bounds.width();
        *height = bounds.height();

        *x = bounds.pos.x;
        *y = bounds.pos.y;
        GtkWidget *widget = mGtk->_windowMain;
        GdkWindow *window = nullptr;
        if (gtk_widget_get_parent(widget) == nullptr || mGtk->_owner == nullptr)
          window = gtk_widget_get_window(widget);
        else
          window = gtk_widget_get_parent_window(widget);

        gint xWindow = 0, yWindow = 0;
        gdk_window_get_origin(window, &xWindow, &yWindow);
        *x += xWindow;
        *y += yWindow;
        if (coord_type == ATK_XY_WINDOW) {
          window = gdk_window_get_toplevel (gtk_widget_get_window (widget));
          gint xTopLevel = 0, yTopLevel = 0;
          gdk_window_get_origin (window, &xTopLevel, &yTopLevel);

          *x -= xTopLevel;
          *y -= yTopLevel;
        }
      }
    }

    static AtkObject *mforms_object_accessible_new(GType parent_type, GObject *obj) {
      g_return_val_if_fail(MFORMSOBJECT_IS_OBJECT(obj), NULL);

      AtkObject *accessible = (AtkObject *) g_object_new(mforms_object_accessible_get_type(parent_type), "widget", obj,
      NULL);
      atk_object_initialize(accessible, obj);

      return accessible;
    }

    AtkObject *mformsGTKAccessible::WidgetGetAccessibleImpl(GtkWidget *widget, AtkObject **cache,
                                                            gpointer widget_parent_class G_GNUC_UNUSED) {
      if (*cache != nullptr) {
        return *cache;
      }

      *cache = mforms_object_accessible_new(0, G_OBJECT(widget));

      return *cache;
    }

    const gchar* mformsGTKAccessible::getName(AtkObject *accessible) {
      auto mformsGtkAcc = FromAccessible(accessible);
      if (mformsGtkAcc != nullptr && mformsGtkAcc->_mformsAcc != nullptr) {
        if (mformsGtkAcc->_name.empty())
          mformsGtkAcc->_name = mformsGtkAcc->_mformsAcc->getAccessibilityName();

        if (!mformsGtkAcc->_name.empty())
          return mformsGtkAcc->_name.c_str();
      }

      return ATK_OBJECT_CLASS(mforms_object_accessible_parent_class)->get_name(accessible);
    }

    const gchar* mformsGTKAccessible::getDescription(AtkObject *accessible) {
      auto mformsGtkAcc = FromAccessible(accessible);
      if (mformsGtkAcc != nullptr && mformsGtkAcc->_mformsAcc != nullptr) {
        if (mformsGtkAcc->_description.empty())
          mformsGtkAcc->_description = mformsGtkAcc->_mformsAcc->getAccessibilityDescription();

        if (!mformsGtkAcc->_description.empty())
          return mformsGtkAcc->_description.c_str();
      }

      return ATK_OBJECT_CLASS(mforms_object_accessible_parent_class)->get_description(accessible);
    }

    AtkRole mformsGTKAccessible::getRole(AtkObject *accessible) {
      auto acc = getmformsAccessible(accessible);

      if (acc != nullptr && convertAccessibleRole(acc->getAccessibilityRole()) != ATK_ROLE_UNKNOWN)
        return convertAccessibleRole(acc->getAccessibilityRole());
      else {  //we need to check if maybe it's not a "virtual" widget
        for (const auto &it : childMapping) {
          if (it.second == accessible && convertAccessibleRole(it.first->getAccessibilityRole()) != ATK_ROLE_UNKNOWN)
            return convertAccessibleRole(it.first->getAccessibilityRole());
        }
      }

      return ATK_OBJECT_CLASS(mforms_object_accessible_parent_class)->get_role(accessible);
    }

    gint mformsGTKAccessible::getNChildren(AtkObject *accessible) {
      auto baseChildCount = ATK_OBJECT_CLASS(mforms_object_accessible_parent_class)->get_n_children(accessible);

      auto acc = getmformsAccessible(accessible);
      if (acc != nullptr)
        return acc->getAccessibilityChildCount() + baseChildCount;

      return baseChildCount;
    }

    AtkObject* mformsGTKAccessible::refChild(AtkObject *accessible, gint i) {
      auto baseChildCount = ATK_OBJECT_CLASS(mforms_object_accessible_parent_class)->get_n_children(accessible);
      if (i >= baseChildCount) {
        int childPos = i - baseChildCount;
        auto acc = getmformsAccessible(accessible);
        if (acc != nullptr) {
          auto accChild = acc->getAccessibilityChild(childPos);
          if (accChild != nullptr) {
            auto it = childMapping.find(accChild);
            if (it != childMapping.end())
              return (AtkObject*) g_object_ref(it->second);

            auto widget = mforms_new();
            auto parentWidget = gtk_accessible_get_widget(GTK_ACCESSIBLE(accessible));
            gtk_widget_set_parent(widget, parentWidget);
            auto mGtk = mformsGTK::FromWidget(widget);
            mGtk->_windowMain = parentWidget;
            auto wgtAcc = gtk_widget_get_accessible(widget);
            auto mformsGtkAcc = FromAccessible(wgtAcc);
            mformsGtkAcc->_mformsAcc = accChild;

            childMapping.insert( { accChild, (AtkObject*) g_object_ref(wgtAcc) });

            auto thisPtr = FromAccessible(accessible);
            thisPtr->_children.push_back(accChild);  // we need to store ptr cause in the d-tor we can't access children :(
            return wgtAcc;
          }
        }
      }

      return ATK_OBJECT_CLASS(mforms_object_accessible_parent_class)->ref_child(accessible, i);
    }

    mformsGTKAccessible* mformsGTKAccessible::FromAccessible(GtkAccessible *accessible) {
      GtkWidget *widget = gtk_accessible_get_widget(accessible);
      if (!widget)
        return 0;

      return MFORMS_OBJECT_ACCESSIBLE_GET_PRIVATE(accessible)->mfoacc;
    }

    mformsGTKAccessible* mformsGTKAccessible::FromAccessible(AtkObject *accessible) {
      return FromAccessible(GTK_ACCESSIBLE(accessible));
    }

    mformsGTK::mformsGTK(_MFormsObject *mfo)
        : _accessible(nullptr), _owner(nullptr) {
      _mfo = mfo;
      _windowMain = GTK_WIDGET(mfo);
    }

    mformsGTK::~mformsGTK() {
    }

    AtkObject* mformsGTK::GetAccessibleThis(GtkWidget *widget) {
      return mformsGTKAccessible::WidgetGetAccessibleImpl(widget, &_accessible, mforms_object_accessible_parent_class);
    }

    void mformsGTK::SetMFormsOwner(mforms::View *view) {
      _owner = view;
    }

    base::Accessible* mformsGTK::getmformsAcc() {
      return dynamic_cast<base::Accessible*>(_owner);
    }

    AtkObject* mformsGTK::GetAccessible(GtkWidget *widget) {
      MFormsObject *mfo = MFORMSOBJECT(widget);
      if (mfo != nullptr && mfo->pmforms != nullptr)
        return mfo->pmforms->GetAccessibleThis(widget);
      return nullptr;
    }

    void mformsGTK::ClassInit(GObjectClass* object_class, GtkWidgetClass *widget_class,
                              GtkEventBoxClass *container_class) {
      widget_class->get_accessible = GetAccessible;
      object_class->finalize = Destroy;
    }

    static GObjectClass *mforms_class_parent_class = nullptr;

    void mformsGTK::Destroy(GObject *object) {
      try {
        MFormsObject *mfo = MFORMSOBJECT(object);

        if (mfo->pmforms == nullptr)
          return;

        mfo->pmforms->Finalise();

        delete mfo->pmforms;
        mfo->pmforms = 0;
        mforms_class_parent_class->finalize(object);
      } catch (...) {
        // no need to log
      }
    }

    mformsGTK *mformsGTK::FromWidget(GtkWidget *widget) {
      MFormsObject *mfo = MFORMSOBJECT(widget);
      return mfo->pmforms;
    }

    void mformsGTK::Finalise() {
      if (_accessible != nullptr) {
        gtk_accessible_set_widget(GTK_ACCESSIBLE(_accessible), nullptr);
        g_object_unref(_accessible);
        _accessible = nullptr;
      }
    }

    void mforms_class_init(MFormsClass *klass) {
      try {
        GObjectClass *gobject_class = (GObjectClass*) klass;
        GtkWidgetClass *widget_class = (GtkWidgetClass*) klass;
        GtkEventBoxClass *container_class = (GtkEventBoxClass*) klass;

        mforms_class_parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(klass));
        mformsGTK::ClassInit(gobject_class, widget_class, container_class);
      } catch (...) {
      }
    }

    void mforms_init(MFormsObject *mf) {
      try {
        mf->pmforms = new mformsGTK(mf);
      } catch (...) {
      }
    }

    GType mforms_get_type() {
      static GType mforms_type = 0;
      try {

        if (!mforms_type) {
          mforms_type = g_type_from_name("MFormsObject");
          if (!mforms_type) {
            static GTypeInfo mforms_info = { (guint16) sizeof(MFormsObjectClass),
            NULL,  //(GBaseInitFunc)
                NULL,  //(GBaseFinalizeFunc)
                (GClassInitFunc) mforms_class_init,
                NULL,  //(GClassFinalizeFunc)
                NULL,  //gconstpointer data
                (guint16) sizeof(MFormsObject), 0,  //n_preallocs
                (GInstanceInitFunc) mforms_init,
                NULL  //(GTypeValueTable*)
                };
            mforms_type = g_type_register_static(
            GTK_TYPE_EVENT_BOX,
                                                 "MFormsObject", &mforms_info, (GTypeFlags) 0);
          }
        }

      } catch (...) {
      }
      return mforms_type;
    }

    GtkWidget* mforms_new() {
      return GTK_WIDGET(g_object_new(mforms_get_type(), NULL));
    }
  }
}

#pragma GCC diagnostic pop
