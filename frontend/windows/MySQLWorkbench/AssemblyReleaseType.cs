using System;

namespace MySQL.GUI.Workbench
{
  [AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
  class AssemblyReleaseTypeAttribute: Attribute
  {
    string _release_type;
    public AssemblyReleaseTypeAttribute() : this(string.Empty) 
    { 
    }
    
    public AssemblyReleaseTypeAttribute(string value) 
    {
      _release_type = value; 
    }
    
    public string ReleaseType
    {
      get { return _release_type; }
    }
  }
}
