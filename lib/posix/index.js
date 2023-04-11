"use strict";
var path = require("path");

// Attempt to load the bindings module from various possible locations
function load_extension() {
  var ext_dirs = [
      "build/bindings",
      "build/Release",
      "out/Release",
      "Release",
      "build/Debug",
      "out/Debug",
    ],
    i;
  for (i in ext_dirs) {
    try {
      return require(path.join(__dirname, "../..", ext_dirs[i], "posix.node"));
    } catch (error) {
      if (!/Cannot find module/.test(error.message)) {
        throw error;
      }
    }
  }
  throw new Error("unable to load the node-posix extension module");
}

var posix = load_extension();

module.exports = {
  getrlimit: posix.getrlimit,
  setrlimit: posix.setrlimit,
};
