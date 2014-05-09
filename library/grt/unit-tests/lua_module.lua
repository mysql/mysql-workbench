



function getModuleInfo()
    local moduleInfo= {
      name= "LuaModuleTest",
      version= "1.0.0",
      author= "tester",
      functions= {
          "add2Numbers:i:i,i",
          "div2Numbers:i:i,i",
          "sumList:i:l<i>",
          "errorFunc:i:s,s",
          "testArgInt:i:i,i",
          "testArgDouble:r:r,r",
          "testArgString:s:s,s",
          "testArgIntList:i:l<i>,i",
          "testArgDoubleList:r:l<r>,r",
          "testArgStringList:s:l<s>,i",
          "testArgObjectList:s:l<o>,s",
          "testArgDictList:d:l<d>,s",
          "testArgDict:a:d,s",
          "testArgObject:s:o,s",
          "testRetInt:i:i",
          "testRetDouble:r:i",
          "testRetString:s:i",
          "testRetList:l:i",
          "testRetIntList:l<i>:i",
          "testRetDoubleList:l<r>:i",
          "testRetStringList:l<s>:i",
          "testRetDictList:l<d>:i",
          "testRetObjectList:l<o>:i",
          "testRetDict:d:i",
          "testRetObject:o:i",
          "testRetAny:a:i",
          "doLuaUnitTests:i:"
        },
        extends= ""
    }
    return moduleInfo
end



function add2Numbers(a, b)
    local r= a+b

    return r
end


function div2Numbers(a, b)
    local r= a/b

    return r
end


function sumList(l)
  local i, v
  local r=0
  for i,v in ipairs(l) do
    r=r+v
  end
  return r
end

function errorFunc(s1, s2)
  error("error func")
  return 123
end


function testArgInt(a, b)
    return a+b
end


function testArgDouble(a, b)
    return a+b
end

function testArgString(a, b)
    return a..b
end

function testArgIntList(a, b)
    local i,v,r
    for i, v in ipairs(a) do
        b=b+v
    end
    return b
end

function testArgDoubleList(a, b)
    local i,v,r
    for i, v in ipairs(a) do
        b=b+v
    end
    return b
end

function testArgStringList(a, b)
    return a[b]
end


function testArgObjectList(a, b)
    local i,v,r
    for i, v in ipairs(a) do
        if v.name == b then
            return b
        end
    end
    return nil
end

function testArgDictList(a, b)
    local i,v,r
    for i, v in pairs(grtV.toLua(a)) do
        if v["name"] == b then
            return v
        end
    end
    return nil
end

function testArgDict(a, b)
    return a[b]
end

function testArgObject(a, b)
    return a.name..b
end


function testRetInt(g)
    return 123
end

function testRetDouble(g)
    return 123.456
end

function testRetString(g)
    return "hello"
end

function testRetList(g)
    if g ~= 0 then
        return {1, 2.1, "three"}
    else
        local l= grtV.newList()
        grtV.insert(l, 2)
        grtV.insert(l, 3.1)
        grtV.insert(l, "four")
        return l
    end
end

function testRetIntList(g)
    if g ~= 0 then
        return {1,2,3,4}
    else
        local l= grtV.newList("int")
        grtV.insert(l, 2)
        grtV.insert(l, 3)
        grtV.insert(l, 4)
        grtV.insert(l, 5)
        return l
    end
end

function testRetDoubleList(g)
    if g ~= 0 then
        return {1.1,2.2,3.3,4.4}
    else
        local l= grtV.newList("real")
        grtV.insert(l, 2.2)
        grtV.insert(l, 3.3)
        grtV.insert(l, 4.4)
        grtV.insert(l, 5.5)
        return l
    end
end

function testRetStringList(g)
    if g ~= 0 then
        return {"one","two","three","four"}
    else
        local l= grtV.newList("string")
        grtV.insert(l, "two")
        grtV.insert(l, "three")
        grtV.insert(l, "four")
        grtV.insert(l, "five")
        return l
    end
end

function testRetDictList(g)
    if g ~= 0 then
        return {{k1=1,k2=2}, {k1=11,k2=22}}
    else
        local l= grtV.newList("dict")
        local d1, d2
        d1= grtV.newDict()
        d2= grtV.newDict()
        d1["key1"]=1
        d1["key2"]=2
        d2["key1"]=11
        d2["key2"]=22
        grtV.insert(l, d1)
        grtV.insert(l, d2)
        return l
    end
end

function testRetObjectList(g)
    local o1
    local o2
    local o3

    o1= grtV.newObj("test.Book")
    o1.title="Book1"
    o2= grtV.newObj("test.Book")
    o2.title="Book2"
    o3= grtV.newObj("test.Book")
    o3.title="Book3"

    if g ~= 0 then
        return {o1, o2, o3}
    else
        local d= grtV.newList("object")
        grtV.insert(d, o1)
        grtV.insert(d, o2)
        grtV.insert(d, o3)
        return d
    end
end

function testRetDict(g)
    if g ~= 0 then
        return {k1=1,k2=2,k3=3}
    else
        local d= grtV.newDict()
        d["key1"]=1
        d["key2"]=2
        d["key3"]=3
        return d
    end
end

function testRetObject(g)
    return grtV.newObj("test.Book")
end

function testRetAny(g)
    if g ~= 0 then
        return {12345}
    else
        local l= grtV.newList()
        grtV.insert(l, 123456)
        return l
    end
end



-- #########################################################################
-- Unit tests in lua
-- #########################################################################



function testChangeGlobalBugCrash()
    -- this used to crash
    d= grtV.newDict()
    grtV.setGlobal("/test", d)
end


function testNativeMemberAccess()
    local o= grtV.newObj("test.Book")

    -- check if object members of simple types are returned as natives

    if grtV.toLua(o.title) ~= o.title then
        error("string memeber not returned as string")
    end

    if grtV.toLua(o.pages) ~= o.pages then
        error("int memeber not returned as int")
    end

    if grtV.toLua(o.price) ~= o.price then
        error("dbl memeber not returned as dbl")
    end

end


function testModuleCall()
    -- test calling a module function from Lua
    -- also tests that the order of arguments is being passed
    -- correctly
    local result

    result= LuaModuleTest:add2Numbers(10, 2)
    if result ~= 12 then
        error("Error in module call ")
    end

    result= LuaModuleTest:div2Numbers(10, 2)
    if result ~= 5 then
        error("Error in module call order")
    end
end

function testObjectCreation()
    local o= grtV.newObj("test.Book")
    o.title="The Book Name"

    if o.title ~= "The Book Name" then
        error("object creation ignored args")
    end
end

function testNullArgCrash()
    LuaModuleTest:add2Numbers(nil, nil)
end


function doLuaUnitTests()
    testObjectCreation()

    pcall(testChangeGlobalBugCrash)

    testNativeMemberAccess()

    testModuleCall()

    pcall(testNullArgCrash)
    
    return 0
end
