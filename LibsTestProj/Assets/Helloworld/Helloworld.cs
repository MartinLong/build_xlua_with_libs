using UnityEngine;
using XLua;

public class HelloWorld : MonoBehaviour
{
    // Use this for initialization
    void Start()
    {
        LuaEnv luaenv = new LuaEnv();
        luaenv.AddBuildin("lpeg", XLua.LuaDLL.Lua.LoadLpeg);
        luaenv.AddBuildin("sproto.core", XLua.LuaDLL.Lua.LoadSproto);
        luaenv.DoString(@"
        ------------------------------------
        local lpeg = require 'lpeg'
        print(lpeg.match(lpeg.R '09','123'))
        ------------------------------------
        local core = require 'sproto.core'
        print('sproto required')
        for k, v in pairs(core) do
            print(k..':'..tostring(v))
        end
        ------------------------------------
        ");
        luaenv.Dispose();
    }

    // Update is called once per frame
    void Update()
    {

    }
}
