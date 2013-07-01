#include <node.h>
#include <v8.h>
#include "manager.h"

using namespace v8;

Handle<Value> CreateManager(const Arguments& args) {
  HandleScope scope;
  return scope.Close(Manager::NewInstance(args));
}

void InitAll(Handle<Object> exports) {
  Manager::Init(exports);

  exports->Set(String::NewSymbol("createManager"),
      FunctionTemplate::New(CreateManager)->GetFunction());
}

NODE_MODULE(iohid, InitAll)