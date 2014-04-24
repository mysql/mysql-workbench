import types
import copy

#-------------------------------------------------------------------------------
def ver_cmp(v1, v2):
    minlen = min(len(v1), len(v2))
    return cmp(v1[0:minlen], v2[0:minlen])

#===============================================================================
#
#===============================================================================
# Class that represents option definition. Must be r/o. Init from opts.py
# dict for an option. All the values
class OptionDef(object):
    name        = None
    caption     = None
    description = None
    values      = None
    versions    = None

    #---------------------------------------------------------------------------
    def __init__(self, option_dict):
        self.name        = option_dict.get("name")
        self.caption     = option_dict.get("caption")
        self.description = option_dict.get("description")
        self.versions    = option_dict.get("versions")
        self.values      = option_dict.get("values")
        self.disabledby  = option_dict.get("disabledby")

        # Fix empty values

        if type(self.versions) is not list:
            self.versions = ((), (), ())
            print "Option '%s' has no versions" % self.name

    #---------------------------------------------------------------------------
    def __str__(self):
        r = ["Option"]
        for i in dir(self):
            if i[0] != '_':
                attr = getattr(self, i)
                if type(attr) is not types.MethodType:
                    r.append("%s = %s"%(i, str(attr)))
        return "\n  ".join(r)

    #---------------------------------------------------------------------------
    def is_valid(self):
        return (type(self.values) is list) and (len(self.values) > 0)

    #---------------------------------------------------------------------------
    # The method is used to filter options which are relevant to a server version
    # @version is a tuple, like (5,1), or (5,1,0)
    def is_valid_for_server_version(self, version):
        (inver, vlist, outver) = self.versions

        ret = False

        matched_version = None
        # Check if we have x.y version in list which allows version
        for v in vlist:
            if ver_cmp(version, v) == 0:
                matched_version = v
                ret = True
                break

        # Check that version is within introduced and removed
        # If not drop ret to False
        if matched_version:
            if inver is not None:
                for iv in inver:
                    if matched_version[:2] == iv[:2]:
                        if ver_cmp(version, iv) < 0:
                            ret = False
                        break

            if outver is not None:
                out_ver = None
                for ov in outver:
                    if matched_version[:2] == ov[:2]:
                        if ver_cmp(version, ov) > 0:
                            ret = False
                        break

        return ret


    #---------------------------------------------------------------------------
    def get_corresponding_value(self, version, platform, bits):
        value = None
        if type(self.values) is list and len(self.values) > 0:
                # Walk all values and pick best match
            for cur_value in self.values:
                inversion  = cur_value.get('inversion')
                outversion = cur_value.get('outversion')

                if inversion is None:
                    inversion = (0,0,0)

                if outversion is None:
                    outversion = (99,0,0)

                if version >= inversion and version <= outversion:
                    platform_match = False
                    if 'platform' in cur_value:
                        pl = cur_value['platform']
                        if pl == platform or pl == 'all' or platform == "all" or platform is None:
                            platform_match = True
                    else:
                        platform_match = True

                    if platform_match:
                        value = cur_value
                        break

        return value

#===============================================================================
#
#===============================================================================
class Type:
    _otypes   = {'boolean' : type(True), 'numeric' : type(1), 'string' : type(""), 'set' : type([]), 'enum' : type([]), 'filename' : type(""), 'dirname' : type(""), None: type(None)}
    _type_as_text  = None
    _type_as_ptype = type(None)

    def __init__(self, atype):
        self._type_as_text  = atype
        self._type_as_ptype = self._otypes.get(atype)

    def as_type(self):
        return self._type_as_ptype

    def __str__(self):
        return self._type_as_text

#===============================================================================
#
#===============================================================================
class CfgValue:
    value = None
    line  = None
    def __init__(self, value, line):
        self.value = value
        self.line  = line

    def __repr__(self):
        return "<'%s', line=%s>" % (str(self.value), str(self.line))

#===============================================================================
#
#===============================================================================
"""
"""
class Value:
    value   = None
    enabled = None

    def __init__(self, value, enabled):
        self.value   = value
        self.enabled = enabled

    def __repr__(self):
        return "<'%s', enbl=%s>" % (str(self.value), str(self.enabled))

#-------------------------------------------------------------------------------
def str2bool(value):
    ret = False

    if type(value) == bool:
        ret = value
    else:
        if value and (type(value) is str or type(value) is unicode):
            value = value.strip(" \r\t\n").lower()

        if value == 'checked' or value == "on" or value == "true" or value == "1":
            ret = True
        elif value == 'unchecked' or value == "off" or value == "false" or value == "" or value == "0":
            ret = False
        else:
            raise ValueError("can not convert '%s' to boolean" % str(value))

    return ret

#-------------------------------------------------------------------------------
def str2int(value):
    ret = 0
    if value is not None and value != "":
        ret = int(value)
    return ret

#===============================================================================
#
#===============================================================================
class Option(object):
    odef      = None
    value     = None # Is used as a list of Value instances
    value_def = None
    value_cfg = None
    otype     = None


    #---------------------------------------------------------------------------
    def __init__(self, option_def, version, platform, bits):
        self.odef = option_def
        self.value_def = option_def.get_corresponding_value(version, platform, bits)
        if self.value_def:
            self.otype = Type(self.value_def.get('type'))
            self.default = self.value_def.get("default") if self.value_def else None
            if self.otype.as_type() is type(True) and self.default is not None:
                self.default = str2bool(self.default)

            self.default = Value(self.default, True)
            self.on  = self.value_def.get('on')
            self.off = self.value_def.get('off')
        else:
            self.otype = None
            self.default = None
        self.desc = self.odef.description
        self.option_lookup_call = None
        self.value = []
        self.value_cfg = []
        self.name_in_file = self.odef.name
        self.set_option_lookup_call(None)
        self.reset()

    #---------------------------------------------------------------------------
    def is_valid(self):
        return self.value_def is not None

    #---------------------------------------------------------------------------
    def set_name_as_in_file(self, name):
        self.name_in_file = name

    #---------------------------------------------------------------------------
    def get_option_type(self):
        return self.otype

    #---------------------------------------------------------------------------
    def get_name(self):
        return self.odef.name

    #---------------------------------------------------------------------------
    def get_description(self):
        return self.desc

    #---------------------------------------------------------------------------
    def default_value(self):
        return self.default

    #---------------------------------------------------------------------------
    def get_values(self):
        return copy.copy(self.value)

    #---------------------------------------------------------------------------
    def get_cfg_values(self):
        return self.value_cfg

    #---------------------------------------------------------------------------
    """
        cast2opt converts passed list of values to the type of the option. This
        method is usefull when handling input from UI, or when casting config values
        and values from ui to a common format for further comparision
    """
    def cast2opt_type(self, values_list):
        new_values = []

        if type(values_list) is not list:
            raise Exception("Option.cast2opt expects list of Value objects")

        if self.otype is None:
            raise Exception("Option '%s' has no type defined" % str(self.get_name()))

        converter = None
        otype = self.otype.as_type()

        if otype is type(True):  # 'boolean'
            converter = str2bool
        elif otype is type(1):   # 'numeric'
            converter = str2int
        elif otype is type([]):  # 'set', 'enum'
            converter = str
        elif otype is type(""):  # 'string', 'filename', 'dirname'
            converter = str

        if not converter:
            raise Exception("Option.cast2opt: option '%s' has unknown type '%s'" % (str(self.get_name()), str(self.otype)))

        for v in values_list:
            try:
                new_v = copy.copy(v)
                if otype is not type(True): # Boolean values are modified using enabled field
                    new_v.value = converter(v.value) if v.value is not None else None
                else:
                    if v.value is not None: # v.value == None is usually when data passed from UI
                        new_v.enabled = converter(v.value)
                    else:
                        new_v.value = str(new_v.enabled)
                new_values.append(new_v)
            except Exception, e:
                raise ValueError("Failed Option.cast2opt for option '%s': '%s'" % (str(self.get_name()), str(e)))

        return new_values

    #---------------------------------------------------------------------------
    """ Warning: data arrives in text repr, convert accordingly
        Also note, that data arrives for all values if the option is a multi-line
        even for unchanged ones. Each value is a Value object

        v is a list of Value objects

        Boolean options are set/cleared by Value.enabled and not by value

        Returns: None upon success, otherwise - error message
    """
    def set_values(self, vlist):
        ret = None
        otype = self.otype.as_type()
        values = self.cast2opt_type(vlist)
        if len(values) != len(vlist):
            ret = "Failed type conversion for '%s'" % (str(self.get_name()))

        if otype is type("") or otype is type(1) or otype is type([]):
            # If cast2opt_type did not raise exceptions, then values were converted
            self.value = values
        elif otype is type(True): # self.append_value_from_cfg ensures that self.value_cfg/value have only 1 item (last one) initially
            self.value = [values[0]]
            disabledby_opt = self.lookup_option(self.odef.disabledby) if self.odef.disabledby else None

            if self.value[0].enabled is True:
                if self.off == 'disabledby' and disabledby_opt:
                    disabledby_opt.set_values([Value(False, False)]) # remove disabledby option
                if self.default and self.default.value is True:
                    self.value = []
                    #self.value[0].value = self.value[0].enabled = False # remove option from config file, as it is on by default
                                                                        # otherwise leave the passed value being set to true
            else:
                if self.off == 'disabledby':
                    if disabledby_opt:
                        disabledby_opt.set_values([Value(True, True)])   # add disabledby option
                        self.value = []
                    else:
                        raise Exception("Option '%s' has 'disabledby' off method, but no corresponding disabledby option can be found" % str(self.get_name()))

        return ret


    #---------------------------------------------------------------------------
    def append_value_from_cfg(self, v, line):
        # Accept boolean for bool
        # numbers for numeric
        # and string for the rest
        vtype = type(v)
        otype = self.otype.as_type()
        type_is_fine = False

        if otype is type(True):
            type_is_fine = vtype is type(True) or vtype is type(1)
        elif otype is type(1):
            if vtype is type(1):
                type_is_fine = True
            elif vtype is type(""):
                try:
                    v = int(v)
                    type_is_fine = True
                except:
                    pass
        else:
            type_is_fine = True

        value = [Value(v, True)]
        try:
            value = self.cast2opt_type(value)

            if type_is_fine and otype is not type(True):
                value = value[0]
                self.value_cfg.append(CfgValue(value.value, line))
                self.value.append(Value(value.value, value.enabled))
            else: # With multi-line bools latest takes precedence
                self.value_cfg = [CfgValue(value[0].value, line)]
                self.value     = value


        except ValueError, e:
            # Work on better error signalling
            print "TOLOG: failed append_value_from_cfg"

    #---------------------------------------------------------------------------
    def reset(self):
        self.value     = []
        self.value_cfg = []
        if self.otype and self.otype.as_type() is type(True):
            if self.default and self.default.value is True:
                self.value_cfg.append(CfgValue(True, None))
                self.value.append(Value(True, True))

    #---------------------------------------------------------------------------
    def reset_changed(self):
        self.value = []
        for v in self.value_cfg:
            opt = Value(v.value, True)

            if self.otype.as_type() is type(True):
                opt.value   = v.value
                opt.enabled = v.value

            self.value.append(opt)

    #---------------------------------------------------------------------------
    def get_enum_choices(self):
        return self.value_def.get("choice")

    #---------------------------------------------------------------------------
    def disabledby(self):
        return self.odef.disabledby

    #---------------------------------------------------------------------------
    def lookup_option(self, name):
        return self.option_lookup_call(name) if self.option_lookup_call else None

    #---------------------------------------------------------------------------
    def set_option_lookup_call(self, call):
        self.option_lookup_call = call

    #---------------------------------------------------------------------------
    # Returns tuple with formed lines: (<changed list>, <deleted list>, <added list>)
    def get_changeset(self):
        changed = []
        removed = []
        added   = []

        values_cfg = self.cast2opt_type(self.value_cfg)
        values     = self.cast2opt_type(self.value)

        otype = self.otype.as_type() if self.otype else None

        if otype is type(True):
            orig_value = values_cfg[0] if len(values_cfg) > 0 else None
            new_value  = values[0] if len(values) > 0 else None
            remove = False
            add    = False

            if orig_value is not None and new_value is not None:
                if orig_value.enabled != new_value.enabled:
                    if orig_value.line is None: # That is default value loaded on reset
                        add = True
                    else:
                        text = None
                        value = new_value.enabled
                        if value is True:
                            value = self.on
                            if value == 'name':
                                text = self.name_in_file
                            else:
                                if value is None:
                                    value = '1'
                                text = "%s = %s" % (self.name_in_file, value)
                        elif value is False:
                            value = self.off
                            if value == 'disabledby' or value == 'del':
                                remove = True
                            else:
                                if value is None:
                                    value = '0'
                                text = "%s = %s" % (self.name_in_file, value)
                        elif value is None:
                            remove = True

                        if text is not None and remove == False:
                            changed.append(CfgValue(text, orig_value.line))
            elif orig_value is not None:
                remove = True
            elif new_value is not None:
                if new_value.enabled is True:
                    add = True
                elif new_value.enabled is False and (self.default and self.default.value is True):
                    add = True

            if remove and orig_value.line is not None:
                removed.append(orig_value) # Removal
            if add:
                value = self.on if new_value.enabled is True else self.off

                if value == 'name':
                    value = self.name_in_file
                else:
                    if value is None:
                        value = '1'
                    value = "%s = %s" % (self.name_in_file, value)
                added.append(CfgValue(value, None))
        elif otype is not None:
            zipped = zip(values_cfg, values)
            lcfg   = len(values_cfg)
            lval   = len(values)
            lzip   = len(zipped)

            if lcfg < lval:
                zipped += [(None, v) for v in values[lzip:]]
            elif lcfg > lval:
                zipped += [(v, None) for v in values_cfg[lzip:]]

            for cfg, val in zipped:
                if cfg is not None and val is not None: # we have value and cfg_value, means it could have been changed
                    if val.enabled is False:
                        removed.append(cfg)
                    else:
                        if cfg.value != val.value:
                            value = val.value
                            if str(self.otype) == "filename" or str(self.otype) == "dirname":
                                value = value.strip(" \t")
                                if not value.startswith('"'):
                                    value = '"' + value
                                if not value.endswith('"'):
                                    value = value + '"'
                            changed.append(CfgValue("%s = %s" % (self.name_in_file, value), cfg.line))
                elif cfg:
                    # val removed
                    removed.append(cfg)
                elif val:
                    # val added
                    if val.enabled is True:
                        value = val.value
                        if value is None:
                            value = ""
                        if str(self.otype) == "filename" or str(self.otype) == "dirname":
                            value = '"' + value + '"'
                        added.append(CfgValue("%s = %s" % (self.name_in_file, value), None))

        return (changed, removed, added)

    #---------------------------------------------------------------------------
    def is_in_cfg_file(self):
        return len(self.values_cfg) > 0
