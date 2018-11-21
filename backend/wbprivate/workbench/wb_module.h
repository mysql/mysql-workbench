/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WB_MODULE_H_
#define _WB_MODULE_H_

#include "grtpp_module_cpp.h"

#include "wb_context.h"
#include "grts/structs.db.mgmt.h"
#include "interfaces/plugin.h"

#ifdef _MSC_VER
#include "wmi.h"
#endif

#define WBModule_VERSION "5.2.27"

namespace wb {

  class WorkbenchImpl : public grt::ModuleImplBase, public PluginInterfaceImpl {
    typedef grt::ModuleImplBase super;

  public:
    WorkbenchImpl(grt::CPPModuleLoader *);
    virtual ~WorkbenchImpl();

    void set_context(WBContext *wb);
    std::string getSystemInfo(bool indent);
    std::map<std::string, std::string> getSystemInfoMap();
    int isOsSupported(const std::string &os);

    DEFINE_INIT_MODULE(
      WBModule_VERSION, "Oracle and/or its affiliates", grt::ModuleImplBase,
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::getPluginInfo),

      // Non-plugin functions
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::copyToClipboard),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::hasUnsavedChanges),

      // Model
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::newDocument),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::newDocumentFromDB),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::openModel),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::openRecentModel),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::saveModel),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::saveModelAs),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::exit),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::exportPNG),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::exportPDF),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::exportPS),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::exportSVG),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::activateDiagram),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::exportDiagramToPng),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::selectAll),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::selectSimilar),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::selectConnected),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::goToNextSelected),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::goToPreviousSelected),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::highlightFigure),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::editSelectedFigure),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::editSelectedFigureInNewWindow),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::editObject),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::editObjectInNewWindow),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::raiseSelection),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::lowerSelection),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::newDiagram),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::toggleGrid),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::togglePageGrid),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::toggleGridAlign),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::toggleFKHighlight),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::zoomIn),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::zoomOut),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::zoomDefault),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::setFigureNotation),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::setRelationshipNotation),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::setMarker),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::goToMarker),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::startTrackingUndo),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::finishTrackingUndo),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::cancelTrackingUndo),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::isOsSupported),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::addUndoListAdd),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::addUndoListRemove),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::addUndoObjectChange),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::addUndoDictSet),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::beginUndoGroup),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::endUndoGroup),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::setUndoDescription),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::createAttachedFile),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::setAttachedFileContents),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::getAttachedFileContents),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::getAttachedFileTmpPath),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::exportAttachedFileContents),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::openModelFile),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::closeModelFile),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::getDbFilePath),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::getTempDir),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::debugValidateGRT),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::getVideoAdapter),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::runScriptFile),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::installModuleFile),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::showUserTypeEditor),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::showDocumentProperties),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::showModelOptions),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::showOptions),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::showConnectionManager),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::showInstanceManagerFor),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::showInstanceManager),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::showQueryConnectDialog),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::saveConnections),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::saveInstances),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::refreshHomeConnections),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::showGRTShell),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::newGRTFile),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::openGRTFile),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::showPluginManager),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::reportBug),

      // Utilities
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::confirm),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::requestFileOpen),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::requestFileSave),

      DECLARE_MODULE_FUNCTION(WorkbenchImpl::createConnectionsFromLocalServers),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::createInstancesFromLocalServers),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::create_connection),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::initializeOtherRDBMS),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::deleteConnection),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::deleteConnectionGroup),
      DECLARE_MODULE_FUNCTION(WorkbenchImpl::createSSHSession));

  protected:
    virtual void initialization_done() override {
// Called after init_module (defined by DEFINE_INIT_MODULE above) is done.
// Here we register platform dependent functions.
#ifdef _MSC_VER
      register_functions(
        DECLARE_MODULE_FUNCTION(WorkbenchImpl::wmiOpenSession), DECLARE_MODULE_FUNCTION(WorkbenchImpl::wmiCloseSession),
        DECLARE_MODULE_FUNCTION(WorkbenchImpl::wmiQuery), DECLARE_MODULE_FUNCTION(WorkbenchImpl::wmiServiceControl),
        DECLARE_MODULE_FUNCTION(WorkbenchImpl::wmiSystemStat),
        DECLARE_MODULE_FUNCTION(WorkbenchImpl::wmiStartMonitoring),
        DECLARE_MODULE_FUNCTION(WorkbenchImpl::wmiReadValue), DECLARE_MODULE_FUNCTION(WorkbenchImpl::wmiStopMonitoring),
        NULL);
#endif
    };

  private:
    WBContext *_wb;
#ifdef _MSC_VER
    std::map<int, wmi::WmiServices *> _wmi_sessions;
#ifdef DEBUG
    std::map<int, GThread *> _thread_for_wmi_session;
#endif
    int _last_wmi_session_id;

    std::map<int, wmi::WmiMonitor *> _wmi_monitors;
    int _last_wmi_monitor_id;
#endif

    virtual grt::ListRef<app_Plugin> getPluginInfo() override;

    int copyToClipboard(const std::string &str);

    int hasUnsavedChanges();

    // file
    int newDocument();
    int newDocumentFromDB();
    int openModel(const std::string &path);
    int openRecentModel(const std::string &index);
    int saveModel();
    int saveModelAs(const std::string &path);
    int exportPNG(const std::string &filename);
    int exportPDF(const std::string &filename);
    int exportPS(const std::string &filename);
    int exportSVG(const std::string &filename);
    int activateDiagram(const model_DiagramRef &diagram);
    int exportDiagramToPng(const model_DiagramRef &diagram, const std::string &filename);
    int exit();

    // edit
    int selectAll();
    int selectSimilar();
    int selectConnected();

    int editSelectedFigure(const model_DiagramRef &view);
    int editSelectedFigureInNewWindow(const model_DiagramRef &view);

    int editObject(const GrtObjectRef &object);
    int editObjectInNewWindow(const GrtObjectRef &object);

    // canvas manipulation
    int raiseSelection(const model_DiagramRef &view);
    int lowerSelection(const model_DiagramRef &view);

    // view
    int newDiagram(const model_ModelRef &model);

    int toggleGrid(const model_DiagramRef &view);
    int togglePageGrid(const model_DiagramRef &view);
    int toggleGridAlign(const model_DiagramRef &view);
    int toggleFKHighlight(const model_DiagramRef &view);

    int zoomIn();
    int zoomOut();
    int zoomDefault();

    int goToNextSelected();
    int goToPreviousSelected();

    int setMarker(const std::string &marker);
    int goToMarker(const std::string &marker);

    int setFigureNotation(const std::string &name, workbench_physical_ModelRef model);
    int setRelationshipNotation(const std::string &name, workbench_physical_ModelRef model);

    int highlightFigure(const model_ObjectRef &figure);
    // undo
    int startTrackingUndo();
    int finishTrackingUndo(const std::string &description);
    int cancelTrackingUndo();

    int addUndoListAdd(const grt::BaseListRef &list);
    int addUndoListRemove(const grt::BaseListRef &list, int index);
    int addUndoObjectChange(const grt::ObjectRef &object, const std::string &member);
    int addUndoDictSet(const grt::DictRef &dict, const std::string &key);
    int beginUndoGroup();
    int endUndoGroup();
    int setUndoDescription(const std::string &text);

    // attached file management
    std::string createAttachedFile(const std::string &group, const std::string &tmpl);
    int setAttachedFileContents(const std::string &filename, const std::string &text);
    std::string getAttachedFileContents(const std::string &filename);
    std::string getAttachedFileTmpPath(const std::string &filename);
    int exportAttachedFileContents(const std::string &filename, const std::string &export_to);
    workbench_DocumentRef openModelFile(const std::string &path);
    int closeModelFile();
    std::string getDbFilePath();
    std::string getTempDir();

    int runScriptFile(const std::string &filename);
    int installModuleFile(const std::string &filename);

    // debugging
    int debugValidateGRT();

    int showUserTypeEditor(const workbench_physical_ModelRef &model);
    int showDocumentProperties();
    int showModelOptions(const workbench_physical_ModelRef &model);
    int showOptions();
    int showConnectionManager();
    int showInstanceManagerFor(const db_mgmt_ConnectionRef &conn);
    int showInstanceManager();
    int showQueryConnectDialog();
    int saveConnections();
    int saveInstances();
    int showGRTShell();
    int showPluginManager();
    int newGRTFile();
    int openGRTFile();
    int reportBug(const std::string error_info = "");

    // UI
    int refreshHomeConnections();
    int confirm(const std::string &title, const std::string &caption);

    std::string requestFileOpen(const std::string &caption, const std::string &extensions);
    std::string requestFileSave(const std::string &caption, const std::string &extensions);
    bool _is_other_dbms_initialized;

#ifdef _MSC_VER
    int wmiOpenSession(const std::string server, const std::string &user, const std::string &password);
    int wmiCloseSession(int session);
    grt::DictListRef wmiQuery(int session, const std::string &query);
    std::string wmiServiceControl(int session, const std::string &service, const std::string &action);
    std::string wmiSystemStat(int session, const std::string &what);

    int wmiStartMonitoring(int session, const std::string &what);
    std::string wmiReadValue(int monitor);
    int wmiStopMonitoring(int monitor);

#endif

    db_mgmt_ConnectionRef create_connection(const std::string &host, const std::string &user,
                                            const std::string socket_or_pipe_name, int can_use_networking,
                                            int can_use_socket_or_pipe, int port, const std::string &name);
    grt::DictListRef getLocalServerList();
    int createConnectionsFromLocalServers();
    int createInstancesFromLocalServers();

    std::string getVideoAdapter();
    std::string getFullVideoAdapterInfo(bool indent);
    int initializeOtherRDBMS();
    db_mgmt_SSHConnectionRef createSSHSession(const grt::ObjectRef &val);
    int deleteConnection(const db_mgmt_ConnectionRef &connection);
    int deleteConnectionGroup(const std::string &group);
  };
};

#endif
