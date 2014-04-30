# Copyright (c) 2012, Oracle and/or its affiliates. All rights reserved.
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

import re

debug = 0

class MiniTemplate:
    def __init__(self, templ):
        self._tokens = re.split("({{[^}]+}}|\[\[[^]]+\]\])", templ)

    def t2l(self, tok):
        return "line %s " % sum([tok.count("\n") for tok in self._tokens[:tok]])

    def render_(self, data, i, result, context):
        assert data is not None
      
        if debug:
            import traceback
            print "ENTER render_() from %s" % traceback.format_stack()[-2].split(",", 1)[-1].strip().split("\n")[0].strip()
    
        def get(d, key, default=""):
            for k in key.split("."):
                if d is None:
                    #raise ValueError("%s not in %s" % (key, od.keys()))
                    return default
                if hasattr(d, "__getitem__"):
                    d = d[k]
                else:
                    d = getattr(d, k)
            return d

        while i < len(self._tokens):
            orig_token = token = self._tokens[i]
            if debug:
                print "--"*len(context)+"> process %s: %s, token: %s (%i)" % (self.t2l(i), ".".join(context), token.strip(), i)
            if token.startswith("{{") and token.endswith("}}"):
                token = token[2:-2]
                try:
                    if token[0] == '#':
                        l = get(data, token[1:])
                        if l is not None:
                            out = str(len(l))
                        else:
                            out = "0"
                    else:
                        out = get(data, token)
                except KeyError:
                    raise KeyError("%s: No value for key %s. context: %s possible: %s" % (self.t2l(i), orig_token, ".".join(context), data.keys()))
                except ValueError, e:
                    raise ValueError("%s: %s. context: %s possible: %s" % (self.t2l(i), e, ".".join(context), data.keys()))
                    
                if out is not None:
                    result.append(out)
            elif token.startswith("[[") and token.endswith("]]"):
                if debug:
                    print "Found block %s" % token
                token = token[2:-2]
                if token[0] in ("/", "!") and token[1:] == (context[-1] if not context[-1].startswith("[") else context[-2]):
                    if debug:
                        print "%s: leaving context %s through %s at %s" % (self.t2l(i), ".".join(context), token, i)
                    if token[0] == "!":
                        # skip until the real end of the block
                        count = 1
                        i += 1
                        while i < len(self._tokens):
                            if self._tokens[i] == "[[/"+token[1:]+"]]":
                                count -= 1
                                if count == 0:
                                    break
                            elif self._tokens[i] in ("[["+token[1:]+"]]", "[[?"+token[1:]+"]]"):
                                count += 1
                            i += 1
                    return i
                elif token[0] == "?":
                    token = token[1:]
                    enter = False
                    if token.startswith("if|"):
                        try:
                            enter = eval(token[3:], data, data)
                            if debug:
                                print "%s: evaluated %s in context %s to %s" % (self.t2l(i), token, data.keys(), enter)
                            token = "if"
                        except Exception, exc:
                            print "%s: Error evaluating %s in context %s: %s" % (self.t2l(i), token, data.keys(), exc)
                            raise

                    if enter or data.has_key(token) and get(data, token):
                        if debug:
                            print "%s: entering context %s at %s" % (self.t2l(i), ".".join(context + [token]), i+1)
                        i = self.render_(data, i+1, result, context+[token])
                    else:
                        if debug:
                            print "%s: context %s at %s has no value for block %s, trynig to find !%s or /%s" % (self.t2l(i), ".".join(context + [token]), i+1, token, token, token)
                        count = 1
                        i += 1
                        while i < len(self._tokens):
                            if self._tokens[i] == "[[!"+token+"]]" and count == 1:
                                i = self.render_(data, i+1, result, context + [token])
                                i += 1
                                count = 0
                                break
                            elif self._tokens[i] == "[[/"+token+"]]":
                                count -= 1
                                if count == 0:
                                    break
                            elif self._tokens[i] in ("[["+token+"]]", "[[?"+token+"]]"):
                                count += 1
                            i += 1
                        if count != 0:
                            print "/%s not found!" % token
                else:
                    try:
                        sub = get(data, token)
                    except KeyError:
                        raise KeyError("%s: No value for key %s. context: %s possible: %s" % (self.t2l(i), orig_token, ".".join(context), data.keys()))

                    if type(sub) is list:
                        if debug:
                            print "%s: token %s in context %s is a list of size %i" % (self.t2l(i), ".".join(context), token, len(sub))
                        k = i
                        for j, item in enumerate(sub):
                            itemd = dict(item)
                            itemd[":#"] = str(j+1)
                            if j < len(sub)-1:
                                itemd["needsep"] = 1
                            else:
                                itemd["needsep"] = 0
                            if debug:
                                print "%s: entering lcontext %s at %s" % (self.t2l(i), ".".join(context + [token, "[%s]"%j]), i+1)
                            k = self.render_(itemd, i+1, result, context + [token, "[%s]"%j])
                        if not sub:
                            # empty list, go through tokens until we find the closing one
                            count = 1
                            k += 1
                            while k < len(self._tokens):
                                if debug:
                                    print "empty list %s, skip %s (%i)" % (token, self._tokens[k], count)
                                if self._tokens[k] == "[[/"+token+"]]":
                                    count -= 1
                                    if count == 0:
                                        break
                                elif self._tokens[k] in ("[["+token+"]]", "[[?"+token+"]]"):
                                    count += 1
                                k += 1
                        i = k
                    else:
                        if debug:
                            print "%s: entering context %s at %s" % (self.t2l(i), ".".join(context + [token]), i+1)
                        i = self.render_(sub, i+1, result, context + [token])
            else:
                result.append(token)
            i += 1
        if debug:
            print "leaving at token %i" % i
        return i

    def render(self, data):
        l = []
        self.render_(data, 0, l, [])
        return "".join([str(s) for s in l])




if __name__ == "__main__":
    template = """
{{title}}

[[objects]]
{{name}}

There are {{#subobjects}} objects in this object:
{{:#}}.[[subobjects]]{{:#}} - {{name}}
    [[?error]]ERROR![[/error]][[/subobjects]]

Type: {{thing.type}}
[[thing]]
Value: {{value}}
[[/thing]]
[[/objects]]
"""

    data = {
  "title" : "Title",
  "name" : "Some Name",
  "objects" : [
    {
      "name" : "object1",
      "subobjects" : [
            { "name" : "subobject1", "error" : 1},
      ],
      "thing" : {
          "type" : "int",
          "value" : 12345
      }
    },
    {
      "name" : "object2",
      "subobjects" : [
            { "name" : "subobject1of2", "error" : 0 },
      ],
      "thing" : {
          "type" : "str",
          "value" : "qqqq"
      }
    },
  ]
}

    tem = MiniTemplate(template)
    print tem.render(data).replace("\\\n", "")

