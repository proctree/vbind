#ifndef _VBIND_H
#define _VBIND_H
#endif

#include<string>
#include<vector>
#include<cstring>
#include<iostream>
#include<climits>

extern "C" {
	#include<lua.h>
	#include<lualib.h>
	#include<lauxlib.h>
}

lua_State* vbind_GetState(); //used to expose functions
int vbind_Split(const std::string& str, const std::string& delim, std::vector<std::string>& parts); //i needed a split function for vbind_Get() but strtok gave me headaches for some reason
void vbind_Init(); //sets up Lua state
void vbind_Close(); //clean up
int vbind_Load(const std::string &filename); //loads a lua file, returns 0 if there were no errors, 1 otherwise
void vbind_Run(); //runs loaded lua file
bool vbind_Do(const std::string &filename); //loads and executes a lua file. return 0 if there were no errors, 1 otherwise
std::string vbind_Get(const std::string &varname); //gets variable from loaded Lua scrip (returns string)
int vbind_GetInteger(const std::string &varname); //vbind_Get, but retuns int
std::vector<std::string> vbind_CallFunction(const std::string &funcname,const std::vector<std::string> &args=std::vector<std::string>()); //calls a lua function
std::vector<int> vbind_CallFunctionI(const std::string &funcname,const std::vector<std::string> &args=std::vector<std::string>()); //calls a lua function, but retuns integer
void vbind_ExposeFunction(int (*f)(lua_State*),const std::string &funcname); //registers a function with lua
void vbind_ExposeNumber(const std::string &name,const double &n); //sets a new global
void vbind_ExposeString(const std::string &name,const std::string &str); //sets a new global

///////////////////////////////////////

#define VBIND_ERROR_GET_BADARG 1		//happens when varname is empty
#define VBIND_ERROR_GET_NOTSTRING 2		//happens when the requested value doesn't exist or isn't the right type
#define VBIND_ERROR_GET_BADTABLE 3		//happens when the table requested isn't formatted like the given argument
#define VBIND_ERROR_GETINTEGER_NOTINT 4	//happens when given variable is not int
#define VBIND_ERROR_GETINTEGER_OORANGE 5//happens when number is outside of int range
#define VBIND_ERROR_CALLFUNCTION_NOTFUNCTION 6 //happens when given function doesn't exist or is not a function
#define VBIND_ERROR_CALLFUNCTIONI_BADSTRING 7 //happens when strtol fails

///////////////////////////////////////

lua_State* L;

lua_State* vbind_GetState(){
	return L;
}

int vbind_Split(const std::string& str, const std::string& delim, std::vector<std::string>& parts){
  size_t start,end = 0;
  while(end<str.size()){
    start=end;
    while (start < str.size()&&(delim.find(str[start]) != std::string::npos)){
      start++;
    }
    end=start;
    while(end < str.size()&&(delim.find(str[end]) == std::string::npos)){
      end++;
    }
    if(end-start != 0){
      parts.push_back(std::string(str, start, end-start));
    }
  }
  return parts.size(); //QoL
}

void vbind_Init(){
	L = luaL_newstate();
	luaL_openlibs(L);
}

void vbind_Close(){
	lua_close(L);
}

int vbind_Load(const std::string &filename){
	return luaL_loadfile(L,filename.c_str());
}

void vbind_Run(){
	lua_pcall(L,0,0,0);
}

bool vbind_Do(const std::string &filename){
	return luaL_dofile(L,filename.c_str());
}

std::string vbind_Get(const std::string &varname){
	std::string varstring;
	std::vector<std::string> vars;
	if(!(vbind_Split(varname,".",vars))){
		throw VBIND_ERROR_GET_BADARG;
	}
	lua_getglobal(L,vars[0].c_str());
	if(vars.size()==1&&!(lua_isstring(L,-1))){
		throw VBIND_ERROR_GET_NOTSTRING;
	}
	if(vars.size()!=1&&!(lua_istable(L,-1))){
		throw VBIND_ERROR_GET_BADTABLE;
	}
	for(unsigned int i=1;i<vars.size();i++){
		lua_getfield(L,-1,vars[i].c_str());
		if(i==vars.size()-1&&!(lua_isstring(L,-1))){
			throw VBIND_ERROR_GET_NOTSTRING;
		}
		if(i!=vars.size()-1&&!(lua_istable(L,-1))){
			throw VBIND_ERROR_GET_BADTABLE;
		}
	}
	varstring=lua_tostring(L,-1);
	lua_pop(L,vars.size());
	return varstring;
}
int vbind_GetInteger(const std::string &varname){
	long preres=0;
	std::string strres;
	try{
		strres=vbind_Get(varname);
	}
	catch(int e){
		throw e;
	}
	const char* p=strres.c_str();
	char* t;
	preres=strtol(strres.c_str(),&t,10);
	if(t==p||errno==ERANGE){
		if(preres==0){
			throw VBIND_ERROR_GETINTEGER_NOTINT;
		}
		else{
			throw VBIND_ERROR_GETINTEGER_OORANGE;
		}
	}
	return (int)preres;
}
std::vector<std::string> vbind_CallFunction(const std::string &funcname,const std::vector<std::string> &args){
	std::vector<std::string> res;
	res.clear();
	lua_getglobal(L,funcname.c_str());
	if(!(lua_isfunction(L,-1))){
		throw VBIND_ERROR_CALLFUNCTION_NOTFUNCTION;
	}
	for(unsigned int i=0;i<args.size();i++){
		lua_pushstring(L,args[i].c_str());
	}
	int b=lua_gettop(L);
	lua_call(L,args.size(),LUA_MULTRET);
	int resn=lua_gettop(L)-b+1+args.size();
	for(int i=0;i<resn;i++){
		res.push_back(lua_tostring(L,-1));
		lua_pop(L,1);
	}
	return res;
}

std::vector<int> vbind_CallFunctionI(const std::string &funcname,const std::vector<std::string> &args){
	std::vector<std::string> preres;
	try{
		preres=vbind_CallFunction(funcname,args);
	}
	catch(int e){
		throw e;
	}
	std::vector<int> res;
	for(unsigned int i=0;i<preres.size();i++){
		const char* p=preres[i].c_str();
		char* t;
		int x=strtol(preres[i].c_str(),&t,10);
		if(t==p||x>=INT_MAX||x<=INT_MIN){
			throw VBIND_ERROR_CALLFUNCTIONI_BADSTRING;
		}
		res.push_back(x);
	}
	return res;
}

void vbind_ExposeFunction(int (*f)(lua_State*),const std::string &funcname){
	lua_register(L,funcname.c_str(),f);
}

void vbind_ExposeNumber(const std::string &name,const double &n){
	lua_pushnumber(L,n);
	lua_setglobal(L,name.c_str());
}

void vbind_ExposeString(const std::string &name,const std::string &str){
	lua_pushstring(L,str.c_str());
	lua_setglobal(L,name.c_str());
}