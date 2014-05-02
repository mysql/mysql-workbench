using MySQL.Grt;

namespace MySQL.GUI.Workbench.Plugins
{
	/// <summary>
	/// Generic GRT Object Editor
	/// </summary>
	public class Plugin
  {
    #region Member Variables

    /// <summary>
		/// The GRT Manager that controlls the GRT
		/// </summary>
		protected GrtManager grtManager;

		/// <summary>
		/// The GRT Object this Editor is operating on
		/// </summary>
		protected GrtValue grtObject;

    #endregion

    #region Constructors

    /// <summary>
		/// Standard constructor
		/// </summary>
		protected Plugin()
		{
		}

		/// <summary>
		/// Overloaded constructor taking the GRT Manager and GRT object to edit
		/// </summary>
		/// <param name="GrtManager">The GRT Manager</param>
		/// <param name="GrtObject">The object to edit</param>
		public Plugin(GrtManager GrtManager, GrtValue GrtObject)
			: this()
		{
			grtManager = GrtManager;
			grtObject = GrtObject;
		}

    #endregion

    #region Virtual Functions

    public virtual void Execute()
    {
    }

    #endregion

  }
}