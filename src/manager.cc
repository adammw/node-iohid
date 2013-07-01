#include <node.h>
#include <v8.h>
#include "manager.h"

using namespace v8;

Manager::Manager() {
  hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
  if (!hidManager) {
    ThrowException(Exception::Error(String::New("IOHIDManagerCreate failed")));
    return;
  }
  IOHIDManagerSetDeviceMatching(hidManager, NULL);
}
Manager::~Manager() {
  CFRelease(hidManager);
}

Persistent<Function> Manager::constructor;

void Manager::Init(Handle<Object> exports) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("IOHIDManager"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("setDeviceMatchingCriteria"),
      FunctionTemplate::New(SetDeviceMatchingCriteria)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("setDeviceMatchingCallback"),
      FunctionTemplate::New(SetDeviceMatchingCallback)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("setDeviceRemovalCallback"),
      FunctionTemplate::New(SetDeviceRemovalCallback)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("open"),
      FunctionTemplate::New(Open)->GetFunction());

  constructor = Persistent<Function>::New(tpl->GetFunction());
  exports->Set(String::NewSymbol("Manager"), constructor);
}

void Manager::RunCFLoop(uv_work_t *req) {
  CFRunLoopRun();
}

void Manager::AfterRunCFLoop(uv_work_t *req, int status) {
  //TODO: emit close event
}

void Manager::Open() {
  //TODO: support kIOHIDOptionsTypeSeizeDevice
  if (open)
    return;

  IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
  IOReturn ret = IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);
  if (ret != kIOReturnSuccess) {
    ThrowException(Exception::Error(String::New("IOHIDManagerOpen failed")));
    return;
  }
  open = true;

  uv_work_t* req = new uv_work_t;
  uv_queue_work(uv_default_loop(), req, Manager::RunCFLoop, Manager::AfterRunCFLoop);
}

void Manager::SetDeviceMatchingMultiple(CFArrayRef multiple) {
  IOHIDManagerSetDeviceMatchingMultiple(hidManager, multiple);
}

void Manager::SetDeviceMatching(CFDictionaryRef matching) {
  IOHIDManagerSetDeviceMatching(hidManager, matching);
}

void Manager::RegisterDeviceMatchingCallback(Local<Function> cb) {
  deviceMatchingCallback = cb;
  IOHIDDeviceCallback cb2 = Manager::HandleDeviceMatchingCallback;
  IOHIDManagerRegisterDeviceMatchingCallback(hidManager, cb2, (void*)this);
}

void Manager::RegisterDeviceRemovalCallback(Local<Function> cb) {
  deviceRemovalCallback = cb;
  IOHIDDeviceCallback cb2 = Manager::HandleDeviceRemovalCallback;
  IOHIDManagerRegisterDeviceRemovalCallback(hidManager, cb2, (void*)this);
}

void Manager::UnregisterDeviceMatchingCallback() {
  IOHIDManagerRegisterDeviceMatchingCallback(hidManager, NULL, NULL);
}

void Manager::UnregisterDeviceRemovalCallback() {
  IOHIDManagerRegisterDeviceRemovalCallback(hidManager, NULL, NULL);
}

void Manager::HandleDeviceMatchingCallback(void* context, IOReturn result, void* sender, IOHIDDeviceRef device) {
  Manager* obj = (Manager*)context;
  obj->InvokeDeviceMatchingCallback(device);
}

void Manager::HandleDeviceRemovalCallback(void* context, IOReturn result, void* sender, IOHIDDeviceRef device) {
  Manager* obj = (Manager*)context;
  obj->InvokeDeviceRemovalCallback(device);
}

void Manager::InvokeDeviceMatchingCallback(IOHIDDeviceRef device) {
  const unsigned argc = 1;
  Local<Value> argv[argc] = { Local<Value>::New(String::New("arguments go here")) };
  deviceMatchingCallback->Call(Context::GetCurrent()->Global(), argc, argv);
}

void Manager::InvokeDeviceRemovalCallback(IOHIDDeviceRef device) {
  const unsigned argc = 1;
  Local<Value> argv[argc] = { Local<Value>::New(String::New("arguments go here")) };
  deviceRemovalCallback->Call(Context::GetCurrent()->Global(), argc, argv);
}

Handle<Value> Manager::New(const Arguments& args) {
  HandleScope scope;

  if (!args.IsConstructCall()) {
    return NewInstance(args);
  }

  Manager* obj = new Manager();
  obj->Wrap(args.This());

  return args.This();
}

Handle<Value> Manager::NewInstance(const Arguments& args) {
  HandleScope scope;

  const unsigned argc = 1;
  Handle<Value> argv[argc] = { args[0] };
  Local<Object> instance = constructor->NewInstance(argc, argv);

  return scope.Close(instance);
}

Handle<Value> Manager::Open(const Arguments& args) {
  HandleScope scope;

  Manager* obj = ObjectWrap::Unwrap<Manager>(args.This());
  obj->Open();
  //TODO: accept a callback
  return scope.Close(Undefined());
}

Handle<Value> Manager::SetDeviceMatchingCriteria(const Arguments& args) {
  HandleScope scope;

  Manager* obj = ObjectWrap::Unwrap<Manager>(args.This());

  if (args.Length() != 1) {
    ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    return scope.Close(Undefined());
  }

  Local<Value> matchingV8Obj = args[0];
  if (args[0]->IsArray()) {
    CFArrayRef matching = V8ArrayToCFArray(Local<Array>::Cast(matchingV8Obj));
    obj->SetDeviceMatchingMultiple(matching);
    CFRelease(matching);
  } else if (args[0]->IsObject()) {
    CFDictionaryRef matching = V8ObjectToCFDictionary(matchingV8Obj->ToObject());
    obj->SetDeviceMatching(matching);
    CFRelease(matching);
  }

  return scope.Close(Undefined());
}

Handle<Value> Manager::SetDeviceMatchingCallback(const Arguments& args) {
  HandleScope scope;

  if (args.Length() != 1) {
    ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    return scope.Close(Undefined());
  }

  if (!args[0]->IsFunction() && !args[0]->IsNull()) {
    ThrowException(Exception::TypeError(String::New("Argument must be a function or null")));
    return scope.Close(Undefined());
  }

  Manager* obj = ObjectWrap::Unwrap<Manager>(args.This());
  if (args[0]->IsNull()) {
    obj->UnregisterDeviceMatchingCallback();
  } else {
    obj->RegisterDeviceMatchingCallback(Local<Function>::Cast(args[0]));
  }

  return scope.Close(Undefined());
}

Handle<Value> Manager::SetDeviceRemovalCallback(const Arguments& args) {
  HandleScope scope;

  if (args.Length() != 1) {
    ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    return scope.Close(Undefined());
  }

  if (!args[0]->IsFunction()) {
    ThrowException(Exception::TypeError(String::New("Argument must be a function")));
    return scope.Close(Undefined());
  }

  Manager* obj = ObjectWrap::Unwrap<Manager>(args.This());
  if (args[0]->IsNull()) {
    obj->UnregisterDeviceRemovalCallback();
  } else {
    obj->RegisterDeviceRemovalCallback(Local<Function>::Cast(args[0]));
  }

  return scope.Close(Undefined());
}

CFArrayRef Manager::V8ArrayToCFArray(const Local<Array>& obj) {
  CFMutableArrayRef arr = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

  for(int i = 0, l = obj->Length(); i < l; i++) {
    Local<Value> v8Val = obj->Get(Number::New(i));
    CFTypeRef cfVal = V8ValueToCFType(v8Val);
    CFArrayAppendValue(arr, cfVal);
    CFRelease(cfVal);
  }
  return arr;
}

CFDictionaryRef Manager::V8ObjectToCFDictionary(const Local<Object>& obj) {
  CFMutableDictionaryRef dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

  Local<Array> keys = obj->GetOwnPropertyNames();
  for(int i = 0, l = keys->Length(); i < l; i++) {
    Local<String> v8Key = keys->Get(Number::New(i))->ToString();
    Local<Value> v8Val = obj->Get(v8Key);
    CFTypeRef cfKey = V8ValueToCFType(v8Key);
    CFTypeRef cfVal = V8ValueToCFType(v8Val);
    CFDictionarySetValue(dict, cfKey, (void*)cfVal);
    CFRelease(cfKey);
    CFRelease(cfVal);
  }
  return dict;
}

CFTypeRef Manager::V8ValueToCFType(const Local<Value>& obj) {
  CFTypeRef ret = NULL;
  if (obj->IsNumber()) {
    int num = obj->Int32Value();
    ret = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &num);
  } else if (obj->IsArray()) {
    ret = V8ArrayToCFArray(Local<Array>::Cast(obj));
  } else if (obj->IsObject()) {
    ret = V8ObjectToCFDictionary(obj->ToObject());
  } else if (obj->IsString() || obj->IsStringObject()) {
    ret = CFStringCreateWithCString(kCFAllocatorDefault, *v8::String::Utf8Value(obj->ToString()), kCFStringEncodingUTF8);
  } else {
    ThrowException(Exception::Error(String::New("Cannot convert V8 Object to CoreFoundation Type")));
  }
  return ret;
}
