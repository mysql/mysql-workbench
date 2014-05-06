import warnings

# Display all deprecation warnings that are ignored by default:
warnings.simplefilter('always', DeprecationWarning, lineno=0)

warnings.warn("Use 'from workbench import db_utils' instead of importing db_utils directly. This will be deprecated in the future", 
              DeprecationWarning,
              stacklevel=2)

from workbench.db_utils import *
