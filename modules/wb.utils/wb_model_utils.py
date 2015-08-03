# Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; version 2 of the
# License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301  USA

import os

import grt
import mforms
from wb import wbinputs
from wb_utils_grt import ModuleInfo


# this is just a function used by the plugin, it's not exported
def printTableLine(fields, filler= " "):
    print "|",
    for text, size in fields:
        print text.ljust(size, filler), "|",
    print


# @wbexport makes this function be exported by the module and also describes the return and
# argument types of the function
# @wbplugin defines the name of the plugin to "wb.catalog.util.dumpColumns", sets the caption to be
# shown in places like the menu, where to take input arguments from and also that it should be included
# in the Catalog submenu in Plugins.
@ModuleInfo.plugin("wb.catalog.util.dumpColumns", caption= "Dump All Table Columns", input= [wbinputs.currentCatalog()], pluginMenu= "Catalog")
@ModuleInfo.export(grt.INT, grt.classes.db_Catalog)
def printAllColumns(catalog):
    lines= []
    schemalen= 0
    tablelen= 0
    columnlen= 0
    typelen= 0

    for schema in catalog.schemata:
        schemalen= max(schemalen, len(schema.name))
        for table in schema.tables:
            tablelen= max(tablelen, len(table.name))
            for column in table.columns:
                columnlen= max(columnlen, len(column.name))
                typelen= max(typelen, len(column.formattedType))
                lines.append((schema.name, table.name, column.name, column.formattedType))

    printTableLine([("-", schemalen), ("-", tablelen), ("-", columnlen), ("-", typelen)], "-")
    printTableLine([("Schema", schemalen), ("Table", tablelen), ("Column", columnlen), ("Type", typelen)])

    printTableLine([("-", schemalen), ("-", tablelen), ("-", columnlen), ("-", typelen)], "-")

    for s,t,c,dt in lines:
        printTableLine([(s, schemalen), (t, tablelen), (c, columnlen), (dt, typelen)])

    printTableLine([("-", schemalen), ("-", tablelen), ("-", columnlen), ("-", typelen)], "-")
    print len(lines), "columns printed"
    
    return 0



@ModuleInfo.plugin("wb.model.print_diagram_pdf", caption= "Export Diagram to PDF...", input= [wbinputs.selectedDiagram()], pluginMenu="Overview")
@ModuleInfo.export(grt.INT, grt.classes.model_Diagram)
def printDiagramToPDF(diagram):
    fc = mforms.FileChooser(mforms.Form.main_form(), mforms.SaveFile)
    fc.set_title("Export to PDF...")
    fc.set_extensions("PDF files (*.pdf)|*.pdf", ".pdf")
    if fc.run_modal():
        path = fc.get_path()
        mforms.App.get().set_status_text("Exporting diagram %s to PDF file..." % diagram.name)
        grt.modules.WbPrinting.printToPDFFile(diagram, path)
        mforms.App.get().set_status_text("Diagram %s exported to %s" % (diagram.name, path))
    return 0



class PrintToFileDialog(mforms.Form):
    def __init__(self, model):
        mforms.Form.__init__(self, mforms.Form.main_form(), mforms.FormNormal)
        self.set_title("Print Model to File")

        self.model = model

        box = mforms.newBox(False)
        box.set_spacing(8)
        box.set_padding(20)

        self.tree = mforms.newTreeView(mforms.TreeFlatList)
        self.tree.add_column(mforms.CheckColumnType, "Print", 50, True)
        self.tree.add_column(mforms.StringColumnType, "Diagram", 300, False)
        self.tree.end_columns()
        box.add(self.tree, True, True)

        for d in model.diagrams:
            node = self.tree.add_node()
            node.set_bool(0, True)
            node.set_string(1, d.name)

        help_text = """The following variables will be substituted:
$document, $doc_version, $doc_author, $doc_project, $doc_date_changed, $doc_date_created, $diagram, $timestamp, $page, $total_pages, $doc_page, $doc_total_pages
            """
        hbox = mforms.newBox(True)
        hbox.set_spacing(8)
        label = mforms.newLabel("Header Text:", True)
        label.set_size(100, -1)
        hbox.add(label, False, True)
        self.header = mforms.newTextEntry()
        self.header.set_tooltip(help_text)
        hbox.add(self.header, True, True)
        box.add(hbox, False, True)
        self.header.set_value(self.model.options.get("wb.PrintModel:HeaderText", "$timestamp, $document - $diagram (part $page of $total_pages)"))

        hbox = mforms.newBox(True)
        hbox.set_spacing(8)
        label = mforms.newLabel("Footer Text:", True)
        label.set_size(100, -1)
        hbox.add(label, False, True)
        self.footer = mforms.newTextEntry()
        self.footer.set_tooltip(help_text)
        hbox.add(self.footer, True, True)
        box.add(hbox, False, True)
        self.footer.set_value(self.model.options.get("wb.PrintModel:FooterText", "$doc_page of $doc_total_pages"))

        hbox = mforms.newBox(True)
        hbox.set_spacing(8)
        label = mforms.newLabel("File:", True)
        label.set_size(100, -1)
        hbox.add(label, False, True)
        self.file = mforms.newTextEntry()
        hbox.add(self.file, True, True)
        self.browse = mforms.newButton()
        self.browse.set_text("...")
        self.browse.add_clicked_callback(self.do_browse)
        self.browse.enable_internal_padding(False)
        hbox.add(self.browse, False, True)
        box.add(hbox, False, True)

        self.file.set_value(self.model.options.get("wb.PrintModel:Path", os.path.join(mforms.Utilities.get_special_folder(mforms.Desktop), "model.pdf")))

        hbox = mforms.newBox(True)
        hbox.set_spacing(8)
        label = mforms.newLabel("Format:", True)
        label.set_size(100, -1)
        hbox.add(label, False, True)
        self.format = mforms.newSelector()
        self.format.add_item("PDF")
        self.format.add_item("PostScript File")
        self.format.add_changed_callback(self.format_changed)
        hbox.add(self.format, True, True)
        box.add(hbox, False, True)

        hbox = mforms.newBox(True)
        hbox.set_spacing(8)
        self.ok = mforms.newButton()
        self.ok.set_text("OK")
        self.cancel = mforms.newButton()
        self.cancel.set_text("Cancel")
        mforms.Utilities.add_end_ok_cancel_buttons(hbox, self.ok, self.cancel)
        box.add_end(hbox, False, True)

        self.set_content(box)
        self.set_size(500, 400)


    def do_browse(self):
        fc = mforms.FileChooser(mforms.Form.main_form(), mforms.SaveFile)
        fc.set_path(self.file.get_string_value())
        fc.set_title("Print to File...")
        fc.set_extensions("PDF files (*.pdf)|*.pdf|PostScript Files (*.ps)|*.ps", ".pdf")
        if fc.run_modal():
            self.file.set_value(fc.get_path())


    def format_changed(self):
        path = self.file.get_string_value()
        format = [".pdf", ".ps"][self.format.get_selected_index()]
        if not path.endswith(format):
            path = os.path.splitext(path)[0]+format
            self.file.set_value(path)


    def run(self):
        if self.run_modal(self.ok, self.cancel):
            l = grt.List("object", "model.Diagram")
            for i, d in enumerate(self.model.diagrams):
                if self.tree.node_at_row(i).get_bool(0):
                    l.append(d)

            if not l:
                mforms.App.get().set_status_text("No diagrams selected to print")
                return

            file = self.file.get_string_value()
            format = [".pdf", ".ps"][self.format.get_selected_index()]
            header = self.header.get_string_value()
            footer = self.footer.get_string_value()

            self.model.options["wb.PrintModel:Path"] = file
            self.model.options["wb.PrintModel:HeaderText"] = header
            self.model.options["wb.PrintModel:FooterText"] = footer


            if not file:
                mforms.App.get().set_status_text("Invalid path")

            if not file.endswith(format):
                file = file + format

            mforms.App.get().set_status_text("Exporting diagrams to %s..." % file)

            def replace_variables(text, model):
                text = text.replace("$document", model.owner.info.caption)
                text = text.replace("$doc_version", model.owner.info.version)
                text = text.replace("$doc_author", model.owner.info.author)
                text = text.replace("$doc_project", model.owner.info.project)
                text = text.replace("$doc_date_changed", model.owner.info.dateChanged)
                text = text.replace("$doc_date_created", model.owner.info.dateCreated)
                import time
                text = text.replace("$timestamp", time.ctime())
                return text

            options = {
                "header_text" : replace_variables(header, self.model),
                "footer_text" : replace_variables(footer, self.model),
            }
            grt.modules.WbPrinting.printDiagramsToFile(l, file, format[1:], options)

            mforms.App.get().set_status_text("Exported %i diagrams to %s." % (len(l), file))



@ModuleInfo.plugin("wb.model.print_model", caption= "Print Model to File", input= [wbinputs.currentModel()])
@ModuleInfo.export(grt.INT, grt.classes.model_Model)
def printModel(model):
    dlg = PrintToFileDialog(model)
    dlg.run()
    return 0


# Quick impl for Rename Diagram... item in modeling overview
@ModuleInfo.plugin("wb.model.rename_diagram", caption= "Rename Diagram...", input= [wbinputs.selectedDiagram()], pluginMenu="Overview")
@ModuleInfo.export(grt.INT, grt.classes.model_Diagram)
def renameDiagram(diagram):
    ret, name = mforms.Utilities.request_input("Rename Diagram", "Enter new name for the diagram", diagram.name)
    if ret:
        grt.modules.Workbench.startTrackingUndo()
        diagram.name = name
        grt.modules.Workbench.finishTrackingUndo("Rename diagram to %s" % name)
    return 0
