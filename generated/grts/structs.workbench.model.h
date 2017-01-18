#pragma once

#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

#include "grt.h"

#ifdef _WIN32
#pragma warning(disable : 4355) // 'this' : used in base member initializer list
#ifdef GRT_STRUCTS_WORKBENCH_MODEL_EXPORT
#define GRT_STRUCTS_WORKBENCH_MODEL_PUBLIC __declspec(dllexport)
#else
#define GRT_STRUCTS_WORKBENCH_MODEL_PUBLIC __declspec(dllimport)
#endif
#else
#define GRT_STRUCTS_WORKBENCH_MODEL_PUBLIC
#endif

#include "grts/structs.h"
#include "grts/structs.model.h"

class workbench_model_ImageFigure;
typedef grt::Ref<workbench_model_ImageFigure> workbench_model_ImageFigureRef;
class workbench_model_NoteFigure;
typedef grt::Ref<workbench_model_NoteFigure> workbench_model_NoteFigureRef;

namespace mforms {
  class Object;
};

namespace grt {
  class AutoPyObject;
};

/** a model figure representing an image */
class GRT_STRUCTS_WORKBENCH_MODEL_PUBLIC workbench_model_ImageFigure : public model_Figure {
  typedef model_Figure super;

public:
  class ImplData;
  friend class ImplData;
  workbench_model_ImageFigure(grt::MetaClass *meta = 0)
    : model_Figure(meta ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _filename(""),
      _keepAspectRatio(0),
      _data(nullptr)

  {
  }

  virtual ~workbench_model_ImageFigure();

  static std::string static_class_name() {
    return "workbench.model.ImageFigure";
  }

  /** Getter for attribute filename

    the image file name
   \par In Python:
value = obj.filename
   */
  grt::StringRef filename() const {
    return _filename;
  }
  /** Setter for attribute filename

    the image file name
    \par In Python:
obj.filename = value
   */
  virtual void filename(const grt::StringRef &value) {
    grt::ValueRef ovalue(_filename);
    _filename = value;
    member_changed("filename", ovalue, value);
  }

  /** Getter for attribute keepAspectRatio


   \par In Python:
value = obj.keepAspectRatio
   */
  grt::IntegerRef keepAspectRatio() const {
    return _keepAspectRatio;
  }
  /** Setter for attribute keepAspectRatio


    \par In Python:
obj.keepAspectRatio = value
   */
  virtual void keepAspectRatio(const grt::IntegerRef &value);

  /** Method.
  \param name
  \return

   */
  virtual grt::StringRef setImageFile(const std::string &name);

  ImplData *get_data() const {
    return _data;
  }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:
  grt::StringRef _filename;
  grt::IntegerRef _keepAspectRatio;

private: // wrapper methods for use by grt
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_model_ImageFigure());
  }

  static grt::ValueRef call_setImageFile(grt::internal::Object *self, const grt::BaseListRef &args) {
    return dynamic_cast<workbench_model_ImageFigure *>(self)->setImageFile(grt::StringRef::cast_from(args[0]));
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (!meta)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_model_ImageFigure::create);
    {
      void (workbench_model_ImageFigure::*setter)(const grt::StringRef &) = &workbench_model_ImageFigure::filename;
      grt::StringRef (workbench_model_ImageFigure::*getter)() const = &workbench_model_ImageFigure::filename;
      meta->bind_member("filename",
                        new grt::MetaClass::Property<workbench_model_ImageFigure, grt::StringRef>(getter, setter));
    }
    {
      void (workbench_model_ImageFigure::*setter)(const grt::IntegerRef &) =
        &workbench_model_ImageFigure::keepAspectRatio;
      grt::IntegerRef (workbench_model_ImageFigure::*getter)() const = &workbench_model_ImageFigure::keepAspectRatio;
      meta->bind_member("keepAspectRatio",
                        new grt::MetaClass::Property<workbench_model_ImageFigure, grt::IntegerRef>(getter, setter));
    }
    meta->bind_method("setImageFile", &workbench_model_ImageFigure::call_setImageFile);
  }
};

/** a model figure representing a text box */
class GRT_STRUCTS_WORKBENCH_MODEL_PUBLIC workbench_model_NoteFigure : public model_Figure {
  typedef model_Figure super;

public:
  class ImplData;
  friend class ImplData;
  workbench_model_NoteFigure(grt::MetaClass *meta = 0)
    : model_Figure(meta ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _font(""),
      _text(""),
      _textColor(""),
      _data(nullptr)

  {
  }

  virtual ~workbench_model_NoteFigure();

  static std::string static_class_name() {
    return "workbench.model.NoteFigure";
  }

  /** Getter for attribute font

    the font to be used for the note
   \par In Python:
value = obj.font
   */
  grt::StringRef font() const {
    return _font;
  }
  /** Setter for attribute font

    the font to be used for the note
    \par In Python:
obj.font = value
   */
  virtual void font(const grt::StringRef &value);

  /** Getter for attribute text

    the text contents
   \par In Python:
value = obj.text
   */
  grt::StringRef text() const {
    return _text;
  }
  /** Setter for attribute text

    the text contents
    \par In Python:
obj.text = value
   */
  virtual void text(const grt::StringRef &value);

  /** Getter for attribute textColor

    the text color
   \par In Python:
value = obj.textColor
   */
  grt::StringRef textColor() const {
    return _textColor;
  }
  /** Setter for attribute textColor

    the text color
    \par In Python:
obj.textColor = value
   */
  virtual void textColor(const grt::StringRef &value);

  ImplData *get_data() const {
    return _data;
  }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:
  grt::StringRef _font;
  grt::StringRef _text;
  grt::StringRef _textColor;

private: // wrapper methods for use by grt
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_model_NoteFigure());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (!meta)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_model_NoteFigure::create);
    {
      void (workbench_model_NoteFigure::*setter)(const grt::StringRef &) = &workbench_model_NoteFigure::font;
      grt::StringRef (workbench_model_NoteFigure::*getter)() const = &workbench_model_NoteFigure::font;
      meta->bind_member("font",
                        new grt::MetaClass::Property<workbench_model_NoteFigure, grt::StringRef>(getter, setter));
    }
    {
      void (workbench_model_NoteFigure::*setter)(const grt::StringRef &) = &workbench_model_NoteFigure::text;
      grt::StringRef (workbench_model_NoteFigure::*getter)() const = &workbench_model_NoteFigure::text;
      meta->bind_member("text",
                        new grt::MetaClass::Property<workbench_model_NoteFigure, grt::StringRef>(getter, setter));
    }
    {
      void (workbench_model_NoteFigure::*setter)(const grt::StringRef &) = &workbench_model_NoteFigure::textColor;
      grt::StringRef (workbench_model_NoteFigure::*getter)() const = &workbench_model_NoteFigure::textColor;
      meta->bind_member("textColor",
                        new grt::MetaClass::Property<workbench_model_NoteFigure, grt::StringRef>(getter, setter));
    }
  }
};

inline void register_structs_workbench_model_xml() {
  grt::internal::ClassRegistry::register_class<workbench_model_ImageFigure>();
  grt::internal::ClassRegistry::register_class<workbench_model_NoteFigure>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_workbench_model_xml {
  _autoreg__structs_workbench_model_xml() {
    register_structs_workbench_model_xml();
  }
} __autoreg__structs_workbench_model_xml;
#endif

#ifndef _WIN32
#pragma GCC diagnostic pop
#endif
