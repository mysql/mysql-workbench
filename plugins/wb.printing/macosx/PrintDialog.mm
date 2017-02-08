/* 
 * Copyright (c) 2009, 2016, Oracle and/or its affiliates. All rights reserved.
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

#import "PrintDialog.h"
#include "grts/structs.model.h"
#include "wb_printing.h"
#include "mdc_canvas_view_printing.h"
#include "wbcanvas/model_diagram_impl.h"
#include <cairo/cairo-quartz.h>

@interface CairoPrintView : NSView
{
  mdc::CanvasViewExtras *_printer;
  NSSize _paperSize;
  NSRange _pageRange;
  int _xpages, _ypages;
}
@end

@implementation CairoPrintView

- (instancetype)initWithFrame:(NSRect)frame
            diagram:(model_DiagramRef&)diagram
       pageSettings:(app_PageSettingsRef&)pageSettings
          printInfo:(NSPrintInfo*)printInfo
{
  self= [super initWithFrame: frame];
  if (self)
  {
    app_PaperTypeRef paperType= pageSettings->paperType();
    // page settings in cairo coordinates
    float paperWidth= (*paperType->width() * *pageSettings->scale());
    float paperHeight= (*paperType->height() * *pageSettings->scale());
    float marginLeft= (*pageSettings->marginLeft() * *pageSettings->scale());
    float marginRight= (*pageSettings->marginRight() * *pageSettings->scale());
    float marginTop= (*pageSettings->marginTop() * *pageSettings->scale());
    float marginBottom= (*pageSettings->marginBottom() * *pageSettings->scale());
    
    // size of the printable area in system coords
    _paperSize= [printInfo paperSize];
    
    if (pageSettings->orientation() == "landscape")
    {
      std::swap(paperWidth, paperHeight);
      std::swap(marginLeft, marginTop);
      std::swap(marginRight, marginBottom);
    }
    
    // size of the printable area in cairo coords
    base::Size pageSize;
    pageSize.width= paperWidth - marginLeft - marginRight;
    pageSize.height= paperHeight - marginTop - marginBottom;
    
    // scaling to transform from cairo coordinates to Cocoa coordinates
    float xscale= [printInfo paperSize].width / paperWidth;
    float yscale= [printInfo paperSize].height / paperHeight;
    float scale= (xscale + yscale) / 2;
    
    _printer= new mdc::CanvasViewExtras(diagram->get_data()->get_canvas_view());
    
    // margins are already added by the system
    _printer->set_page_margins(marginTop, marginLeft, marginBottom, marginRight);
    _printer->set_paper_size(paperWidth, paperHeight);
//    _printer->set_orientation(pageSettings->orientation()=="landscape"?mdc::Landscape:mdc::Portrait);
    _printer->set_scale(scale);
    _printer->set_print_border(false);

    _pageRange.location= 1;
    _pageRange.length= wbprint::getPageCount(diagram);
    
    wbprint::getPageLayout(diagram, _xpages, _ypages);
  }
  return self;
}


- (void) dealloc
{
  
  delete _printer;
}


- (BOOL)knowsPageRange:(NSRangePointer)range 
{
  *range= _pageRange;
  
  return YES;
}


- (NSRect)rectForPage:(NSInteger)page
{
  return NSMakeRect(0, 0, _paperSize.width, _paperSize.height);
}


- (void)drawRect:(NSRect)rect
{
  CGContextRef cgContext= (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
  
  CGContextTranslateCTM(cgContext, 0.0, _paperSize.height);
  CGContextScaleCTM(cgContext, 1.0, -1.0);
  
  cairo_surface_t *surface =
      cairo_quartz_surface_create_for_cg_context(cgContext,
                                                 _paperSize.width,
                                                 _paperSize.height);

  mdc::CairoCtx cairoctx(surface);
      
  NSInteger page = [[NSPrintOperation currentOperation] currentPage] - 1;
    
  _printer->render_page(&cairoctx, page % _xpages, (int)page / _xpages);
  
  cairoctx.show_page();
  
  cairo_surface_destroy(surface);
}

@end

@interface PrintDialog()
{
}

@end

@implementation PrintDialog

- (instancetype)initWithModule: (grt::Module*)module
                     arguments: (const grt::BaseListRef &)args
{
  self = [super initWithModule: module arguments: args];
  if (self != nil)
  {
    model_DiagramRef diagram(model_DiagramRef::cast_from(args[0]));
    
    NSRect rect = NSMakeRect(0, 0, diagram->width(), diagram->height());
        
    int xpages, ypages;
    
    wbprint::getPageLayout(diagram, xpages, ypages);
    
    app_PageSettingsRef pageSettings(app_PageSettingsRef::cast_from(grt::GRT::get()->get("/wb/doc/pageSettings")));
    app_PaperTypeRef paperType(pageSettings->paperType());
        
    printInfo= [[NSPrintInfo alloc] initWithDictionary:
                @{NSPrintPaperName: @(paperType->name().c_str())}];

    if (pageSettings->orientation() == "landscape")
      [printInfo setOrientation: NSPaperOrientationLandscape];
    else
      [printInfo setOrientation: NSPaperOrientationPortrait];
    
    printView= [[CairoPrintView alloc] initWithFrame: rect 
                                             diagram: diagram
                                        pageSettings: pageSettings
                                           printInfo: printInfo];
  }
  return self;
}

- (void) dealloc
{
  bec::GRTManager::get()->get_plugin_manager()->forget_gui_plugin_handle((__bridge void *)self);

}

- (void)showModal
{
  NSPrintOperation *op = [NSPrintOperation printOperationWithView: printView];
  [op setPrintInfo: printInfo];
  [op runOperation];
}

@end


