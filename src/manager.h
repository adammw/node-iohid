#ifndef MANAGER_H
#define MANAGER_H

#include <node.h>
#include <IOKit/hid/IOHIDLib.h>

class Manager : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> exports);
  static v8::Handle<v8::Value> NewInstance(const v8::Arguments& args);

 private:
  Manager() throw(char*);
  ~Manager();

  static v8::Persistent<v8::Function> constructor;
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  static v8::Handle<v8::Value> Open(const v8::Arguments& args);
  static void RunCFLoop(uv_work_t *req);
  static void AfterRunCFLoop(uv_work_t *req, int status);
  static v8::Handle<v8::Value> SetDeviceMatchingCriteria(const v8::Arguments& args);
  static v8::Handle<v8::Value> SetDeviceMatchingCallback(const v8::Arguments& args);
  static v8::Handle<v8::Value> SetDeviceRemovalCallback(const v8::Arguments& args);
  static void HandleDeviceMatchingCallback(void* context, IOReturn result, void* sender, IOHIDDeviceRef device);
  static void HandleDeviceRemovalCallback(void* context, IOReturn result, void* sender, IOHIDDeviceRef device);
  static CFDictionaryRef V8ObjectToCFDictionary(const v8::Local<v8::Object>& obj);
  static CFArrayRef V8ArrayToCFArray(const v8::Local<v8::Array>& obj);
  static CFTypeRef V8ValueToCFType(const v8::Local<v8::Value>& obj);

  IOHIDManagerRef hidManager;
  v8::Local<v8::Function> deviceMatchingCallback;
  v8::Local<v8::Function> deviceRemovalCallback;
  void Init();
  void SetDeviceMatchingMultiple(CFArrayRef multiple);
  void SetDeviceMatching(CFDictionaryRef matching);
  void RegisterDeviceMatchingCallback(v8::Local<v8::Function> cb);
  void RegisterDeviceRemovalCallback(v8::Local<v8::Function> cb);
  void UnregisterDeviceMatchingCallback();
  void UnregisterDeviceRemovalCallback();
  void InvokeDeviceMatchingCallback(IOHIDDeviceRef device);
  void InvokeDeviceRemovalCallback(IOHIDDeviceRef device);
  void Open();
  bool open;
};

#endif
