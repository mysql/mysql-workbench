/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "MFFileChooser.h"

#import "MFView.h"
#import "MFMForms.h"

@implementation MFFileChooserImpl

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithObject: (mforms::FileChooser*) chooser type: (mforms::FileChooserType) type showHidden: (BOOL) showHidden
{
  self= [super init];
  if (self)
  {
    mOwner= chooser;
    mOwner->set_data(self);
    
    mOptionControls = [[NSMutableDictionary alloc] init];
    
    switch (type)
    {
      case mforms::OpenFile:
      {
        NSOpenPanel *panel= [NSOpenPanel openPanel];
        mPanel= panel;
        [panel setCanChooseDirectories:NO];
        [panel setCanChooseFiles:YES];
        [panel setExtensionHidden: NO];
        
        if (showHidden)
          [panel setShowsHiddenFiles: YES];
        break;
      }
      
      case mforms::SaveFile:
        mPanel= [NSSavePanel savePanel];
        [mPanel setCanCreateDirectories: YES];
        [mPanel setExtensionHidden: NO];
        mPanel.delegate = self;
        break;

      case mforms::OpenDirectory:
      {
        NSOpenPanel *panel= [NSOpenPanel openPanel];
        mPanel= panel;
        [panel setCanChooseDirectories:YES];
        [panel setCanChooseFiles:NO];
        [panel setExtensionHidden: NO];

        if (showHidden)
          [panel setShowsHiddenFiles: YES];
        break;
      }
    }
    [mPanel setAllowsOtherFileTypes:YES];
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSPopUpButton*)addSelectorOption: (NSArray*)values label: (NSString*)label
{
  if (!mOptionsView)
    mOptionsView = [[NSView alloc] initWithFrame: NSMakeRect(0, 0, 420, 30)];
  
  NSPopUpButton *pop = [[NSPopUpButton alloc] initWithFrame: NSMakeRect(0, 0, 250, 20)];
  for (NSUInteger c = values.count, i = 0; i < c; i+= 2)
  {
    [pop addItemWithTitle: values[i]];
    pop.lastItem.representedObject = values[i+1];
  }
  [pop sizeToFit];
  
  NSTextField *text = [[NSTextField alloc] initWithFrame: NSMakeRect(0, 0, 150, 20)];
  text.stringValue = label;
  [text setEditable: NO];
  [text setDrawsBackground: NO];
  [text setBordered: NO];
  text.alignment = NSTextAlignmentRight;
  [mOptionsView addSubview: text];
  text.autoresizingMask = NSMinXEdge|NSMaxYEdge;
  [mOptionsView addSubview: pop];
  pop.autoresizingMask = NSMinXEdge|NSMaxYEdge|NSViewWidthSizable|NSMaxXEdge;
  
  [mOptionsView setFrameSize: NSMakeSize(NSWidth(mOptionsView.frame), (mOptionControls.count+1) * 25 + 20)];
  float y = NSHeight(mOptionsView.frame) - 10;
  for (id view in mOptionsView.subviews)
  {
    if ([view isKindOfClass: [NSTextField class]])
    {
      y -= 25;
      [view setFrameOrigin: NSMakePoint(0, y)];
    }
    else
      [view setFrameOrigin: NSMakePoint(154, y)];
  }
  return pop;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)panel:(id)sender userEnteredFilename:(NSString *)filename confirmed:(BOOL)okFlag
{
  NSPopUpButton *fileTypeMenu = mOptionControls[@"format"];
  if (fileTypeMenu)
  {
    NSString *extension = fileTypeMenu.selectedItem.representedObject;
    if (![filename hasSuffix: extension])
      filename = [filename stringByAppendingPathExtension: extension];
  }
  return filename;  
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityGroupRole;
}

//----------------------------------------------------------------------------------------------------------------------

static bool filechooser_create(mforms::FileChooser *self, mforms::Form *owner, mforms::FileChooserType type, bool show_hidden)
{
  MFFileChooserImpl *chooser = [[MFFileChooserImpl alloc] initWithObject: self type: type showHidden: show_hidden];
  chooser->mParent = owner;
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

static void filechooser_set_title(mforms::FileChooser *self, const std::string &title)
{
  MFFileChooserImpl *chooser= self->get_data();
  if (chooser)
  {
    chooser->mPanel.title = wrap_nsstring(title);
  }
}

//----------------------------------------------------------------------------------------------------------------------

static bool filechooser_run_modal(mforms::FileChooser *self)
{
  MFFileChooserImpl *chooser= self->get_data();
  if (chooser)
  {
    NSString *filename = chooser->mDefaultFileName;
    
    if (chooser->mOptionsView)
      chooser->mPanel.accessoryView = chooser->mOptionsView;
    
    chooser->mPanel.message = @"";
    if (filename != nil)
      chooser->mPanel.nameFieldStringValue = filename;
    if ([chooser->mPanel runModal] == NSModalResponseOK)
      return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

static void filechooser_set_directory(mforms::FileChooser *self, const std::string &path)
{
  MFFileChooserImpl *chooser= self->get_data();
  if (chooser)
  {
    chooser->mPanel.directoryURL = [NSURL fileURLWithPath: wrap_nsstring(path) isDirectory: YES];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static std::string filechooser_get_directory(mforms::FileChooser *self)
{
  MFFileChooserImpl *chooser= self->get_data();
  if (chooser)
  {
    return ((chooser->mPanel.URL).URLByDeletingLastPathComponent.path).UTF8String;
  }
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

static std::string filechooser_get_path(mforms::FileChooser *self)
{
  MFFileChooserImpl *chooser= self->get_data();
  if (chooser)
    return (chooser->mPanel.URL.path).UTF8String;
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

static void filechooser_set_path(mforms::FileChooser *self, const std::string &path)
{
  MFFileChooserImpl *chooser= self->get_data();
  if (chooser)
  {
    NSString *fpath = wrap_nsstring(path);

    NSString *extension = fpath.pathExtension;
    NSString *directory = fpath.stringByDeletingLastPathComponent;
    if (directory)
      chooser->mPanel.directoryURL = [NSURL fileURLWithPath: directory isDirectory: YES];
    chooser->mDefaultFileName = fpath.lastPathComponent;
    
    NSPopUpButton *popup = chooser->mOptionControls[@"format"];
    if (popup)
    {
      NSInteger i = [popup indexOfItemWithRepresentedObject: extension];
      if (i >= 0)
        [popup selectItemAtIndex: i];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void filechooser_set_extensions(mforms::FileChooser *self, const std::string &extensions, const std::string &default_extension, bool allow_all_file_types = true)
{
  MFFileChooserImpl *chooser= self->get_data();
  if (chooser)
  {
    NSMutableArray *array= [NSMutableArray array];
    NSMutableArray *fileTypes= [NSMutableArray array];

    // extensions format:
    // AAA Files (*.aaa)|*.aaa|BBB Files (*.bbb)|*.bbb
    
    std::vector<std::pair<std::string, std::string> > extlist= self->split_extensions(extensions);
    for (std::vector<std::pair<std::string, std::string> >::const_iterator iter= extlist.begin();
      iter != extlist.end(); ++iter)
    {
      [fileTypes addObject: wrap_nsstring(iter->first)];
      if (iter->second.size() > 2 || iter->second.substr(0, 2) == "*.")
        [array addObject: @(iter->second.substr(2).c_str())];
      else if (iter->second[0] == '*')
        [array addObject: @(iter->second.substr(1).c_str())];
      [fileTypes addObject: array.lastObject];
    }

    if (array.count > 0)
      chooser->mPanel.allowedFileTypes = array;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void filechooser_add_selector_option(mforms::FileChooser *self, const std::string &name, const std::string &label, const std::vector<std::pair<std::string, std::string> > &options)
{
  MFFileChooserImpl *chooser= self->get_data();
  if (chooser)
  {
    NSMutableArray *optlist = [NSMutableArray arrayWithCapacity: options.size()*2];
    for (std::vector<std::pair<std::string, std::string> >::const_iterator iter = options.begin();
         iter != options.end(); ++iter)
    {
      [optlist addObject: wrap_nsstring(iter->first)];
      [optlist addObject: wrap_nsstring(iter->second)];
    }
    id popupButton = [chooser addSelectorOption: optlist label: wrap_nsstring(label)];
    if (popupButton)
      chooser->mOptionControls[wrap_nsstring(name)] = popupButton;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static std::string filechooser_get_selector_option_value(mforms::FileChooser *self, const std::string &name)
{
  MFFileChooserImpl *chooser= self->get_data();
  if (chooser)
  {
    if (name == "format")
      return [chooser->mOptionControls[wrap_nsstring(name)] titleOfSelectedItem].UTF8String ?: "";
    return [[chooser->mOptionControls[wrap_nsstring(name)] selectedItem].representedObject UTF8String] ?: "";
  }
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

void cf_filechooser_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_filechooser_impl.create= &filechooser_create;
  f->_filechooser_impl.set_title= &filechooser_set_title;
  f->_filechooser_impl.set_directory= &filechooser_set_directory;
  f->_filechooser_impl.get_directory= &filechooser_get_directory;
  f->_filechooser_impl.get_path= &filechooser_get_path;
  f->_filechooser_impl.set_path= &filechooser_set_path;
  f->_filechooser_impl.run_modal= &filechooser_run_modal; 
  f->_filechooser_impl.set_extensions= &filechooser_set_extensions;
  f->_filechooser_impl.add_selector_option = &filechooser_add_selector_option;
  f->_filechooser_impl.get_selector_option_value = &filechooser_get_selector_option_value;
}


@end
