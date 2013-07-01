{
  "targets": [
    {
      "target_name": "iohid",
      "sources": [ "src/iohid.cc", "src/manager.cc" ],
      "link_settings": {
          "libraries": [
            "CoreFoundation.framework",
            "IOKit.framework"
          ],
        }
    }
  ]
}