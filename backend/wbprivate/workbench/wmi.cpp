/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <iostream>
#include <sstream>

#include "base/string_utilities.h"
#include "base/threading.h"
#include "base/log.h"

#include "wmi.h"

#pragma comment(lib, "comsuppw.lib")

using namespace wmi;
using namespace grt;

DEFAULT_LOG_DOMAIN("wmi")

//--------------------------------------------------------------------------------------------------

/**
 * Converts the given variant to an UTF-8 encoded standard string. The variant content is converted to
 * a string if it is not already one.
 */
std::string variant2string(VARIANT& value) {
  if (value.vt == VT_NULL)
    return "NULL";

  VariantChangeType(&value, &value, VARIANT_ALPHABOOL, VT_BSTR);
  CW2A valueString(V_BSTR(&value), CP_UTF8);
  return (LPSTR)valueString;
}

//--------------------------------------------------------------------------------------------------

/**
 * Converts the given string (which must be UTF-8 encoded) to a BSTR, encapsulated by CComBSTR
 * which frees us from taking care to deallocate the result.
 */
CComBSTR string2Bstr(const std::string& value) {
  CComBSTR result = CA2W(value.c_str(), CP_UTF8);
  return result;
}

//--------------------------------------------------------------------------------------------------

#ifdef _DEBUG

void dumpObject(IWbemClassObject* object) {
  if (object != NULL) {
    BSTR objectText;
    object->GetObjectText(0, &objectText);
    std::wcout << objectText << std::endl;
    SysFreeString(objectText);
  } else
    std::wcout << L"Object is NULL" << std::endl;
}

//--------------------------------------------------------------------------------------------------

#endif

std::string wmiResultToString(HRESULT wmiResult) {
  std::string result;
  IWbemStatusCodeText* status = NULL;

  HRESULT hres =
    CoCreateInstance(CLSID_WbemStatusCodeText, 0, CLSCTX_INPROC_SERVER, IID_IWbemStatusCodeText, (LPVOID*)&status);

  if (SUCCEEDED(hres)) {
    CComBSTR errorString;
    hres = status->GetErrorCodeText(wmiResult, 0, 0, &errorString);

    if (FAILED(hres)) {
      logError("Converting result code to string failed with error: %d\n", hres);
      result = "Internal error: WMI error description retrieval failed.";
    }

    CW2A converted_text(errorString, CP_UTF8);
    result = converted_text;
    status->Release();
  } else {
    logError("Could not instatiate a status code text converter. Error code: %d\n", hres);
    result = "Internal error: WMI status code text creation failed.";
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

std::string serviceResultToString(unsigned int result) {
  // Error codes according to http://msdn.microsoft.com/en-us/library/aa393660%28VS.85%29.aspx.
  static std::string code2String[] = {"Success",
                                      "Not Supported",
                                      "Access Denied",
                                      "Dependent Services Running",
                                      "Invalid Service Control",
                                      "Service Cannot Accept Control",
                                      "Service Not Active",
                                      "Service Request Timeout",
                                      "Unknown Failure",
                                      "Path Not Found",
                                      "Service Already Running",
                                      "Service Database Locked",
                                      "Service Dependency Deleted",
                                      "Service Dependency Failure",
                                      "Service Disabled",
                                      "Service Logon Failure",
                                      "Service Marked For Deletion",
                                      "Service No Thread",
                                      "Status Circular Dependency",
                                      "Status Duplicate Name",
                                      "Status Invalid Name",
                                      "Status Invalid Parameter",
                                      "Status Invalid Service Account",
                                      "Status Service Exists",
                                      "Service Already Paused"};

  if (result < 25)
    return code2String[result];

  return "Unknown error";
}

//----------------- WmiMonitor ---------------------------------------------------------------------

WmiMonitor::WmiMonitor(IWbemServices* services, const std::string& parameter)
  : _services(services), _propertyHandle(0), _namePropertyHandle(0), _findTotal(false) {
  logDebug("Creating new wmi monitor for parameter: %s\n", parameter.c_str());

  const std::vector<std::string> args = base::split(parameter, ".");

  if (args.size() < 2) {
    logError("Invalid parameter format - cannot continue\n");
    throw std::runtime_error(_("Wrong monitor format. Got '") + parameter + _("', expected '<WMIClass.Name>'"));
  }

  CComBSTR instancePath;

  HRESULT hr =
    CoCreateInstance(CLSID_WbemRefresher, NULL, CLSCTX_INPROC_SERVER, IID_IWbemRefresher, (void**)&_refresher);
  if (FAILED(hr)) {
    logError("Could not create a wbem refresher instance. Error: %d\n", hr);
    throw std::runtime_error(_("WMI - Could not create monitor object.\n\n") + wmiResultToString(hr));
  }

  CComPtr<IWbemConfigureRefresher> config;
  hr = _refresher->QueryInterface(IID_IWbemConfigureRefresher, (void**)&config);
  if (FAILED(hr)) {
    logError("QueryInterface for wbem configure refresher failed with error: %d\n", hr);

    // _refersher is a smart pointer and is automatically freed.
    throw std::runtime_error(_("WMI - Could not create monitor object.\n\n") + wmiResultToString(hr));
  }

  CComBSTR path = args[0].c_str();
  _propertyName = args[1].c_str();

  if (path.Length() == 0)
    throw std::runtime_error(_("WMI - Invalid monitoring parameter specified."));

  // Add an enumerator to the refresher. This is what actually gets the value to monitor.
  hr = config->AddEnum(_services, path, 0, NULL, &_enumerator, &_enumeratorId);
  if (FAILED(hr)) {
    std::string result = wmiResultToString(hr);
    logError("Could not register monitoring enumerator. Error: %s\n", result.c_str());
    throw std::runtime_error(_("WMI - Could not register monitoring enumerator.\n"));
  }
}

//--------------------------------------------------------------------------------------------------

WmiMonitor::~WmiMonitor() {
  logDebug("Destroying monitor\n");

  CComPtr<IWbemConfigureRefresher> config;
  HRESULT hr = _refresher->QueryInterface(IID_IWbemConfigureRefresher, (void**)&config);
  if (SUCCEEDED(hr))
    config->Remove(_enumeratorId, WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT);
  else {
    std::string result = wmiResultToString(hr);
    logError("Could not remove enumerator from wbem config refresher. Error: %s\n", result.c_str());
  }
}

//--------------------------------------------------------------------------------------------------

std::string WmiMonitor::readValue() {
  logDebug("Reading next monitoring value\n");

  // Refresh the enumerator so we actually get values.
  _refresher->Refresh(0L);

  // Read the results.
  ULONG returnCount = 0;
  IWbemObjectAccess** accessors = NULL; // List of accessors.

  // Determine required space. We have to provide it to the getter.
  HRESULT hr = _enumerator->GetObjects(0L, 0, accessors, &returnCount);
  if (hr == WBEM_E_BUFFER_TOO_SMALL && returnCount > 0) {
    accessors = new IWbemObjectAccess*[returnCount];
    SecureZeroMemory(accessors, returnCount * sizeof(IWbemObjectAccess*));

    hr = _enumerator->GetObjects(0L, returnCount, accessors, &returnCount);
    if (FAILED(hr)) {
      std::string result = wmiResultToString(hr);
      logError("Cannot get value object from enumerator. Error: %s\n", result.c_str());

      delete[] accessors;
      return "0";
    }
  }

  std::string result = "0";

  // First time we access the property we have to get a handle for it, which
  // can be reused for any subsequent query.
  CIMTYPE propertyType;
  if (_propertyHandle == 0)
    hr = accessors[0]->GetPropertyHandle(_propertyName, &propertyType, &_propertyHandle);

  if (FAILED(hr)) {
    std::string result = wmiResultToString(hr);
    logError("Cannot get property handle from wbem accessor. Error: %s\n", result.c_str());
  }

  CIMTYPE namePropType;
  if (_findTotal && _namePropertyHandle == 0)
    hr = accessors[0]->GetPropertyHandle(L"Name", &namePropType, &_namePropertyHandle);

  if (FAILED(hr)) {
    std::string result = wmiResultToString(hr);
    logError("Cannot get name property handle from wbem accessor. Error: %s\n", result.c_str());
  }

  if (_propertyHandle != 0) {
    DWORD value = 0;
    if (_namePropertyHandle != 0) {
      // For processor time queries a value is returned for every processor and a total entry
      // that comprises all processors.
      for (ULONG i = 0; i < returnCount; i++) {
        long byteCount = 0;
        byte buffer[20];
        hr = accessors[i]->ReadPropertyValue(_namePropertyHandle, 20, &byteCount, buffer);

        if (StrCmpW((LPCWSTR)buffer, L"_Total") == 0) {
          hr = accessors[i]->ReadDWORD(_propertyHandle, &value);
          if (FAILED(hr)) {
            std::string result = wmiResultToString(hr);
            logError("Cannot read DWORD value from wbem accessor. Error: %s\n", result.c_str());
          }

          break;
        }
      }
    } else {
      // No need to search for a specific value. Simply return the first (and mostly only) property value
      // we got.
      hr = accessors[0]->ReadDWORD(_propertyHandle, &value);

      if (FAILED(hr)) {
        std::string result = wmiResultToString(hr);
        logError("Cannot read DWORD value from wbem accessor. Error: %s\n", result.c_str());
      }
    }

    std::stringstream ss;
    ss << value;
    result = ss.str();
  }

  for (ULONG i = 0; i < returnCount; i++)
    accessors[i]->Release();
  delete[] accessors;

  return result;
}

//----------------- WmiServices --------------------------------------------------------------------

static base::Mutex _locator_mutex;
static IWbemLocator* _locator = NULL;
static int _locator_refcount = 0;

WmiServices::WmiServices(const std::string& server, const std::string& user, const std::string& password) {
  if (server.empty())
    logDebug("Creating WmiServices for local server (user: %s)\n", user.c_str());
  else
    logDebug("Creating WmiServices for remote server: %s (user: %s)\n", server.c_str(), user.c_str());

  allocate_locator();

  // Connect to the given target node using the provided credentials.
  HRESULT hr;

  // If node is empty then we are connecting to the local box. In this case don't use the given credentials.
  if (server.size() > 0) {
    std::string unc = "\\\\" + server + "\\root\\cimv2";

    CComBSTR nodeString = string2Bstr(unc);
    CComBSTR userString = string2Bstr(user);
    CComBSTR passwordString = string2Bstr(password);
    hr = _locator->ConnectServer(nodeString, userString, passwordString, NULL, WBEM_FLAG_CONNECT_USE_MAX_WAIT, NULL,
                                 NULL, &_services);
  } else
    hr =
      _locator->ConnectServer(L"root\\cimv2", NULL, NULL, NULL, WBEM_FLAG_CONNECT_USE_MAX_WAIT, NULL, NULL, &_services);

  if (SUCCEEDED(hr)) {
    /*
    // Set security levels on a WMI connection
    COAUTHIDENTITY cID;

    cID.User           = (USHORT*) CA2W(user.c_str(), CP_UTF8).m_psz;
    cID.UserLength     = user.size();
    cID.Password       = (USHORT*) CA2W(password.c_str(), CP_UTF8).m_psz;
    cID.PasswordLength = password.size();
    cID.Domain         = NULL;
    cID.DomainLength   = 0;
    cID.Flags          = SEC_WINNT_AUTH_IDENTITY_UNICODE;
*/
    // Set a blanket to our service proxy to establish a security context.
    // Values as recommended (http://msdn.microsoft.com/en-us/library/windows/desktop/aa393620%28v=vs.85%29.aspx).
    hr = CoSetProxyBlanket(_services,                   // Indicates the proxy to set
                           RPC_C_AUTHN_DEFAULT,         // Authentication service
                           RPC_C_AUTHZ_DEFAULT,         // Authorization service
                           COLE_DEFAULT_PRINCIPAL,      // Server principal name
                           RPC_C_AUTHN_LEVEL_DEFAULT,   // Authentication level
                           RPC_C_IMP_LEVEL_IMPERSONATE, // Impersonation level
                           COLE_DEFAULT_AUTHINFO,       // client identity
                           EOAC_DEFAULT                 // proxy capabilities
                           );

    if (FAILED(hr)) {
      std::string result = wmiResultToString(hr);
      logError("Could not set proxy blanket for our wmi services. Error: %s\n", result.c_str());
      throw std::runtime_error(_("WMI setting security blanket failed.\n"));
    }
  } else {
    std::string result = wmiResultToString(hr);
    logError("Could not connect to target machine. Error: %s\n", result.c_str());
    throw std::runtime_error(_("Could not connect to target machine.\n"));
  }
}

//--------------------------------------------------------------------------------------------------

WmiServices::~WmiServices() {
  logDebug("Destroying services\n");
  deallocate_locator();
}

//--------------------------------------------------------------------------------------------------

void WmiServices::allocate_locator() {
  logDebug("Allocating wbem locator\n");

  base::MutexLock lock(_locator_mutex);
  if (_locator_refcount == 0) {
    HRESULT hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                                  reinterpret_cast<void**>(&_locator));

    if (FAILED(hr)) {
      std::string result = wmiResultToString(hr);
      logError("Could not create wbem locator. Error: %s\n", result.c_str());
      throw std::runtime_error("Internal error: Instantiation of IWbemLocator failed.\n");
    }
  }

  _locator_refcount++;
}

//--------------------------------------------------------------------------------------------------

void WmiServices::deallocate_locator() {
  logDebug("Deallocating wbem locator\n");

  base::MutexLock lock(_locator_mutex);
  if (_locator_refcount > 0) {
    _locator_refcount--;
    if (_locator_refcount == 0) {
      _locator->Release();
      _locator = NULL;
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Executes a wmi query and returns the queried objects as grt dicts.
 *
 * @param query The query to send. It must be of type WQL.
 * @param node The target machine where to execute the query on. Leave blank for localhost.
 * @param user The user name for authentication on a remote box. Ignored for localhost (the current
 *             user is used in this case).
 * @param password The password for the given user. Also ignore for localhost.
 * @return A list of dictionaries containing an entry for each object returned by the query, with
 *         name/value pairs of object properties.
 */
grt::DictListRef WmiServices::query(const std::string& query) {
  logDebug3("Running wmi query: %s\n", query.c_str());

  // Making this function explicitly thread-safe might be unnecessary as we don't have
  // any data which is allocated/deallocated concurrently. But since we know we will be called
  // from different threads we play safe here, as it does not harm either.
  base::MutexLock lock(_locator_mutex);

  grt::DictListRef queryResult(grt::Initialized);

  // Execute the given query.
  CComPtr<IEnumWbemClassObject> enumerator;
  CComBSTR converted_query = string2Bstr(query);
  HRESULT hr = _services->ExecQuery(L"WQL", converted_query, WBEM_FLAG_FORWARD_ONLY, NULL, &enumerator);

  if (SUCCEEDED(hr)) {
    // We also need to set a security context for the enumerator or the next code will fail
    // with "access denied" on remote boxes.
    hr = CoSetProxyBlanket(enumerator, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL,
                           RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    // Read the returned results and create our own response.
    ULONG count = 0;
    while (true) {
      CComPtr<IWbemClassObject> wbemObject = NULL;
      hr = enumerator->Next(WBEM_INFINITE, 1L, &wbemObject, &count);
      if (FAILED(hr))
        throw std::runtime_error(_("WMI enumeration failed.\n\n") + wmiResultToString(hr));

      if (count == 0)
        break;

      SAFEARRAY* names;
      wbemObject->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, &names);

      long lowBound, highBound;
      SafeArrayGetLBound(names, 1, &lowBound);
      SafeArrayGetUBound(names, 1, &highBound);

      DictRef resultNames(true);
      for (long i = lowBound; i <= highBound; i++) {
        CComBSTR name;
        SafeArrayGetElement(names, &i, &name);
        CW2A nameString(name, CP_UTF8);

        // Read the actual value for the given property.
        VARIANT value;
        hr = wbemObject->Get(name, 0, &value, NULL, NULL);

        if (SUCCEEDED(hr)) {
          // In order to ease the value transport everything is converted to a string.
          resultNames.gset((LPSTR)nameString, variant2string(value));

          VariantClear(&value);
        } else {
          char* name_ = _com_util::ConvertBSTRToString(name);

          std::string result = wmiResultToString(hr);
          logError("Couldn't get the value for %s. Error: %s\n", name_, result.c_str());
          delete[] name_;
        }
      }
      SafeArrayDestroy(names);

      queryResult.insert(resultNames);
    }
  } else {
    std::string result = wmiResultToString(hr);
    logError("Query execution failed. Error: %s\n", result.c_str());
    throw std::runtime_error("WMI query execution failed");
  }

  return queryResult;
}

//--------------------------------------------------------------------------------------------------

/**
 * Allows to work with a Windows service locally or remotely. The result depends on the action.
 *
 * @param service The display name of the service (as stored in the server instance) to act on.
 * @param action The action to execute. Supported are:
 *   - status: return the status of the service
 *   - start: start the service if it is not running.
 *   - stop: stop a running service.
 * @return A string describing the outcome of the action.
 *   - for status: either running, stopping, starting, stopped, unknown
 *   - for start: either completed, already-running, already-starting, stopping, error
 *   - for stop: either completed, already-stopped, already-stopping, starting, error
 */
std::string WmiServices::serviceControl(const std::string& service, const std::string& action) {
  logDebug3("Running wmi service control query for service: %s (action: %s)\n", service.c_str(), action.c_str());

  base::MutexLock lock(_locator_mutex);

  CComBSTR instancePath = string2Bstr("Win32_Service.Name='" + service + "'");

  CComPtr<IWbemClassObject> serviceInstance;
  HRESULT hr = _services->GetObject(instancePath, 0, NULL, &serviceInstance, NULL);
  if (FAILED(hr)) {
    std::string result = wmiResultToString(hr);
    logError("Query execution failed. Error: %s\n", result.c_str());
    return "error, see log";
  }

  VARIANT value;
  hr = serviceInstance->Get(L"State", 0, &value, NULL, NULL);
  if (FAILED(hr)) {
    std::string result = wmiResultToString(hr);
    logError("Could not get state value for the service. Error: %s\n", result.c_str());
    return "unknown";
  }

  std::string state = base::tolower(variant2string(value));
  VariantClear(&value);

  if (action == "status")
    return state;
  else {
    CComBSTR methodName;
    std::string waitState;
    std::string expectedState;
    if (action == "start") {
      if (state == "running")
        return "already-running";
      if (state == "stop pending")
        return "stopping";
      if (state == "start pending")
        return "already-starting";
      if (state != "stopped")
        return "error";

      methodName = "StartService";
      waitState = "start pending";
      expectedState = "running";
    } else if (action == "stop") {
      if (state == "stopped")
        return "already-stopped";
      if (state == "stop pending")
        return "stopping";
      if (state == "start pending")
        return "starting";
      if (state != "running")
        return "error";

      methodName = "StopService";
      waitState = "stop pending";
      expectedState = "stopped";
    }

    if (methodName.Length() > 0) {
      CComBSTR serviceClassPath = "Win32_Service";
      CComPtr<IWbemClassObject> serviceClass;
      CComPtr<IWbemClassObject> outInstance;
      CComPtr<IWbemClassObject> inClass;
      CComPtr<IWbemClassObject> inInstance;
      CComPtr<IWbemClassObject> outClass;

      hr = _services->GetObject(serviceClassPath, 0, NULL, &serviceClass, NULL);
      if (SUCCEEDED(hr))
        hr = serviceClass->GetMethod(methodName, 0, &inClass, &outClass);
      else {
        char* serviceClassPath_ = _com_util::ConvertBSTRToString(serviceClassPath);
        std::string result = wmiResultToString(hr);
        logError("Could not get object for service class path: %s. Error: %s\n", serviceClassPath_, result.c_str());
        delete[] serviceClassPath_;
      }

      if (SUCCEEDED(hr) && inClass)
        hr = inClass->SpawnInstance(0, &inInstance);
      else {
        char* methodName_ = _com_util::ConvertBSTRToString(methodName);
        std::string result = wmiResultToString(hr);
        logError("Could not get in/out class for method: %s. Error: %s\n", methodName_, result.c_str());
        delete[] methodName_;
      }

      if (SUCCEEDED(hr))
        hr = _services->ExecMethod(instancePath, methodName, 0, NULL, inInstance, &outInstance, NULL);
      else {
        std::string result = wmiResultToString(hr);
        logError("Could not spawn in-class instance. Error: %s\n", result.c_str());
      }

      if (FAILED(hr)) {
        char* serviceClassPath_ = _com_util::ConvertBSTRToString(serviceClassPath);
        char* methodName_ = _com_util::ConvertBSTRToString(methodName);
        std::string result = wmiResultToString(hr);
        logError("Could not execute method %d at path %s. Error: %s\n", methodName_, serviceClassPath_, result.c_str());
        delete[] methodName_;
        delete[] serviceClassPath_;

        return "error, see log";
      }

      // Try to figure out what happened if the call as such succeeded but returned an error code.
      // Service methods return uint32 error codes.
      VARIANT returnValue;
      hr = outInstance->Get(L"ReturnValue", 0, &returnValue, NULL, NULL);

      if (FAILED(hr)) {
        std::string result = wmiResultToString(hr);
        logError("Could not get the return value of the query. Error: %s\n", result.c_str());

        return "error, see log";
      }

      if (returnValue.vt != VT_EMPTY) {
        unsigned int result = V_UI4(&returnValue);
        VariantClear(&returnValue);
        if (result != 0) {
          std::string text = serviceResultToString(result);
          logError("Variant conversion for return value failed. Error: %s\n", text.c_str());

          return "error, see log";
        }
      }

      // So far everything worked fine. Now check the service if it changed as we intended.
      // Wait in a loop with increasing time steps for the result. If there was not the expected
      // change after at most 34 secs then assume an error and return.
      int i = 10;
      int sleepTime = 250; // Start with 1/4 sec steps, to return as quick as possible if things are fine.
      do {
        Sleep(sleepTime);

        VARIANT value;
        hr = serviceInstance->Get(L"State", 0, &value, NULL, NULL);
        if (FAILED(hr)) {
          std::string result = wmiResultToString(hr);
          logError("Could not get state value for the service. Error: %s\n", result.c_str());

          return "unknown";
        }

        state = base::tolower(variant2string(value));
        VariantClear(&value);

        if (state != waitState)
          break;

        if (--i == 0) {
          i = 10;
          sleepTime *= 2; // Double the time we wait for the next check of the service.
          if (sleepTime >= 4000)
            break;
        }
      } while (true);

      if (state != expectedState) {
        logError("Timeout waiting for the service to change status. Returning error to caller.\n");
        return "error";
      }

      return "completed";
    }
  }

  return "unknown";
}

//--------------------------------------------------------------------------------------------------

/**
 * Retrieves operating system values like total available memory, OS description, boot device etc.
 *
 * @param what Specifies which value to query and return. Supported are all properties which belong
 * to the Win32_OperatingSystem class (see http://msdn.microsoft.com/en-us/library/aa394239%28VS.85%29.aspx).
 */
std::string WmiServices::systemStat(const std::string& what) {
  logDebug3("Running wmi system stat call (what: %s)\n", what.c_str());

  base::MutexLock lock(_locator_mutex);

  CComBSTR instancePath = string2Bstr("Win32_OperatingSystem");

  CComPtr<IEnumWbemClassObject> enumerator;
  HRESULT hr =
    _services->ExecQuery(L"WQL", L"select * from Win32_OperatingSystem", WBEM_FLAG_FORWARD_ONLY, NULL, &enumerator);

  std::string result = "-1";
  if (SUCCEEDED(hr)) {
    hr = CoSetProxyBlanket(enumerator, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL,
                           RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    CComPtr<IWbemClassObject> wbemObject = NULL;
    ULONG count = 0;
    hr = enumerator->Next(WBEM_INFINITE, 1L, &wbemObject, &count);
    if (FAILED(hr) || count == 0)
      return "-1";

    CComBSTR propertyName = string2Bstr(what);
    VARIANT value;
    hr = wbemObject->Get(propertyName, 0, &value, NULL, NULL);

    if (SUCCEEDED(hr)) {
      result = base::tolower(variant2string(value));
      VariantClear(&value);
    } else {
      char* propertyName_ = _com_util::ConvertBSTRToString(propertyName);

      std::string result = wmiResultToString(hr);
      logError("Could not get the value for property %s. Error: %s\n", propertyName_, result.c_str());
      delete[] propertyName_;
    }
  } else {
    std::string result = wmiResultToString(hr);
    logError("Could not run the system stat query. Error: %s\n", result.c_str());
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

WmiMonitor* WmiServices::startMonitoring(const std::string& parameter) {
  return new WmiMonitor(_services, parameter);
}

//--------------------------------------------------------------------------------------------------

void wmi::WmiServices::stopMonitoring(WmiMonitor* monitor) {
  delete monitor;
}

//--------------------------------------------------------------------------------------------------
