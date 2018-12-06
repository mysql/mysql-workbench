/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include <atk/atk.h>
#include <atkmm/selection.h>
#include <glib.h>
#include "lf_mforms.h"
#include "base/accessibility.h"
#include "mforms/view.h"

#define MFORMSOBJECT(obj) G_TYPE_CHECK_INSTANCE_CAST (obj, mforms_get_type (), MFormsObject)
#define MFORMSOBJECT_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, mforms_get_type (), MFormsClass)
#define IS_MFORMSOBJECT(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, mforms_get_type ())

#define MFORMSOBJECT_TYPE_OBJECT             (mforms_get_type())
#define MFORMSOBJECT_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), MFORMSOBJECT_TYPE_OBJECT, MFormsObject))
#define MFORMSOBJECT_IS_OBJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MFORMSOBJECT_TYPE_OBJECT))
#define MFORMSOBJECT_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), MFORMSOBJECT_TYPE_OBJECT, MFormsObjectClass))
#define MFORMSOBJECT_IS_OBJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), MFORMSOBJECT_TYPE_OBJECT))
#define MFORMSOBJECT_OBJECT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), MFORMSOBJECT_TYPE_OBJECT, MFormsObjectClass))

#define MFORMS_OBJECT_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), MFORMS_TYPE_OBJECT_ACCESSIBLE, mformsObjectAccessible))
#define MFORMS_TYPE_OBJECT_ACCESSIBLE (mforms_object_accessible_get_type(0))

namespace mforms {
  namespace gtk {

    typedef GtkAccessible mformsObjectAccessible;
    typedef GtkAccessibleClass mformsObjectAccessibleClass;

    typedef struct _MFormsObject MFormsObject;

    class mformsGTK;

    class mformsGTKAccessible {
    public:
      class AtkActionIface {
      public:
        static void init(::AtkActionIface *iface);

      private:
        AtkActionIface() {};

        static gboolean doAction(AtkAction *action, gint i);
        static gint getNActions(AtkAction *action);
        static const gchar* getName(AtkAction *action, gint i);
      };

      class AtkComponentIface {
      public:
        static void init(::AtkComponentIface *iface);

      private:
        AtkComponentIface() {};

        static void getPosition(AtkComponent *component, gint *x, gint *y, AtkCoordType coord_type);
        static void getSize(AtkComponent *component, gint *width, gint *height);
        static void getExtents(AtkComponent *component, gint *x, gint *y, gint *width, gint *height, AtkCoordType coord_type);
        static gboolean grabFocus(AtkComponent *component);
      };

      class AtkTextIface {
       public:
        static void init(::AtkTextIface *iface);
       private:
        AtkTextIface() {};
        static gchar* getText(AtkText* text, gint start, gint end);
        static gint getCharacterCount(AtkText* text);
      };

      mformsGTKAccessible(GtkAccessible *accessible, base::Accessible *acc);
      virtual ~mformsGTKAccessible();
      static AtkObject *WidgetGetAccessibleImpl(GtkWidget *widget, AtkObject **cache, gpointer widget_parent_class);
      static base::Accessible* getmformsAccessible(AtkObject *accessible);
      static const gchar* getName(AtkObject *accessible);
      static const gchar* getDescription(AtkObject *accessible);
      static AtkRole getRole(AtkObject *accessible);
      static gint getNChildren(AtkObject *accessible);
      static AtkObject* refChild(AtkObject *accessible, gint i);
      static mformsGTKAccessible *FromAccessible(GtkAccessible *accessible);
      static mformsGTKAccessible *FromAccessible(AtkObject *accessible);

    protected:
      GtkAccessible *_accessible;
      base::Accessible *_mformsAcc;
      std::string _name;
      std::string _description;
      std::string _accActionName;
      std::vector<base::Accessible*> _children;
    };

    struct mformsObjectAccessiblePrivate {
      mformsGTKAccessible *mfoacc;
    };

    class mformsGTK {
      friend class mformsGTKAccessible;
    public:
      mformsGTK(_MFormsObject *mfo);
      virtual ~mformsGTK();

      static AtkObject* GetAccessible(GtkWidget *widget);
      static void ClassInit(GObjectClass* object_class, GtkWidgetClass *widget_class,
                            GtkEventBoxClass *container_class);
      static void Destroy(GObject *object);
      static mformsGTK* FromWidget(GtkWidget *widget);
      AtkObject* GetAccessibleThis(GtkWidget *widget);
      void SetMFormsOwner(mforms::View *view);
      base::Accessible* getmformsAcc();
    protected:
      _MFormsObject *_mfo;
      GtkWidget* _windowMain;
      AtkObject *_accessible;
      mforms::View *_owner;
      virtual void Finalise();
    };

    struct _MFormsObject {
      GtkEventBox parent;
      mformsGTK *pmforms;
    };

    typedef struct _MFormsClass MFormsObjectClass;
    typedef struct _MFormsClass MFormsClass;

    struct _MFormsClass {
      GtkEventBoxClass parent_class;
      void (*set_backend)(MFormsObject *mfo, mforms::View *view);
    };

    void mforms_class_init(MFormsClass *klass);
    void mforms_init(MFormsObject *mf);

    GType mforms_get_type();
    GType mforms_object_accessible_get_type(GType parent_type G_GNUC_UNUSED);
    static gint mformsObject_private_offset = 0;
    static inline mformsObjectAccessiblePrivate* mforms_get_instance_private(mformsObjectAccessible *self) {
      return reinterpret_cast<mformsObjectAccessiblePrivate*>(G_STRUCT_MEMBER_P(self, mformsObject_private_offset));
    }

    GtkWidget* mforms_new();
  }
}
